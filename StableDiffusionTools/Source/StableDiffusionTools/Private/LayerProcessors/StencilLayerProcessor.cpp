#include "LayerProcessors/StencilLayerProcessor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"

FString UStencilLayerProcessor::StencilLayerMaterialAsset = TEXT("/StableDiffusionTools/Materials/SD_StencilMask.SD_StencilMask");

FScopedActorLayerStencil::FScopedActorLayerStencil(const FActorLayer& Layer, bool RestoreOnDelete)
{
	State.CaptureActorLayer(Layer);
}

FScopedActorLayerStencil::FScopedActorLayerStencil(const FScopedActorLayerStencil& ref) {
	State = ref.State;
}

FScopedActorLayerStencil::~FScopedActorLayerStencil()
{
	if (RestoreOnDelete)
		State.RestoreActorLayer();
}

ULayerProcessorOptions* UStencilLayerProcessor::AllocateLayerOptions_Implementation()
{
	return NewObject<UStencilLayerOptions>();
}

void UStencilLayerProcessor::BeginCaptureLayer_Implementation(FIntPoint Size, USceneCaptureComponent2D* CaptureSource, UObject* LayerOptions)
{
	// Set stencil mask properties on layer actors
	FActorLayer ActorLayer;
	if (auto ActorOptions = Cast<UStencilLayerOptions>(LayerOptions)) {
		ActorLayer = (ActorOptions->ActorLayerNameOverride.IsNone()) ? ActorOptions->ActorLayer : FActorLayer{ ActorOptions->ActorLayerNameOverride };
		ActorLayerState.CaptureActorLayer(ActorLayer);
	}

	// Allocate materials
	if (!IsValid(StencilMatInst) && PostMaterial) {
		StencilMatInst = UMaterialInstanceDynamic::Create(PostMaterial, this);
	}
	ActivePostMaterialInstance = StencilMatInst;

	if (CaptureSource) {
		LastBloomState = CaptureSource->ShowFlags.Bloom;
		CaptureSource->ShowFlags.SetBloom(false);
	}

	Super::BeginCaptureLayer_Implementation(Size, CaptureSource, LayerOptions);
}

UTextureRenderTarget2D* UStencilLayerProcessor::CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame, UObject* LayerOptions){
	ULayerProcessorBase::CaptureLayer(CaptureSource, SingleFrame);
	return RenderTarget;
}

void UStencilLayerProcessor::EndCaptureLayer_Implementation(USceneCaptureComponent2D* CaptureSource)
{
	if (CaptureSource)
		CaptureSource->ShowFlags.SetBloom(LastBloomState);

	ActorLayerState.RestoreActorLayer();

	Super::EndCaptureLayer_Implementation(CaptureSource);
}

void FActorLayerStencilState::CaptureActorLayer(const FActorLayer& Layer)
{
	// Set custom depth variable to allow for stencil masks
	IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"));
	if (CVar)
	{
		PreviousCustomDepthValue = CVar->GetInt();
		const int32 CustomDepthWithStencil = 3;
		if (PreviousCustomDepthValue != CustomDepthWithStencil)
		{
			UE_LOG(LogTemp, Log, TEXT("Overriding project custom depth/stencil value to support a stencil pass."));
			// We use ECVF_SetByProjectSetting otherwise once this is set once by rendering, the UI silently fails
			// if you try to change it afterwards. This SetByProjectSetting will fail if they have manipulated the cvar via the console
			// during their current session but it's less likely than changing the project settings.
			CVar->Set(CustomDepthWithStencil, EConsoleVariableFlags::ECVF_SetByProjectSetting);
		}
	}

	// If we're going to be using stencil layers, we need to cache all of the users
	// custom stencil/depth settings since we're changing them to do the mask.
	for (TActorIterator<AActor> ActorItr(GEditor->GetEditorWorldContext().World()); ActorItr; ++ActorItr)
	{
		AActor* Actor = *ActorItr;
		if (Actor)
		{
			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component && Component->IsA<UPrimitiveComponent>())
				{
					UPrimitiveComponent* PrimitiveComponent = CastChecked<UPrimitiveComponent>(Component);
					FStencilValues& Values = ActorLayerSavedStencilValues.Add(PrimitiveComponent);
					Values.StencilMask = PrimitiveComponent->CustomDepthStencilWriteMask;
					Values.CustomStencil = PrimitiveComponent->CustomDepthStencilValue;
					Values.bRenderCustomDepth = PrimitiveComponent->bRenderCustomDepth;
				}
			}

			// The way stencil masking works is that we draw the actors on the given layer to the stencil buffer.
			// Then we apply a post-processing material which colors pixels outside those actors black, before
			// post processing. Then, TAA, Motion Blur, etc. is applied to all pixels. An alpha channel can preserve
			// which pixels were the geometry and which are dead space which lets you apply that as a mask later.
			bool bInLayer = true;

			// If this a normal layer, we only add the actor if it exists on this layer.
			bInLayer = Actor->Layers.Contains(Layer.Name);
			if (bInLayer) {
				UE_LOG(LogTemp, Log, TEXT("Actor found in stencil layer"));
			}

			for (UActorComponent* Component : Actor->GetComponents())
			{
				if (Component && Component->IsA<UPrimitiveComponent>())
				{
					UPrimitiveComponent* PrimitiveComponent = CastChecked<UPrimitiveComponent>(Component);
					// We want to render all objects not on the layer to stencil too so that foreground objects mask.
					if (IsValid(PrimitiveComponent)) {
						PrimitiveComponent->SetVisibility(true);
						PrimitiveComponent->SetCustomDepthStencilValue(bInLayer ? 1 : 0);
						PrimitiveComponent->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
						PrimitiveComponent->SetRenderCustomDepth(true);
					}
				}
			}

			// Immediately commit the stencil changes to the render thread.
			FlushRenderingCommands();
		}
	}
}

void FActorLayerStencilState::RestoreActorLayer()
{
	if (PreviousCustomDepthValue.IsSet())
	{
		IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.CustomDepth"));
		if (CVar)
		{
			if (CVar->GetInt() != PreviousCustomDepthValue.GetValue())
			{
				UE_LOG(LogTemp, Log, TEXT("Restoring custom depth/stencil value to: %d"), PreviousCustomDepthValue.GetValue());
				CVar->Set(PreviousCustomDepthValue.GetValue(), EConsoleVariableFlags::ECVF_SetByProjectSetting);
			}
		}
	}

	// Now we can restore the custom depth/stencil/etc. values so that the main render pass acts as the user expects next time.
	for (TPair<UPrimitiveComponent*, FStencilValues>& KVP : ActorLayerSavedStencilValues){
		if (IsValid(KVP.Key)) {
			KVP.Key->SetCustomDepthStencilValue(KVP.Value.CustomStencil);
			KVP.Key->SetCustomDepthStencilWriteMask(KVP.Value.StencilMask);
			KVP.Key->SetRenderCustomDepth(KVP.Value.bRenderCustomDepth);
		}
	}

	// Immediately commit the stencil changes to the render thread.
	FlushRenderingCommands();
}

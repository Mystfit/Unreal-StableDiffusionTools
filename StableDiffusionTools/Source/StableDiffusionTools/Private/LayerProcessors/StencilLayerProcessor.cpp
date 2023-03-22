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

void UStencilLayerProcessor::BeginCaptureLayer(FIntPoint Size, USceneCaptureComponent2D* CaptureSource)
{
	if (!CaptureSource)
		return;

	LastBloomState = CaptureSource->ShowFlags.Bloom;
	CaptureSource->ShowFlags.SetBloom(false);

	ActorLayerState.CaptureActorLayer(ActorLayer);

	if (!StencilMatInst) {
		StencilMatInst = UMaterialInstanceDynamic::Create(PostMaterial, this);
	}
	ActivePostMaterialInstance = StencilMatInst;

	ULayerProcessorBase::BeginCaptureLayer(Size, CaptureSource);
}

UTextureRenderTarget2D* UStencilLayerProcessor::CaptureLayer(USceneCaptureComponent2D* CaptureSource, bool SingleFrame){
	ULayerProcessorBase::CaptureLayer(CaptureSource, SingleFrame);
	return RenderTarget;
}

void UStencilLayerProcessor::EndCaptureLayer(USceneCaptureComponent2D* CaptureSource)
{
	if (!CaptureSource)
		return;

	ULayerProcessorBase::EndCaptureLayer();

	CaptureSource->ShowFlags.SetBloom(LastBloomState);
	
	ActorLayerState.RestoreActorLayer();
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
					PrimitiveComponent->SetCustomDepthStencilValue(bInLayer ? 1 : 0);
					PrimitiveComponent->SetCustomDepthStencilWriteMask(ERendererStencilMask::ERSM_Default);
					PrimitiveComponent->SetRenderCustomDepth(true);
				}
			}
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
	for (TPair<UPrimitiveComponent*, FStencilValues>& KVP : ActorLayerSavedStencilValues)
	{
		KVP.Key->SetCustomDepthStencilValue(KVP.Value.CustomStencil);
		KVP.Key->SetCustomDepthStencilWriteMask(KVP.Value.StencilMask);
		KVP.Key->SetRenderCustomDepth(KVP.Value.bRenderCustomDepth);
	}
}

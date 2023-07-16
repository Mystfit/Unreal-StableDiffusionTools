#include "ImagePipelineStageCustomization.h"
#include "StableDiffusionGenerationOptions.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "DetailWidgetRow.h"
#include "IPropertyUtilities.h"
#include "LayerProcessorBase.h"

TSharedRef<IDetailCustomization> FImagePipelineStageCustomization::MakeInstance()
{
    return MakeShareable(new FImagePipelineStageCustomization);
}

void FImagePipelineStageCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	/*UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Models")
		UStableDiffusionModelAsset* Model;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Models")
		UStableDiffusionPipelineAsset* Pipeline;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Models")
		UStableDiffusionLORAAsset* LORAAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Models")
		UStableDiffusionTextualInversionAsset* TextualInversionAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Layers")
		TArray<FLayerProcessorContext> Layers;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stage Output")
		TEnumAsByte<EPipelineOutputType> OutputType;*/

	TSharedRef<IPropertyHandle> ModelHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UImagePipelineStageAsset, Model));
	DetailLayout.AddPropertyToCategory(ModelHandle);

	TSharedRef<IPropertyHandle> PipelineHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UImagePipelineStageAsset, Pipeline));
	DetailLayout.AddPropertyToCategory(PipelineHandle);

	TSharedRef<IPropertyHandle> LORAHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UImagePipelineStageAsset, LORAAsset));
	DetailLayout.AddPropertyToCategory(LORAHandle);

	TSharedRef<IPropertyHandle> TextualInversionHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UImagePipelineStageAsset, TextualInversionAsset));
	DetailLayout.AddPropertyToCategory(TextualInversionHandle);

	TSharedRef<IPropertyHandle> LayersHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UImagePipelineStageAsset, Layers));
	DetailLayout.AddPropertyToCategory(LayersHandle);

	TSharedRef<IPropertyHandle> OverrideOptionsHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UImagePipelineStageAsset, OverrideInputOptions));
	DetailLayout.AddPropertyToCategory(OverrideOptionsHandle);

	//TSharedRef<IPropertyHandle> OutputTypeHandle = DetailLayout.GetProperty(GET_MEMBER_NAME_CHECKED(UImagePipelineStageAsset, OutputType));
	//DetailLayout.AddPropertyToCategory(OutputTypeHandle);
	//DetailLayout.AddCustomRowToCategory(OutputTypeHandle, FText::FromString("Layer type"));
	//	.NameContent()
	//	[
	//		SNew(STextBlock)
	//		.Font(IDetailLayoutBuilder::GetDetailFont())
	//	.Text(LOCTEXT("ButtonCategory", "Rebuild from Raw Config"))
	//	]
	//.ValueContent()
	//	[
	//		SNew(SButton)
	//		.Text(LOCTEXT("ButtonName", "Reload and Rebuild"))
	//	.HAlign(HAlign_Center)
	//	.ToolTipText(LOCTEXT("ButtonToolTip", "Reload the current OCIO config file and rebuild shaders."))
	//	.OnClicked_Lambda([Objects]()
	//		{
	//			if (UOpenColorIOConfiguration* OpenColorIOConfig = Cast<UOpenColorIOConfiguration>(Objects[0].Get()))
	//			{
	//				OpenColorIOConfig->ReloadExistingColorspaces();
	//			}
	//			return FReply::Handled();
	//		})
	//	];
}

//void FImagePipelineStageCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
//{
//    //StructPropertyHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FLayerDataCustomization::OnValueChanged));
//
//    HeaderRow
//        .NameContent()
//        [
//            StructPropertyHandle->CreatePropertyNameWidget()
//        ]
//    .ValueContent()
//        [
//            StructPropertyHandle->CreatePropertyValueWidget()
//        ];
//
//}
//
//void FImagePipelineStageCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
//{
//    //PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();
//
//    uint32 NumChildren;
//    StructPropertyHandle->GetNumChildren(NumChildren);
//
//    for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
//    {
//        TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
//        const FName PropertyName = ChildHandle->GetProperty()->GetFName();
//
//        StructBuilder.AddProperty(ChildHandle);
//    }
//}

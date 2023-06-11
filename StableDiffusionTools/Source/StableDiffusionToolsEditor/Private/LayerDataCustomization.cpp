#include "LayerDataCustomization.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

TSharedRef<IPropertyTypeCustomization> FLayerDataCustomization::MakeInstance()
{
    return MakeShareable(new FLayerDataCustomization);
}

void FLayerDataCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	StructPropertyHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FLayerDataCustomization::OnValueChanged));

    HeaderRow
        .NameContent()
        [
            StructPropertyHandle->CreatePropertyNameWidget()
        ]
        .ValueContent()
        [
            StructPropertyHandle->CreatePropertyValueWidget()
        ];

}

void FLayerDataCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	PropertyUtilities = CustomizationUtils.GetPropertyUtilities();

	uint32 NumChildren;
	PropertyHandle->GetNumChildren(NumChildren);

	for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
	{
		TSharedRef<IPropertyHandle> ChildHandle = PropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		//StructBuilder.AddProperty(ChildHandle);

		ChildBuilder.AddProperty(ChildHandle);
		/*
		if (PropertyName.IsEqual(FName("ProcessorOptions"), ENameCase::CaseSensitive))
		{
			UObject* OptVal = nullptr;
			ChildHandle->GetValue(OptVal);
			if (IsValid(OptVal)) {			
				ChildBuilder.AddCustomRow(FText::FromString("Layer options"))
				[
					ChildHandle->CreatePropertyValueWidget()
				];
			}
		}
		else {
			ChildBuilder.AddProperty(ChildHandle);
		}
		*/
	}
}

#if 0
void FLayerDataCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

    uint32 NumChildren;
    StructPropertyHandle->GetNumChildren(NumChildren);

    for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
    {
		TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
		const FName PropertyName = ChildHandle->GetProperty()->GetFName();

		//StructBuilder.AddProperty(ChildHandle);

		
		if (PropertyName.IsEqual(FName("ProcessorOptions"), ENameCase::CaseSensitive))
		{
			UObject* OptVal = nullptr;
			ChildHandle->GetValue(OptVal);
			if (IsValid(OptVal)) {
				TSharedRef<SWidget> ChildWidget = SNew(SPropertyEditor, ChildHandle.ToSharedRef());

				/*
				//IDetailGroup& OptionsGroup = StructBuilder.AddGroup("Layer Options", FText::FromString("Layer Options"));
				TArray<UObject*> ObjArr;
				ObjArr.Add(OptVal);
				StructBuilder.AddExternalObjects(ObjArr);
				
				//for (TFieldIterator<FProperty> PropIt(OptVal->GetClass()); PropIt; ++PropIt)
				//{
				//	auto Property = *PropIt;
				//	auto PropHandle = ChildHandle->GetChildHandle(Property->GetFName());
				//	if (PropHandle) {
				//		TSharedRef<IPropertyHandle> OptHandle = PropHandle.ToSharedRef();
				//		StructBuilder.AddProperty(OptHandle);
				//		//if (OptHandle->GetDefaultCategoryName() == FName("Layer options")) {
				//			//OptionsGroup.AddPropertyRow(OptHandle);
				//		//}
				//	}
				//}
				
			}
			*/
		}
		else {
			StructBuilder.AddProperty(ChildHandle);
		}
		
    }
}
#endif

void FLayerDataCustomization::OnValueChanged()
{
	PropertyUtilities->ForceRefresh();
}

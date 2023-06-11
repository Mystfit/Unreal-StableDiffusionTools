#include "LayerProcessorOptionsCustomization.h"
#include "DetailWidgetRow.h"
#include "IDetailGroup.h"
#include "IDetailChildrenBuilder.h"
#include "IPropertyUtilities.h"

TSharedRef<IPropertyTypeCustomization> FLayerProcessorOptionsCustomization::MakeInstance()
{
    return MakeShareable(new FLayerProcessorOptionsCustomization);
}

void FLayerProcessorOptionsCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    //StructPropertyHandle->SetOnChildPropertyValueChanged(FSimpleDelegate::CreateSP(SharedThis(this), &FLayerDataCustomization::OnValueChanged));

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

void FLayerProcessorOptionsCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& StructBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
    //PropertyUtilities = StructCustomizationUtils.GetPropertyUtilities();

    uint32 NumChildren;
    StructPropertyHandle->GetNumChildren(NumChildren);

    for (uint32 ChildIndex = 0; ChildIndex < NumChildren; ++ChildIndex)
    {
        TSharedRef<IPropertyHandle> ChildHandle = StructPropertyHandle->GetChildHandle(ChildIndex).ToSharedRef();
        const FName PropertyName = ChildHandle->GetProperty()->GetFName();

        StructBuilder.AddProperty(ChildHandle);
    }
}

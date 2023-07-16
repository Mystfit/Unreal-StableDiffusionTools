#pragma once

#include "IDetailCustomization.h"


class FImagePipelineStageCustomization : public IDetailCustomization
{
public:
	/** @return A new instance of this class */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IPropertyTypeCustomization interface begin */
	//virtual void CustomizeHeader(TSharedRef<IPropertyHandle> StructPropertyHandle, class FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	//virtual void CustomizeChildren(TSharedRef<IPropertyHandle> StructPropertyHandle, class IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	/** IPropertyTypeCustomization interface end */

private:
	//void OnValueChanged();

	//TSharedPtr<IPropertyUtilities> PropertyUtilities;
};
// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "AssetTypeCategories.h"
#include "Modules/ModuleManager.h"

class IAssetTypeActions;
class FSlateStyleSet;
struct FGraphPanelNodeFactory;

class FAITCSEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterStyle();
	void UnregisterStyle();
	void RegisterGraphNodeFactory();
	void UnregisterGraphNodeFactory();

	TSharedPtr<IAssetTypeActions> TacticalGraphAssetTypeActions;
	TSharedPtr<IAssetTypeActions> TacticalGroupDataAssetTypeActions;
	TSharedPtr<FSlateStyleSet> StyleSet;
	TSharedPtr<FGraphPanelNodeFactory> GraphNodeFactory;
	EAssetTypeCategories::Type AITCSAssetCategory;
};

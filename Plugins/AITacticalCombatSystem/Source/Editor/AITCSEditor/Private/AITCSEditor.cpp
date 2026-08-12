// Pavel Gornostaev <https://github.com/Pavreally>

#include "AITCSEditor.h"

#include "AssetToolsModule.h"
#include "AssetTools/AITCSTacticalGroupDataAssetTypeActions.h"
#include "AssetTools/AITCSTacticalGraphAssetTypeActions.h"
#include "Brushes/SlateImageBrush.h"
#include "EdGraphUtilities.h"
#include "Graph/AITCSEditorGraphNodeFactory.h"
#include "IAssetTools.h"
#include "Interfaces/IPluginManager.h"
#include "Modules/ModuleManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "PropertyEditorModule.h"
#include "Details/AITCSTacticalObjectCustomization.h"

#define LOCTEXT_NAMESPACE "FAITCSEditorModule"

namespace AITCSEditorStyle
{
	static const FName StyleSetName(TEXT("AITCSEditorStyle"));
}

void FAITCSEditorModule::StartupModule()
{
	RegisterStyle();
	RegisterGraphNodeFactory();

	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	AITCSAssetCategory = AssetTools.RegisterAdvancedAssetCategory(TEXT("AITCS"), LOCTEXT("AITCSAssetCategory", "AITCS"));

	TacticalGraphAssetTypeActions = MakeShared<FAITCSTacticalGraphAssetTypeActions>(AITCSAssetCategory);
	AssetTools.RegisterAssetTypeActions(TacticalGraphAssetTypeActions.ToSharedRef());

	TacticalGroupDataAssetTypeActions = MakeShared<FAITCSTacticalGroupDataAssetTypeActions>(AITCSAssetCategory);
	AssetTools.RegisterAssetTypeActions(TacticalGroupDataAssetTypeActions.ToSharedRef());

	// Register custom property type layout for FAITCSTacticalObject to expose ExternalExtensions nicely
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.RegisterCustomPropertyTypeLayout(TEXT("FAITCSTacticalObject"), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FAITCSTacticalObjectCustomization::MakeInstance));
	}
}

void FAITCSEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (TacticalGraphAssetTypeActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(TacticalGraphAssetTypeActions.ToSharedRef());
		}

		if (TacticalGroupDataAssetTypeActions.IsValid())
		{
			AssetTools.UnregisterAssetTypeActions(TacticalGroupDataAssetTypeActions.ToSharedRef());
		}
	}

	TacticalGraphAssetTypeActions.Reset();
	TacticalGroupDataAssetTypeActions.Reset();
	UnregisterGraphNodeFactory();
	UnregisterStyle();

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(TEXT("FAITCSTacticalObject"));
	}
}

void FAITCSEditorModule::RegisterStyle()
{
	if (StyleSet.IsValid())
	{
		return;
	}

	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AITacticalCombatSystem"));
	if (!Plugin.IsValid())
	{
		return;
	}

	StyleSet = MakeShared<FSlateStyleSet>(AITCSEditorStyle::StyleSetName);
	StyleSet->SetContentRoot(Plugin->GetBaseDir() / TEXT("Resources"));

	StyleSet->Set(
		TEXT("ClassIcon.AITCSTacticalGraphAsset"),
		new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("Assets/Icon_TacticalGraph_20"), TEXT(".svg")), FVector2D(16.0f, 16.0f))
	);

	StyleSet->Set(
		TEXT("ClassThumbnail.AITCSTacticalGraphAsset"),
		new FSlateImageBrush(StyleSet->RootToContentDir(TEXT("Assets/Thumbnail_TacticalGraph_128"), TEXT(".png")), FVector2D(64.0f, 64.0f))
	);

	// Toolbar icons
	StyleSet->Set(
		TEXT("AITCSEditor.Toolbar.AddObject"),
		new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("Toolbar/Add"), TEXT(".svg")), FVector2D(16.0f, 16.0f))
	);

	StyleSet->Set(
		TEXT("AITCSEditor.Toolbar.Duplicate"),
		new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("Toolbar/Duplicate"), TEXT(".svg")), FVector2D(16.0f, 16.0f))
	);

	StyleSet->Set(
		TEXT("AITCSEditor.Toolbar.Validate"),
		new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("Toolbar/Validate"), TEXT(".svg")), FVector2D(16.0f, 16.0f))
	);

	StyleSet->Set(
		TEXT("AITCSEditor.Toolbar.DeleteObject"),
		new FSlateVectorImageBrush(StyleSet->RootToContentDir(TEXT("Toolbar/Del"), TEXT(".svg")), FVector2D(16.0f, 16.0f))
	);

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FAITCSEditorModule::UnregisterStyle()
{
	if (!StyleSet.IsValid())
	{
		return;
	}

	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
	StyleSet.Reset();
}

void FAITCSEditorModule::RegisterGraphNodeFactory()
{
	if (!GraphNodeFactory.IsValid())
	{
		GraphNodeFactory = MakeShared<FAITCSEditorGraphNodeFactory>();
		FEdGraphUtilities::RegisterVisualNodeFactory(GraphNodeFactory);
	}
}

void FAITCSEditorModule::UnregisterGraphNodeFactory()
{
	if (GraphNodeFactory.IsValid())
	{
		FEdGraphUtilities::UnregisterVisualNodeFactory(GraphNodeFactory);
		GraphNodeFactory.Reset();
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FAITCSEditorModule, AITCSEditor)

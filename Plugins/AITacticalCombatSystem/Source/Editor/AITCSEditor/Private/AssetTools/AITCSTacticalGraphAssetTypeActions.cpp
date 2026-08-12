// Pavel Gornostaev <https://github.com/Pavreally>

#include "AssetTools/AITCSTacticalGraphAssetTypeActions.h"

#include "Editor/AITCSTacticalGraphAssetEditor.h"
#include "Graph/AITCSTacticalGraphAsset.h"

#define LOCTEXT_NAMESPACE "AITCSTacticalGraphAssetTypeActions"

FAITCSTacticalGraphAssetTypeActions::FAITCSTacticalGraphAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

FText FAITCSTacticalGraphAssetTypeActions::GetName() const
{
	return LOCTEXT("AITCSTacticalGraphAssetName", "AITCS Tactical Graph");
}

FColor FAITCSTacticalGraphAssetTypeActions::GetTypeColor() const
{
	return FColor(35, 140, 220);
}

UClass* FAITCSTacticalGraphAssetTypeActions::GetSupportedClass() const
{
	return UAITCSTacticalGraphAsset::StaticClass();
}

uint32 FAITCSTacticalGraphAssetTypeActions::GetCategories()
{
	return AssetCategory;
}

void FAITCSTacticalGraphAssetTypeActions::OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	const EToolkitMode::Type Mode = EditWithinLevelEditor.IsValid() ? EToolkitMode::WorldCentric : EToolkitMode::Standalone;

	for (UObject* Object : InObjects)
	{
		if (UAITCSTacticalGraphAsset* GraphAsset = Cast<UAITCSTacticalGraphAsset>(Object))
		{
			TSharedRef<FAITCSTacticalGraphAssetEditor> Editor = MakeShared<FAITCSTacticalGraphAssetEditor>();
			Editor->InitTacticalGraphAssetEditor(Mode, EditWithinLevelEditor, GraphAsset);
		}
	}
}

#undef LOCTEXT_NAMESPACE

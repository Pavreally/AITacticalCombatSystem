// Pavel Gornostaev <https://github.com/Pavreally>

#include "AssetTools/AITCSTacticalGroupDataAssetTypeActions.h"

#include "Data/AITCSTacticalGroupDataAsset.h"

#define LOCTEXT_NAMESPACE "AITCSTacticalGroupDataAssetTypeActions"

FAITCSTacticalGroupDataAssetTypeActions::FAITCSTacticalGroupDataAssetTypeActions(EAssetTypeCategories::Type InAssetCategory)
	: AssetCategory(InAssetCategory)
{
}

FText FAITCSTacticalGroupDataAssetTypeActions::GetName() const
{
	return LOCTEXT("AITCSTacticalGroupDataAssetName", "AITCS Tactical Group");
}

FColor FAITCSTacticalGroupDataAssetTypeActions::GetTypeColor() const
{
	return FColor(70, 175, 105);
}

UClass* FAITCSTacticalGroupDataAssetTypeActions::GetSupportedClass() const
{
	return UAITCSTacticalGroupDataAsset::StaticClass();
}

uint32 FAITCSTacticalGroupDataAssetTypeActions::GetCategories()
{
	return AssetCategory;
}

#undef LOCTEXT_NAMESPACE

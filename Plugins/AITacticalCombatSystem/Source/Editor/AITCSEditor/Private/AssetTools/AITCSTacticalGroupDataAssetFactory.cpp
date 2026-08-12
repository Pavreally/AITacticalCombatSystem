// Pavel Gornostaev <https://github.com/Pavreally>

#include "AssetTools/AITCSTacticalGroupDataAssetFactory.h"

#include "Data/AITCSTacticalGroupDataAsset.h"

UAITCSTacticalGroupDataAssetFactory::UAITCSTacticalGroupDataAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UAITCSTacticalGroupDataAsset::StaticClass();
}

UObject* UAITCSTacticalGroupDataAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	(void)Context;
	(void)Warn;

	UAITCSTacticalGroupDataAsset* GroupAsset = NewObject<UAITCSTacticalGroupDataAsset>(InParent, Class, Name, Flags | RF_Transactional);
	GroupAsset->Group.DisplayName = FText::FromName(Name);
	GroupAsset->Group.ExpectedSize = 5;
	GroupAsset->Group.MaxMembers = 5;
	return GroupAsset;
}

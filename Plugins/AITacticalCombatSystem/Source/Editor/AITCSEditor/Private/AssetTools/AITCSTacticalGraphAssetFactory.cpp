// Pavel Gornostaev <https://github.com/Pavreally>

#include "AssetTools/AITCSTacticalGraphAssetFactory.h"

#include "Graph/AITCSTacticalGraphAsset.h"

UAITCSTacticalGraphAssetFactory::UAITCSTacticalGraphAssetFactory()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UAITCSTacticalGraphAsset::StaticClass();
}

UObject* UAITCSTacticalGraphAssetFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
	(void)Context;
	(void)Warn;

	UAITCSTacticalGraphAsset* GraphAsset = NewObject<UAITCSTacticalGraphAsset>(InParent, Class, Name, Flags | RF_Transactional);
	GraphAsset->EnsurePlayerObject();
	return GraphAsset;
}

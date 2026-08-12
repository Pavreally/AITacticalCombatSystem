// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "AITCSTacticalGroupDataAssetFactory.generated.h"

UCLASS()
class UAITCSTacticalGroupDataAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UAITCSTacticalGroupDataAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

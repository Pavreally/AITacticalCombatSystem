// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Factories/Factory.h"

#include "AITCSTacticalGraphAssetFactory.generated.h"

UCLASS()
class UAITCSTacticalGraphAssetFactory : public UFactory
{
	GENERATED_BODY()

public:
	UAITCSTacticalGraphAssetFactory();

	virtual UObject* FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn) override;
};

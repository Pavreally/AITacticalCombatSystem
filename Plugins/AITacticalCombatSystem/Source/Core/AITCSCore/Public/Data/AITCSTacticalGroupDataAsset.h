// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Data/AITCSCoreData.h"
#include "Engine/DataAsset.h"

#include "AITCSTacticalGroupDataAsset.generated.h"

/**
 * Data asset container for tactical group definitions used by AITCS.
 */
UCLASS(BlueprintType)
class AITCSCORE_API UAITCSTacticalGroupDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Tactical group definition used by the director and graph compiler. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Group")
	FAITCSTacticalGroup Group;
};

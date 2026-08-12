// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/AITCSCoreData.h"

#include "AITCSTacticalDecisionProvider.generated.h"

/**
 * Interface for objects that provide tactical order generation to the AITCS director.
 */
UINTERFACE(BlueprintType)
class AITCSCORE_API UAITCSTacticalDecisionProvider : public UInterface
{
	GENERATED_BODY()
};

/** Implement this interface to generate tactical orders in response to a context snapshot. */
class AITCSCORE_API IAITCSTacticalDecisionProvider
{
	GENERATED_BODY()

public:
	/**
	 * Build tactical orders based on the current tactical context snapshot.
	 * @param Context Current runtime tactical context.
	 * @param OutOrders Output order list to dispatch.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AITCS|Decision Provider")
	void BuildTacticalOrders(const FAITCSTacticalContextSnapshot& Context, TArray<FAITCSTacticalOrder>& OutOrders);
};

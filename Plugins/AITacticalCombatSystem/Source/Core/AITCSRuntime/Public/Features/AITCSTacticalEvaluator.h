// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Features/AITCSTacticalFeatureTypes.h"

#include "AITCSTacticalEvaluator.generated.h"

/** Interface that marks an object as an AITCS tactical evaluator. */
UINTERFACE(BlueprintType)
class AITCSRUNTIME_API UAITCSTacticalEvaluator : public UInterface
{
	GENERATED_BODY()
};

/** Implement this interface to participate in AITCS evaluation passes and generate tactical proposals. */
class AITCSRUNTIME_API IAITCSTacticalEvaluator
{
	GENERATED_BODY()

public:
	/** Return whether this evaluator is currently enabled. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AITCS|Evaluator")
	bool IsTacticalEvaluatorEnabled() const;
	virtual bool IsTacticalEvaluatorEnabled_Implementation() const;

	/** Return the display name of this evaluator. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AITCS|Evaluator")
	FName GetTacticalEvaluatorName() const;
	virtual FName GetTacticalEvaluatorName_Implementation() const;

	/** Evaluate the tactical context and output order proposals. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AITCS|Evaluator")
	void EvaluateTacticalState(const FAITCSTacticalEvaluationContext& Context, TArray<FAITCSTacticalOrderProposal>& OutProposals);
	virtual void EvaluateTacticalState_Implementation(const FAITCSTacticalEvaluationContext& Context, TArray<FAITCSTacticalOrderProposal>& OutProposals);
};

// Pavel Gornostaev <https://github.com/Pavreally>

#include "Features/AITCSTacticalEvaluator.h"

bool IAITCSTacticalEvaluator::IsTacticalEvaluatorEnabled_Implementation() const
{
	return true;
}

FName IAITCSTacticalEvaluator::GetTacticalEvaluatorName_Implementation() const
{
	return NAME_None;
}

void IAITCSTacticalEvaluator::EvaluateTacticalState_Implementation(const FAITCSTacticalEvaluationContext& Context, TArray<FAITCSTacticalOrderProposal>& OutProposals)
{
	(void)Context;
	OutProposals.Reset();
}

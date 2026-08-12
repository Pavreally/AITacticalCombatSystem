// Pavel Gornostaev <https://github.com/Pavreally>

#include "Features/AITCSTacticalFeatureTypes.h"

FAITCSTacticalEvaluationContext::FAITCSTacticalEvaluationContext()
{
}

const FAITCSCompiledTacticalGraph* FAITCSTacticalEvaluationContext::GetCompiledGraph() const
{
	return CompiledGraph;
}

bool FAITCSTacticalEvaluationContext::HasCompiledGraph() const
{
	return CompiledGraph && CompiledGraph->IsCompiled();
}

FAITCSTacticalOrderProposal::FAITCSTacticalOrderProposal()
	: ProposalId(FGuid::NewGuid())
{
}

bool FAITCSTacticalOrderProposal::IsDispatchable() const
{
	return Order.OrderType != EAITCSOrderType::None;
}

float FAITCSTacticalOrderProposal::GetWeightedScore(float EvaluatorWeight) const
{
	const float SafeConfidence = FMath::Clamp(Confidence, 0.0f, 1.0f);
	const float SafeWeight = FMath::Max(0.0f, EvaluatorWeight);
	return (Priority + Score * SafeConfidence) * SafeWeight;
}

void FAITCSTacticalOrderProposal::PrepareForDispatch()
{
	if (!ProposalId.IsValid())
	{
		ProposalId = FGuid::NewGuid();
	}

	if (!Order.OrderId.IsValid())
	{
		Order.OrderId = FGuid::NewGuid();
	}

	if (Order.Priority <= 0.0f && Priority > 0.0f)
	{
		Order.Priority = Priority;
	}

	Order.State = EAITCSOrderState::Pending;
}

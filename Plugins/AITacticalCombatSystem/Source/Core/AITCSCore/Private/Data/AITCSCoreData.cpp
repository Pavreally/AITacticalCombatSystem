// Pavel Gornostaev <https://github.com/Pavreally>

#include "Data/AITCSCoreData.h"

FAITCSTacticalObject::FAITCSTacticalObject()
	: ObjectId(FGuid::NewGuid())
{
}

FAITCSTacticalLink::FAITCSTacticalLink()
	: LinkId(FGuid::NewGuid())
{
}

bool FAITCSTacticalLink::Connects(const FGuid& InSourceObjectId, const FGuid& InTargetObjectId) const
{
	return SourceObjectId == InSourceObjectId && TargetObjectId == InTargetObjectId;
}

bool FAITCSTacticalAgentHandle::IsValid() const
{
	return AgentId.IsValid();
}

bool FAITCSTacticalRuntimeGroupState::HasCapacityLimit() const
{
	return MaxMembers > 0;
}

bool FAITCSTacticalRuntimeGroupState::HasFreeSlot() const
{
	return !HasCapacityLimit() || MemberAgentIds.Num() < MaxMembers;
}

int32 FAITCSTacticalRuntimeGroupState::GetFreeSlotCount() const
{
	if (!HasCapacityLimit())
	{
		return MAX_int32;
	}

	return FMath::Max(0, MaxMembers - MemberAgentIds.Num());
}

FAITCSTacticalOrder::FAITCSTacticalOrder()
	: OrderId(FGuid::NewGuid())
{
}

bool FAITCSTacticalOrder::HasExplicitTarget() const
{
	return TargetAgentId.IsValid() || TargetObjectId.IsValid() || TargetGroupId.IsValid() || !TargetTags.IsEmpty();
}

// Pavel Gornostaev <https://github.com/Pavreally>

#include "Actors/AITCSDirectorActor.h"

#include "Director/AITCSDirectorSubsystem.h"
#include "Engine/World.h"
#include "Graph/AITCSTacticalGraphAsset.h"
#include "TimerManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogAITCSDirectorActor, Log, All);

AAITCSDirectorActor::AAITCSDirectorActor()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AAITCSDirectorActor::BeginPlay()
{
	Super::BeginPlay();

	if (bActivateOnBeginPlay)
	{
		ActivateTacticalGraph();
	}
}

bool AAITCSDirectorActor::ActivateTacticalGraph()
{
	UAITCSDirectorSubsystem* Director = GetDirectorSubsystem();
	if (!Director || !TacticalGraphAsset)
	{
		return false;
	}

	const bool bActivated = Director->SetActiveTacticalGraph(TacticalGraphAsset);
	if (!bActivated)
	{
		return false;
	}

	if (bResolveRegisteredUnitsAfterActivation)
	{
		ResolveRegisteredUnits();
	}

	if (bLogRuntimeGroupSummaryOnBeginPlay && !bRunDeferredResolveAfterBeginPlay)
	{
		LogRuntimeGroupSummary();
	}

	if (bRunDeferredResolveAfterBeginPlay)
	{
		ScheduleDeferredResolvePass();
	}

	return true;
}

int32 AAITCSDirectorActor::ResolveRegisteredUnits()
{
	UAITCSDirectorSubsystem* Director = GetDirectorSubsystem();
	if (!Director)
	{
		return 0;
	}

	return Director->ResolveRegisteredAgentAssignments(bOverwriteExistingUnitAssignments);
}

void AAITCSDirectorActor::LogRuntimeGroupSummary() const
{
	const UAITCSDirectorSubsystem* Director = GetDirectorSubsystem();
	if (!Director)
	{
		UE_LOG(LogAITCSDirectorActor, Warning, TEXT("Cannot log AITCS runtime groups: director subsystem is not available."));
		return;
	}

	const TArray<FAITCSTacticalRuntimeGroupState> GroupStates = Director->GetRuntimeGroupStates();
	UE_LOG(LogAITCSDirectorActor, Log, TEXT("AITCS runtime group summary: %d group slot(s)."), GroupStates.Num());

	for (const FAITCSTacticalRuntimeGroupState& GroupState : GroupStates)
	{
		const FString GroupName = GroupState.GroupDisplayName.IsEmpty() ? GroupState.GroupId.ToString() : GroupState.GroupDisplayName.ToString();
		const FString MaxMembersText = GroupState.HasCapacityLimit() ? FString::FromInt(GroupState.MaxMembers) : FString(TEXT("Unlimited"));
		const FAITCSCompiledTacticalObject* CompiledObject = Director->GetCompiledTacticalGraph().FindObject(GroupState.TacticalObjectId);
		UE_LOG(
			LogAITCSDirectorActor,
			Log,
			TEXT("  Object=[%s] Id=[%s] Group=[%s] Members=[%d/%s] Expected=[%d] FreePool=[%s] Desired=[%.1f] Attack=[%.1f] Simple=[%s]"),
			*GroupState.TacticalObjectName.ToString(),
			*GroupState.TacticalObjectId.ToString(EGuidFormats::Short),
			*GroupName,
			GroupState.MemberAgentIds.Num(),
			*MaxMembersText,
			GroupState.ExpectedSize,
			GroupState.bIsSimpleObjectFreePool ? TEXT("true") : TEXT("false"),
			CompiledObject ? CompiledObject->DesiredPlayerDistance : 0.0f,
			CompiledObject ? CompiledObject->AttackTargetDistance : 0.0f,
			CompiledObject && CompiledObject->bIsSimpleObject ? TEXT("true") : TEXT("false"));
	}
}

void AAITCSDirectorActor::ScheduleDeferredResolvePass()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (World->GetTimerManager().IsTimerActive(DeferredResolveTimerHandle))
	{
		return;
	}

	if (DeferredResolveDelay <= 0.0f)
	{
		DeferredResolveTimerHandle = World->GetTimerManager().SetTimerForNextTick(this, &AAITCSDirectorActor::RunDeferredResolvePass);
		return;
	}

	World->GetTimerManager().SetTimer(DeferredResolveTimerHandle, this, &AAITCSDirectorActor::RunDeferredResolvePass, DeferredResolveDelay, false);
}

void AAITCSDirectorActor::RunDeferredResolvePass()
{
	if (!GetDirectorSubsystem() || !TacticalGraphAsset)
	{
		return;
	}

	UAITCSDirectorSubsystem* Director = GetDirectorSubsystem();
	if (!Director->GetActiveTacticalGraph())
	{
		Director->SetActiveTacticalGraph(TacticalGraphAsset);
	}

	Director->ResolveRegisteredAgentAssignments(bOverwriteExistingUnitAssignments);

	if (bLogRuntimeGroupSummaryAfterDeferredResolve)
	{
		LogRuntimeGroupSummary();
	}
}

UAITCSDirectorSubsystem* AAITCSDirectorActor::GetDirectorSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UAITCSDirectorSubsystem>() : nullptr;
}

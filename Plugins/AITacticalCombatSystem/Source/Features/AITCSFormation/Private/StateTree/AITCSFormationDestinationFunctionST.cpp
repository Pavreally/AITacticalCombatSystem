// Pavel Gornostaev <https://github.com/Pavreally>

#include "StateTree/AITCSFormationDestinationFunctionST.h"
#include "Engine/World.h"

#include "Components/AITCSTacticalUnitComponent.h"
#include "Director/AITCSDirectorSubsystem.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "StateTreeExecutionContext.h"
#include "StateTreePropertyBindings.h"

#define LOCTEXT_NAMESPACE "AITCSFormationDestinationFunctionST"

namespace
{
	AActor *ResolveAITCSAgentActor(FStateTreeExecutionContext &Context, const FAITCSFormationDestinationInstanceData &InstanceData)
	{
		if (InstanceData.AgentActor)
		{
			return InstanceData.AgentActor;
		}

		UObject *OwnerObject = Context.GetOwner();
		AActor *OwnerActor = Cast<AActor>(OwnerObject);
		if (!OwnerActor)
		{
			if (const UActorComponent *OwnerComponent = Cast<UActorComponent>(OwnerObject))
			{
				OwnerActor = OwnerComponent->GetOwner();
			}
		}

		if (AController *Controller = Cast<AController>(OwnerActor))
		{
			if (APawn *Pawn = Controller->GetPawn())
			{
				return Pawn;
			}
		}

		return OwnerActor;
	}

	UAITCSTacticalUnitComponent *ResolveAITCSUnitComponent(FStateTreeExecutionContext &Context, const FAITCSFormationDestinationInstanceData &InstanceData)
	{
		if (InstanceData.TacticalUnitComponent)
		{
			return InstanceData.TacticalUnitComponent;
		}

		// StateTree may be owned either by the pawn or by its AIController. Check
		// both ends of that pair because the AITCS component can be attached to
		// either actor.
		AActor *OwnerActor = Cast<AActor>(Context.GetOwner());
		if (!OwnerActor)
		{
			if (const UActorComponent *OwnerComponent = Cast<UActorComponent>(Context.GetOwner()))
			{
				OwnerActor = OwnerComponent->GetOwner();
			}
		}

		if (OwnerActor)
		{
			if (UAITCSTacticalUnitComponent *Component = OwnerActor->FindComponentByClass<UAITCSTacticalUnitComponent>())
			{
				return Component;
			}

			if (const AController *OwnerController = Cast<AController>(OwnerActor))
			{
				if (APawn *ControlledPawn = OwnerController->GetPawn())
				{
					if (UAITCSTacticalUnitComponent *Component = ControlledPawn->FindComponentByClass<UAITCSTacticalUnitComponent>())
					{
						return Component;
					}
				}
			}
			else if (const APawn *OwnerPawn = Cast<APawn>(OwnerActor))
			{
				if (AController *PawnController = OwnerPawn->GetController())
				{
					if (UAITCSTacticalUnitComponent *Component = PawnController->FindComponentByClass<UAITCSTacticalUnitComponent>())
					{
						return Component;
					}
				}
			}
		}

		AActor *AgentActor = ResolveAITCSAgentActor(Context, InstanceData);
		if (!AgentActor)
		{
			return nullptr;
		}

		if (UAITCSTacticalUnitComponent *Component = AgentActor->FindComponentByClass<UAITCSTacticalUnitComponent>())
		{
			return Component;
		}

		if (const APawn *AgentPawn = Cast<APawn>(AgentActor))
		{
			if (AController *PawnController = AgentPawn->GetController())
			{
				return PawnController->FindComponentByClass<UAITCSTacticalUnitComponent>();
			}
		}

		return nullptr;
	}
}

void FAITCSFormationDestinationFunctionST::Execute(FStateTreeExecutionContext &Context) const
{
	FInstanceDataType &InstanceData = Context.GetInstanceData(*this);
	InstanceData.Destination = FVector::ZeroVector;
	InstanceData.FollowDistance = FMath::Max(0.0f, InstanceData.DefaultFollowDistance);
	InstanceData.AcceptanceRadius = FMath::Max(0.0f, InstanceData.DefaultAcceptanceRadius);
	InstanceData.AttackTargetDistance = 0.0f;
	InstanceData.RoleTag = FGameplayTag();
	InstanceData.bIsSimpleObject = false;
	InstanceData.bHasResolvedTacticalObject = false;
	InstanceData.ResolvedTacticalObjectId = FGuid();
	InstanceData.ResolvedTacticalObjectName = NAME_None;
	InstanceData.bFormationControlReleased = false;

	UWorld *World = Context.GetWorld();
	UAITCSDirectorSubsystem *Director = World ? World->GetSubsystem<UAITCSDirectorSubsystem>() : nullptr;
	UAITCSTacticalUnitComponent *UnitComponent = ResolveAITCSUnitComponent(Context, InstanceData);
	AActor *AgentActor = ResolveAITCSAgentActor(Context, InstanceData);

	// Resolve object data independently from movement. A StateTree may need the
	// authored role/distances even when there is no target or formation destination.
	if (Director)
	{
		// Prefer the component explicitly resolved for this StateTree instance.
		// This avoids trusting a stale/shared AgentActor binding. Use the Director's
		// actor-to-registration mapping only when the component path is unavailable.
		FAITCSResolvedTacticalObjectData ObjectData;
		if (UnitComponent)
		{
			ObjectData = Director->ResolveTacticalObjectDataForUnit(UnitComponent, InstanceData.DefaultFollowDistance);
		}
		if (!ObjectData.bIsValid)
		{
			ObjectData = Director->ResolveTacticalObjectDataForActor(AgentActor, InstanceData.DefaultFollowDistance);
		}

		if (ObjectData.bIsValid)
		{
			InstanceData.bHasResolvedTacticalObject = true;
			InstanceData.ResolvedTacticalObjectId = ObjectData.TacticalObjectId;
			InstanceData.ResolvedTacticalObjectName = ObjectData.TacticalObjectName;
			InstanceData.FollowDistance = FMath::Max(0.0f, ObjectData.FollowDistance);
			InstanceData.AcceptanceRadius = (ObjectData.bIsSimpleObject && InstanceData.bPrioritizeDesiredPlayerDistance)
				? InstanceData.FollowDistance
				: FMath::Max(0.0f, InstanceData.DefaultAcceptanceRadius);
			InstanceData.AttackTargetDistance = FMath::Max(0.0f, ObjectData.AttackTargetDistance);
			InstanceData.RoleTag = ObjectData.RoleTag;
			InstanceData.bIsSimpleObject = ObjectData.bIsSimpleObject;
		}

		if (InstanceData.TargetActor)
		{
			FVector ResolvedDestination = FVector::ZeroVector;
			float ResolvedFollowDistance = FMath::Max(0.0f, InstanceData.DefaultFollowDistance);
			bool bUsesSimpleObjectSettings = false;
			const bool bResolved = Director->ResolveFormationMoveDestinationDetailed(
				UnitComponent,
				InstanceData.TargetActor,
				ResolvedDestination,
				ResolvedFollowDistance,
				bUsesSimpleObjectSettings,
				InstanceData.DefaultFollowDistance,
				InstanceData.MemberSpacing,
				InstanceData.bApplyMemberSlotOffset);

			if (bResolved)
			{
				InstanceData.Destination = ResolvedDestination;
				InstanceData.FollowDistance = FMath::Max(0.0f, ResolvedFollowDistance);
				InstanceData.AcceptanceRadius = (bUsesSimpleObjectSettings && InstanceData.bPrioritizeDesiredPlayerDistance)
					? InstanceData.FollowDistance
					: FMath::Max(0.0f, InstanceData.DefaultAcceptanceRadius);

				if (InstanceData.bReleaseFormationControlWhenWithinAttackDistance && UnitComponent && InstanceData.TargetActor)
				{
					const AActor* UnitOwner = UnitComponent->GetOwner();
					if (UnitOwner)
					{
						const float DistanceToTargetSquared = FVector::DistSquared2D(UnitOwner->GetActorLocation(), InstanceData.TargetActor->GetActorLocation());
						const float AttackDistanceSquared = FMath::Square(FMath::Max(0.0f, InstanceData.AttackTargetDistance));
						if (DistanceToTargetSquared <= AttackDistanceSquared)
						{
							InstanceData.bFormationControlReleased = true;
							InstanceData.Destination = InstanceData.TargetActor->GetActorLocation();
							InstanceData.FollowDistance = 0.0f;
							InstanceData.AcceptanceRadius = 0.0f;
						}
					}
				}
				return;
			}
		}
	}

	if (InstanceData.bFallbackToTargetLocation && InstanceData.TargetActor)
	{
		InstanceData.Destination = InstanceData.TargetActor->GetActorLocation();
	}
}

#if WITH_EDITOR
FText FAITCSFormationDestinationFunctionST::GetDescription(const FGuid &ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup &BindingLookup, EStateTreeNodeFormatting Formatting) const
{
	const FInstanceDataType *InstanceData = InstanceDataView.GetPtr<FInstanceDataType>();
	check(InstanceData);

	FText TargetValue = BindingLookup.GetBindingSourceDisplayName(FPropertyBindingPath(ID, GET_MEMBER_NAME_CHECKED(FInstanceDataType, TargetActor)), Formatting);
	if (TargetValue.IsEmpty() && InstanceData->TargetActor)
	{
		TargetValue = FText::FromString(InstanceData->TargetActor->GetName());
	}

	if (TargetValue.IsEmpty())
	{
		TargetValue = LOCTEXT("AITCSFormationDestinationTarget", "Target");
	}

	return FText::Format(LOCTEXT("AITCSFormationDestinationDescription", "AITCS Formation Destination: {0}"), TargetValue);
}
#endif

#undef LOCTEXT_NAMESPACE

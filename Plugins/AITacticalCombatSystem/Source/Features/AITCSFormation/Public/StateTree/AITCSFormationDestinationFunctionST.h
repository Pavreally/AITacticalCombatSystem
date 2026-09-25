// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StateTreePropertyFunctionBase.h"

#include "AITCSFormationDestinationFunctionST.generated.h"

class AActor;
class UAITCSTacticalUnitComponent;

/** How formation movement reacts while its target character is airborne. */
UENUM()
enum class EAITCSAirborneTargetBehavior : uint8
{
	FollowProjectedLocation UMETA(DisplayName = "Keep Following (Project to Navigation)"),
	WanderNearby UMETA(DisplayName = "Wander Nearby")
};

/** Instance data used by the AITCS formation destination StateTree function. */
USTRUCT()
struct AITCSFORMATION_API FAITCSFormationDestinationInstanceData
{
	GENERATED_BODY()

	/** The actor representing the moving agent. */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> AgentActor = nullptr;

	/** Optional override component that provides tactical assignment data. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Tactical Unit Component Override"))
	TObjectPtr<UAITCSTacticalUnitComponent> TacticalUnitComponent = nullptr;

	/** Target actor used when resolving formation destination. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<AActor> TargetActor = nullptr;

	/** Fallback follow distance when no object-specific distance is available. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Fallback Follow Distance", ClampMin = "0.0", UIMin = "0.0"))
	float DefaultFollowDistance = 800.0f;

	/** Fallback acceptance radius used in the absence of explicit formation settings. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Fallback Acceptance Radius", ClampMin = "0.0", UIMin = "0.0"))
	float DefaultAcceptanceRadius = 50.0f;

	/** Fallback spacing between formation members when no specific values are provided. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Fallback Member Spacing", ClampMin = "0.0", UIMin = "0.0"))
	float MemberSpacing = 175.0f;

	/** Whether to apply slot offsets for individual members in the formation. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bApplyMemberSlotOffset = true;

	/** If true, the formation will use the target actor's location when object assignment is unavailable. */
	UPROPERTY(EditAnywhere, Category = Parameter)
	bool bFallbackToTargetLocation = true;

	/** If true, simple object desired distance is treated as the acceptance radius. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Use Desired Distance As Simple Object Acceptance Radius"))
	bool bPrioritizeDesiredPlayerDistance = false;

	/** Release formation control when the unit is within attack distance. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Release Formation Control When Within Attack Distance"))
	bool bReleaseFormationControlWhenWithinAttackDistance = false;

	/** Movement response while Target Actor is a character in the air. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Airborne Target Behavior"))
	EAITCSAirborneTargetBehavior AirborneTargetBehavior = EAITCSAirborneTargetBehavior::FollowProjectedLocation;

	/** Maximum distance from the unit for a reachable wandering point while the target is airborne. */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (DisplayName = "Airborne Wander Radius", ClampMin = "0.0", UIMin = "0.0", EditCondition = "AirborneTargetBehavior == EAITCSAirborneTargetBehavior::WanderNearby"))
	float AirborneWanderRadius = 600.0f;

	/** Resolved follow distance output from the function. */
	UPROPERTY(EditAnywhere, Category = Output)
	float FollowDistance = 800.0f;

	/** Resolved acceptance radius output from the function. */
	UPROPERTY(EditAnywhere, Category = Output)
	float AcceptanceRadius = 50.0f;

	/** Resolved attack target distance output from the function. */
	UPROPERTY(EditAnywhere, Category = Output)
	float AttackTargetDistance = 0.0f;

	/** Role tag resolved for the current tactical object. */
	UPROPERTY(EditAnywhere, Category = Output)
	FGameplayTag RoleTag;

	/** Whether the current object was a simple object config. */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bIsSimpleObject = false;

	/** False means the unit has no valid AITCS object assignment at evaluation time. */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bHasResolvedTacticalObject = false;

	/** Identifier of the resolved tactical object that supplied the destination. */
	UPROPERTY(EditAnywhere, Category = Output)
	FGuid ResolvedTacticalObjectId;

	/** Name of the resolved tactical object that supplied the destination. */
	UPROPERTY(EditAnywhere, Category = Output)
	FName ResolvedTacticalObjectName = NAME_None;

	/** Switch to ordinary follow behavior when true. */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bFormationControlReleased = false;

	/** Calculated destination for the formation movement. */
	UPROPERTY(EditAnywhere, Category = Output)
	FVector Destination = FVector::ZeroVector;

	/** Runtime cache keeps wander movement stable for the duration of one jump. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> CachedAirborneTargetActor = nullptr;

	UPROPERTY(Transient)
	FVector CachedAirborneWanderDestination = FVector::ZeroVector;

	UPROPERTY(Transient)
	bool bHasCachedAirborneWanderDestination = false;
};

/** StateTree property function that resolves formation destination from AITCS tactical data. */
USTRUCT(meta = (DisplayName = "AITCS Formation Destination", Category = "AITCS"))
struct AITCSFORMATION_API FAITCSFormationDestinationFunctionST : public FStateTreePropertyFunctionCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FAITCSFormationDestinationInstanceData;

	virtual const UStruct *GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
	virtual void Execute(FStateTreeExecutionContext &Context) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid &ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup &BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
	virtual FName GetIconName() const override
	{
		return FName("StateTreeEditorStyle|Node.Movement");
	}
#endif
};

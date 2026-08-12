// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "GameFramework/Pawn.h"
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "Integration/AITCSExternalAIExtension.h"

#include "AITCSCoreData.generated.h"

class AAIController;
class AActor;
class APawn;
class UAITCSTacticalGroupDataAsset;

/** Shapes used to visualize tactical objects in the AITCS graph editor and runtime. */
UENUM(BlueprintType)
enum class EAITCSTacticalObjectShape : uint8
{
	Circle UMETA(DisplayName = "Circle"),
	Square UMETA(DisplayName = "Square"),
	Diamond UMETA(DisplayName = "Diamond"),
	Triangle UMETA(DisplayName = "Triangle"),
	Rectangle UMETA(DisplayName = "Rectangle")
};

/** Identifies whether a tactical agent is implemented by a component or exists only as a virtual runtime agent. */
UENUM(BlueprintType)
enum class EAITCSAgentKind : uint8
{
	ActorComponent UMETA(DisplayName = "Actor Component"),
	VirtualAgent UMETA(DisplayName = "Virtual Agent")
};

/** Tactical attitude of an object relative to the player and other teams. */
UENUM(BlueprintType)
enum class EAITCSTacticalObjectAttitude : uint8
{
	Friendly UMETA(DisplayName = "Friendly"),
	Hostile UMETA(DisplayName = "Hostile"),
	Neutral UMETA(DisplayName = "Neutral")
};

/** Specifies how object facing direction should be interpreted when resolving tactical orientation. */
UENUM(BlueprintType)
enum class EAITCSTacticalDirectionSpace : uint8
{
	World UMETA(DisplayName = "World"),
	RelativeToPlayer UMETA(DisplayName = "Relative To Player")
};

/** Available tactical order types that the director can dispatch to agents. */
UENUM(BlueprintType)
enum class EAITCSOrderType : uint8
{
	None UMETA(DisplayName = "None"),
	Hold UMETA(DisplayName = "Hold"),
	MoveTo UMETA(DisplayName = "Move To"),
	Regroup UMETA(DisplayName = "Regroup"),
	Flank UMETA(DisplayName = "Flank"),
	Retreat UMETA(DisplayName = "Retreat"),
	Support UMETA(DisplayName = "Support"),
	Custom UMETA(DisplayName = "Custom")
};

/** State of a tactical order in the dispatch pipeline and execution workflow. */
UENUM(BlueprintType)
enum class EAITCSOrderState : uint8
{
	Pending UMETA(DisplayName = "Pending"),
	Dispatched UMETA(DisplayName = "Dispatched"),
	Completed UMETA(DisplayName = "Completed"),
	Cancelled UMETA(DisplayName = "Cancelled")
};

/** Severity levels used during graph validation and runtime diagnostics. */
UENUM(BlueprintType)
enum class EAITCSValidationSeverity : uint8
{
	Info UMETA(DisplayName = "Info"),
	Warning UMETA(DisplayName = "Warning"),
	Error UMETA(DisplayName = "Error")
};

/**
 * Tactical graph object definition that describes a battlefield entity, virtual anchor, or group role.
 * Objects are used by the AITCS Director to resolve formation behavior, group membership, and tactical context.
 */
USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalObject
{
	GENERATED_BODY()

	FAITCSTacticalObject();

	/** Unique identifier assigned to the object for runtime matching and graph compilation. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object", meta = (IgnoreForMemberInitializationTest))
	FGuid ObjectId;

	/** User-visible object label shown in the tactical graph editor and runtime diagnostics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Object")
	FName DisplayName = NAME_None;

	/** The attitude of this object relative to the player and other tactical teams. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Object")
	EAITCSTacticalObjectAttitude Attitude = EAITCSTacticalObjectAttitude::Hostile;

	/** Editor-only shape used to visualize the object on the tactical canvas. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Visual")
	EAITCSTacticalObjectShape Shape = EAITCSTacticalObjectShape::Circle;

	/** Color used to render the object and convey attitude or role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Visual")
	FLinearColor Color = FLinearColor(0.2f, 0.55f, 1.0f, 1.0f);

	/** Preferred facing direction for the object in degrees. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Direction", meta = (ClampMin = "0.0", ClampMax = "360.0", UIMin = "0.0", UIMax = "360.0"))
	float PreferredDirectionDegrees = 0.0f;

	/** Direction space used when interpreting preferred facing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Direction")
	EAITCSTacticalDirectionSpace DirectionSpace = EAITCSTacticalDirectionSpace::RelativeToPlayer;

	/** Preferred fallback distance to the player when the object acts as a follow anchor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Player", meta = (ClampMin = "0.0", UIMin = "0.0", DisplayName = "Desired Player Distance (Fallback Follow Distance)"))
	float DesiredPlayerDistance = 800.0f;

	/** Maximum distance at which this object should engage its target or goal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Combat", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float AttackTargetDistance = 800.0f;

	/** If enabled, overrides standard formation spacing with attack target distance rules. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Combat")
	bool bUseAttackTargetDistanceAsFormationOverride = false;

	/** Radius around the player within which the object is considered engaged. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Player", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PlayerEngagementRadius = 0.0f;

	/** Marks this object as a simple spatial anchor without an authored group configuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Object")
	bool bIsSimpleObject = false;

	/** If true, the object will return to its initial spawn location when the player is outside engagement distance. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Player")
	bool bReturnToInitialLocationOutsideEngagementRadius = true;

	/** Group assets referenced by this object for tactical membership and behavior profiles. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Groups")
	TArray<TSoftObjectPtr<UAITCSTacticalGroupDataAsset>> GroupAssets;

	/** Index of the currently active group within the referenced group assets array. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Groups", meta = (ClampMin = "0", UIMin = "0"))
	int32 ActiveGroupIndex = 0;

	/** Optional gameplay tag representing the object's tactical role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Grouping")
	FGameplayTag RoleTag;

	/** Soft pawn class to spawn or associate with this tactical object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|NPC")
	TSoftClassPtr<APawn> PawnClass;

	/** Optional AIController class for the object’s agent or virtual actor. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|NPC")
	TSoftClassPtr<AAIController> AIControllerClass;

	/** External integration extensions that receive tactical order callbacks. */
	UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite, Category = "AITCS|Integration", meta=(DisplayName = "External Extensions"))
	TArray<TObjectPtr<UAITCSExternalAIExtension>> ExternalExtensions;

	/** Free-form author note for designers and runtime diagnostics. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Object", meta = (MultiLine = true))
	FString Comment;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalLink
{
	GENERATED_BODY()

	FAITCSTacticalLink();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link", meta = (IgnoreForMemberInitializationTest))
	FGuid LinkId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	FGuid SourceObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	FGuid TargetObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Link")
	FName DisplayName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Relationship")
	FGameplayTag RelationshipTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Relationship")
	FGameplayTagContainer CoordinationTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Relationship", meta = (ClampMin = "0", UIMin = "0"))
	int32 Priority = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Spatial", meta = (ClampMin = "0", UIMin = "0"))
	float MinimumDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Spatial", meta = (ClampMin = "0", UIMin = "0"))
	float MaximumDistance = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Spatial", meta = (ClampMin = "0", UIMin = "0"))
	float PreferredDistance = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Formation")
	bool bAllowFormationAffinity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Support")
	bool bSupportRelationship = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Link", meta = (MultiLine = true))
	FString Notes;

	bool Connects(const FGuid& InSourceObjectId, const FGuid& InTargetObjectId) const;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalGroup
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Group")
	FGameplayTag GroupId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Group")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Behavior")
	FGameplayTag DefaultBehaviorProfile;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Behavior")
	FGameplayTagContainer RuleTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Group", meta = (ClampMin = "0", UIMin = "0"))
	int32 ExpectedSize = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Group", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxMembers = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Spatial", meta = (ClampMin = "0", UIMin = "0"))
	float CohesionDistance = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Formation", meta = (ClampMin = "0", UIMin = "0"))
	float FormationMemberSpacing = 175.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Spatial", meta = (ClampMin = "0", UIMin = "0"))
	float RegroupRadius = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Spatial", meta = (ClampMin = "0", UIMin = "0"))
	float AllySearchRadius = 2500.0f;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalAgentHandle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FGuid AgentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FGuid ObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FGameplayTag GroupId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FName DebugName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	EAITCSAgentKind AgentKind = EAITCSAgentKind::ActorComponent;

	bool IsValid() const;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalAgentRegistration
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FGuid PreferredAgentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FGuid ObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FGameplayTag GroupId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FName DebugName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FGameplayTagContainer AgentTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	TSubclassOf<AAIController> AIControllerClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Agent")
	EAITCSAgentKind AgentKind = EAITCSAgentKind::ActorComponent;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalAgentRuntimeState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	FAITCSTacticalAgentHandle Handle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	FGameplayTagContainer AgentTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	TSubclassOf<APawn> PawnClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	TSubclassOf<AAIController> AIControllerClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	bool bActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	float LastUpdateTime = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Agent")
	TObjectPtr<AActor> Actor = nullptr;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalRuntimeGroupState
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FGuid TacticalObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FName TacticalObjectName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FGameplayTag GroupId;

	/** Runtime-only pool used by a Simple Object when it has no authored group. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	bool bIsSimpleObjectFreePool = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FText GroupDisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	int32 ExpectedSize = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	int32 MaxMembers = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	TArray<FGuid> MemberAgentIds;

	bool HasCapacityLimit() const;
	bool HasFreeSlot() const;
	int32 GetFreeSlotCount() const;
};

/**
 * Authoritative object settings resolved for one registered tactical unit.
 * This is deliberately independent from formation destination calculation so
 * StateTree can consume graph data even when no movement target is available.
 */
USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSResolvedTacticalObjectData
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	bool bIsValid = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	FGuid TacticalObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	FName TacticalObjectName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	FGameplayTag RoleTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Player")
	float DesiredPlayerDistance = 0.0f;

	/** Effective follow distance, including the authored formation override and StateTree fallback. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Player")
	float FollowDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Combat")
	float AttackTargetDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Combat")
	bool bUseAttackTargetDistanceAsFormationOverride = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	bool bIsSimpleObject = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	bool bHasActiveGroup = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	bool bIsSimpleObjectFreePool = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FGameplayTag GroupId;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalContextSnapshot
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Context")
	FName ActiveGraphName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Context")
	TArray<FAITCSTacticalObject> Objects;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Context")
	TArray<FAITCSTacticalLink> Links;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Context")
	TArray<TSoftObjectPtr<UAITCSTacticalGroupDataAsset>> GroupAssets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Context")
	TArray<FAITCSTacticalAgentRuntimeState> Agents;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalOrder
{
	GENERATED_BODY()

	FAITCSTacticalOrder();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order", meta = (IgnoreForMemberInitializationTest))
	FGuid OrderId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order")
	EAITCSOrderType OrderType = EAITCSOrderType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order")
	EAITCSOrderState State = EAITCSOrderState::Pending;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Targeting")
	FGuid TargetAgentId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Targeting")
	FGuid TargetObjectId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Targeting")
	FGameplayTag TargetGroupId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Targeting")
	FGameplayTagContainer TargetTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order")
	FGameplayTag OrderTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order")
	FGameplayTagContainer OrderTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order")
	FVector DesiredLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order")
	float Priority = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order")
	float ExpirationTime = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Order", meta = (MultiLine = true))
	FString Payload;

	bool HasExplicitTarget() const;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSGraphValidationMessage
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Validation")
	EAITCSValidationSeverity Severity = EAITCSValidationSeverity::Info;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Validation")
	FText Message;
};

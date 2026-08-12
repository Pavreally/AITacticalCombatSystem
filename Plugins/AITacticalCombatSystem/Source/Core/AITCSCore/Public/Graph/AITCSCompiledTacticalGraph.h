// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Data/AITCSCoreData.h"

#include "AITCSCompiledTacticalGraph.generated.h"

class UAITCSTacticalGraphAsset;
class UAITCSTacticalGroupDataAsset;

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalGraphBuildOptions
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Graph Compile")
	bool bLoadGroupAssetsSynchronously = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Graph Compile")
	bool bCacheInactiveGroupAssets = true;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalDistanceConstraint
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Constraint")
	float MinimumDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Constraint")
	float MaximumDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Constraint")
	float PreferredDistance = 0.0f;

	bool IsValidRange() const;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalDirectionSector
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Direction")
	FGuid ObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Direction")
	EAITCSTacticalDirectionSpace DirectionSpace = EAITCSTacticalDirectionSpace::RelativeToPlayer;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Direction")
	float AuthoredCenterDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Direction")
	float CompiledCenterDegrees = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Direction")
	float SectorHalfAngleDegrees = 45.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Direction")
	bool bRequiresPlayerReference = false;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalRelationshipContract
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	FGuid LinkId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	FGuid SourceObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	FGuid TargetObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	FGameplayTag RelationshipTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	FGameplayTagContainer CoordinationTags;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	int32 Priority = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	FAITCSCompiledTacticalDistanceConstraint Distance;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	bool bAllowFormationAffinity = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Relationship")
	bool bSupportRelationship = false;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalLink
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	int32 LinkIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	FGuid LinkId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	FGuid SourceObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	FGuid TargetObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	int32 SourceObjectIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	int32 TargetObjectIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	FName DisplayName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Link")
	FAITCSCompiledTacticalRelationshipContract Contract;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalObject
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	int32 ObjectIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	FGuid ObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	FName DisplayName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	EAITCSTacticalObjectAttitude Attitude = EAITCSTacticalObjectAttitude::Hostile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	FGameplayTag RoleTag;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|NPC")
	TSoftClassPtr<APawn> PawnClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|NPC")
	TSoftClassPtr<AAIController> AIControllerClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Player")
	float DesiredPlayerDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Combat")
	float AttackTargetDistance = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Combat")
	bool bUseAttackTargetDistanceAsFormationOverride = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Player")
	float PlayerEngagementRadius = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	bool bIsSimpleObject = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Player")
	bool bReturnToInitialLocationOutsideEngagementRadius = true;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Object")
	bool bIsPlayerObject = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Groups")
	bool bHasActiveGroup = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Groups")
	int32 ActiveGroupIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Groups")
	FGameplayTag ActiveGroupId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Groups")
	TSoftObjectPtr<UAITCSTacticalGroupDataAsset> ActiveGroupAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Groups")
	TObjectPtr<UAITCSTacticalGroupDataAsset> ResolvedActiveGroupAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Groups")
	FAITCSTacticalGroup ActiveGroup;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Direction")
	FAITCSCompiledTacticalDirectionSector DirectionSector;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Adjacency")
	TArray<FGuid> ChildObjectIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Adjacency")
	TArray<FGuid> ParentObjectIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Adjacency")
	TArray<FGuid> ChildLinkIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Adjacency")
	TArray<FGuid> ParentLinkIds;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalGroup
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FGameplayTag GroupId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FText DisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	TSoftObjectPtr<UAITCSTacticalGroupDataAsset> GroupAsset;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	TObjectPtr<UAITCSTacticalGroupDataAsset> ResolvedGroupAsset = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	FAITCSTacticalGroup Group;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	TArray<FGuid> MemberObjectIds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Group")
	TArray<int32> MemberObjectIndices;
};

USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSCompiledTacticalGraph
{
	GENERATED_BODY()

	FAITCSCompiledTacticalGraph();

	bool CompileFromGraph(const UAITCSTacticalGraphAsset* GraphAsset, const FAITCSCompiledTacticalGraphBuildOptions& Options, TArray<FAITCSGraphValidationMessage>& OutMessages);
	void Reset();
	bool IsCompiled() const;

	int32 GetObjectIndex(const FGuid& ObjectId) const;
	int32 GetLinkIndex(const FGuid& LinkId) const;
	int32 GetGroupIndex(const FGameplayTag& GroupId) const;

	const FAITCSCompiledTacticalObject* FindObject(const FGuid& ObjectId) const;
	const FAITCSCompiledTacticalObject* FindSimpleObjectFallback() const;
	const FAITCSCompiledTacticalLink* FindLink(const FGuid& LinkId) const;
	const FAITCSCompiledTacticalGroup* FindGroup(const FGameplayTag& GroupId) const;

	const TArray<int32>* FindChildLinkIndices(const FGuid& ObjectId) const;
	const TArray<int32>* FindParentLinkIndices(const FGuid& ObjectId) const;
	const TArray<int32>* FindGroupObjectIndices(const FGameplayTag& GroupId) const;

	void GetChildLinks(const FGuid& ObjectId, TArray<const FAITCSCompiledTacticalLink*>& OutLinks) const;
	void GetParentLinks(const FGuid& ObjectId, TArray<const FAITCSCompiledTacticalLink*>& OutLinks) const;
	void GetGroupObjects(const FGameplayTag& GroupId, TArray<const FAITCSCompiledTacticalObject*>& OutObjects) const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	FName SourceGraphName = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	FGuid PlayerObjectId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	int32 PlayerObjectIndex = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	TArray<FAITCSCompiledTacticalObject> Objects;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	TArray<FAITCSCompiledTacticalLink> Links;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	TArray<FAITCSCompiledTacticalGroup> Groups;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	TArray<FAITCSCompiledTacticalRelationshipContract> RelationshipContracts;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	TArray<TSoftObjectPtr<UAITCSTacticalGroupDataAsset>> ReferencedGroupAssets;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	TArray<TObjectPtr<UAITCSTacticalGroupDataAsset>> ResolvedGroupAssets;

private:
	static float NormalizeDegrees(float InDegrees);

	void RebuildRuntimeIndexes();
	void CacheReferencedGroupAssets(const UAITCSTacticalGraphAsset* GraphAsset, const FAITCSCompiledTacticalGraphBuildOptions& Options);
	void BuildCompiledObject(const UAITCSTacticalGraphAsset* GraphAsset, const FAITCSTacticalObject& SourceObject, const FAITCSCompiledTacticalGraphBuildOptions& Options, TArray<FAITCSGraphValidationMessage>& OutMessages);
	void BuildCompiledLink(const FAITCSTacticalLink& SourceLink, TArray<FAITCSGraphValidationMessage>& OutMessages);
	void RegisterGroupMembership(const FAITCSCompiledTacticalObject& CompiledObject);
	int32 FindOrAddGroup(const FAITCSCompiledTacticalObject& CompiledObject);
	UAITCSTacticalGroupDataAsset* ResolveGroupAsset(const TSoftObjectPtr<UAITCSTacticalGroupDataAsset>& GroupAsset, const FAITCSCompiledTacticalGraphBuildOptions& Options) const;
	void AddResolvedGroupAsset(UAITCSTacticalGroupDataAsset* GroupAsset);

	bool bCompiled = false;
	TMap<FGuid, int32> ObjectIndexById;
	TMap<FGuid, int32> LinkIndexById;
	TMap<FGameplayTag, int32> GroupIndexById;
	TMap<FGuid, TArray<int32>> ChildLinkIndicesByObjectId;
	TMap<FGuid, TArray<int32>> ParentLinkIndicesByObjectId;
	TMap<FGameplayTag, TArray<int32>> ObjectIndicesByGroupId;
};

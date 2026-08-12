// Pavel Gornostaev <https://github.com/Pavreally>

#include "Graph/AITCSCompiledTacticalGraph.h"

#include "Data/AITCSTacticalGroupDataAsset.h"
#include "Graph/AITCSTacticalGraphAsset.h"

#define LOCTEXT_NAMESPACE "AITCSCompiledTacticalGraph"

bool FAITCSCompiledTacticalDistanceConstraint::IsValidRange() const
{
	return MinimumDistance <= MaximumDistance;
}

FAITCSCompiledTacticalGraph::FAITCSCompiledTacticalGraph()
{
	Reset();
}

bool FAITCSCompiledTacticalGraph::CompileFromGraph(const UAITCSTacticalGraphAsset* GraphAsset, const FAITCSCompiledTacticalGraphBuildOptions& Options, TArray<FAITCSGraphValidationMessage>& OutMessages)
{
	Reset();
	OutMessages.Reset();

	if (!GraphAsset)
	{
		FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
		Message.Severity = EAITCSValidationSeverity::Error;
		Message.Message = LOCTEXT("MissingGraphAsset", "Cannot compile tactical graph: graph asset is null.");
		return false;
	}

	GraphAsset->ValidateGraph(OutMessages);

	bool bHasErrors = false;
	for (const FAITCSGraphValidationMessage& Message : OutMessages)
	{
		if (Message.Severity == EAITCSValidationSeverity::Error)
		{
			bHasErrors = true;
			break;
		}
	}

	if (bHasErrors)
	{
		return false;
	}

	SourceGraphName = GraphAsset->GetFName();
	PlayerObjectId = GraphAsset->PlayerObjectId;

	CacheReferencedGroupAssets(GraphAsset, Options);

	for (int32 ObjectIndex = 0; ObjectIndex < GraphAsset->Objects.Num(); ++ObjectIndex)
	{
		BuildCompiledObject(GraphAsset, GraphAsset->Objects[ObjectIndex], Options, OutMessages);
	}

	for (int32 LinkIndex = 0; LinkIndex < GraphAsset->Links.Num(); ++LinkIndex)
	{
		BuildCompiledLink(GraphAsset->Links[LinkIndex], OutMessages);
	}

	for (const FAITCSCompiledTacticalObject& CompiledObject : Objects)
	{
		RegisterGroupMembership(CompiledObject);
	}

	RebuildRuntimeIndexes();
	bCompiled = true;
	return true;
}

void FAITCSCompiledTacticalGraph::Reset()
{
	SourceGraphName = NAME_None;
	PlayerObjectId = FGuid();
	PlayerObjectIndex = INDEX_NONE;
	Objects.Reset();
	Links.Reset();
	Groups.Reset();
	RelationshipContracts.Reset();
	ReferencedGroupAssets.Reset();
	ResolvedGroupAssets.Reset();
	bCompiled = false;
	ObjectIndexById.Reset();
	LinkIndexById.Reset();
	GroupIndexById.Reset();
	ChildLinkIndicesByObjectId.Reset();
	ParentLinkIndicesByObjectId.Reset();
	ObjectIndicesByGroupId.Reset();
}

bool FAITCSCompiledTacticalGraph::IsCompiled() const
{
	return bCompiled;
}

int32 FAITCSCompiledTacticalGraph::GetObjectIndex(const FGuid& ObjectId) const
{
	if (const int32* FoundIndex = ObjectIndexById.Find(ObjectId))
	{
		return *FoundIndex;
	}

	return INDEX_NONE;
}

int32 FAITCSCompiledTacticalGraph::GetLinkIndex(const FGuid& LinkId) const
{
	if (const int32* FoundIndex = LinkIndexById.Find(LinkId))
	{
		return *FoundIndex;
	}

	return INDEX_NONE;
}

int32 FAITCSCompiledTacticalGraph::GetGroupIndex(const FGameplayTag& GroupId) const
{
	if (const int32* FoundIndex = GroupIndexById.Find(GroupId))
	{
		return *FoundIndex;
	}

	return INDEX_NONE;
}

const FAITCSCompiledTacticalObject* FAITCSCompiledTacticalGraph::FindObject(const FGuid& ObjectId) const
{
	const int32 ObjectIndex = GetObjectIndex(ObjectId);
	return Objects.IsValidIndex(ObjectIndex) ? &Objects[ObjectIndex] : nullptr;
}

const FAITCSCompiledTacticalObject* FAITCSCompiledTacticalGraph::FindSimpleObjectFallback() const
{
	for (const FAITCSCompiledTacticalObject& Object : Objects)
	{
		if (Object.bIsSimpleObject)
		{
			return &Object;
		}
	}

	for (const FAITCSCompiledTacticalObject& Object : Objects)
	{
		if (Object.DisplayName == UAITCSTacticalGraphAsset::SimpleObjectNodeName)
		{
			return &Object;
		}
	}

	for (const FAITCSCompiledTacticalObject& Object : Objects)
	{
		if (Object.DisplayName == UAITCSTacticalGraphAsset::OtherObjectsNodeName)
		{
			return &Object;
		}
	}

	return nullptr;
}

const FAITCSCompiledTacticalLink* FAITCSCompiledTacticalGraph::FindLink(const FGuid& LinkId) const
{
	const int32 LinkIndex = GetLinkIndex(LinkId);
	return Links.IsValidIndex(LinkIndex) ? &Links[LinkIndex] : nullptr;
}

const FAITCSCompiledTacticalGroup* FAITCSCompiledTacticalGraph::FindGroup(const FGameplayTag& GroupId) const
{
	const int32 GroupIndex = GetGroupIndex(GroupId);
	return Groups.IsValidIndex(GroupIndex) ? &Groups[GroupIndex] : nullptr;
}

const TArray<int32>* FAITCSCompiledTacticalGraph::FindChildLinkIndices(const FGuid& ObjectId) const
{
	return ChildLinkIndicesByObjectId.Find(ObjectId);
}

const TArray<int32>* FAITCSCompiledTacticalGraph::FindParentLinkIndices(const FGuid& ObjectId) const
{
	return ParentLinkIndicesByObjectId.Find(ObjectId);
}

const TArray<int32>* FAITCSCompiledTacticalGraph::FindGroupObjectIndices(const FGameplayTag& GroupId) const
{
	return ObjectIndicesByGroupId.Find(GroupId);
}

void FAITCSCompiledTacticalGraph::GetChildLinks(const FGuid& ObjectId, TArray<const FAITCSCompiledTacticalLink*>& OutLinks) const
{
	OutLinks.Reset();

	const TArray<int32>* LinkIndices = FindChildLinkIndices(ObjectId);
	if (!LinkIndices)
	{
		return;
	}

	for (const int32 LinkIndex : *LinkIndices)
	{
		if (Links.IsValidIndex(LinkIndex))
		{
			OutLinks.Add(&Links[LinkIndex]);
		}
	}
}

void FAITCSCompiledTacticalGraph::GetParentLinks(const FGuid& ObjectId, TArray<const FAITCSCompiledTacticalLink*>& OutLinks) const
{
	OutLinks.Reset();

	const TArray<int32>* LinkIndices = FindParentLinkIndices(ObjectId);
	if (!LinkIndices)
	{
		return;
	}

	for (const int32 LinkIndex : *LinkIndices)
	{
		if (Links.IsValidIndex(LinkIndex))
		{
			OutLinks.Add(&Links[LinkIndex]);
		}
	}
}

void FAITCSCompiledTacticalGraph::GetGroupObjects(const FGameplayTag& GroupId, TArray<const FAITCSCompiledTacticalObject*>& OutObjects) const
{
	OutObjects.Reset();

	const TArray<int32>* ObjectIndices = FindGroupObjectIndices(GroupId);
	if (!ObjectIndices)
	{
		return;
	}

	for (const int32 ObjectIndex : *ObjectIndices)
	{
		if (Objects.IsValidIndex(ObjectIndex))
		{
			OutObjects.Add(&Objects[ObjectIndex]);
		}
	}
}

float FAITCSCompiledTacticalGraph::NormalizeDegrees(float InDegrees)
{
	float Result = FMath::Fmod(InDegrees, 360.0f);
	if (Result < 0.0f)
	{
		Result += 360.0f;
	}

	return Result;
}

void FAITCSCompiledTacticalGraph::RebuildRuntimeIndexes()
{
	ObjectIndexById.Reset();
	LinkIndexById.Reset();
	GroupIndexById.Reset();
	ChildLinkIndicesByObjectId.Reset();
	ParentLinkIndicesByObjectId.Reset();
	ObjectIndicesByGroupId.Reset();
	PlayerObjectIndex = INDEX_NONE;

	for (int32 ObjectIndex = 0; ObjectIndex < Objects.Num(); ++ObjectIndex)
	{
		const FAITCSCompiledTacticalObject& Object = Objects[ObjectIndex];
		ObjectIndexById.Add(Object.ObjectId, ObjectIndex);

		if (Object.ObjectId == PlayerObjectId)
		{
			PlayerObjectIndex = ObjectIndex;
		}
	}

	for (int32 LinkIndex = 0; LinkIndex < Links.Num(); ++LinkIndex)
	{
		const FAITCSCompiledTacticalLink& Link = Links[LinkIndex];
		LinkIndexById.Add(Link.LinkId, LinkIndex);

		TArray<int32>* ChildIndices = ChildLinkIndicesByObjectId.Find(Link.SourceObjectId);
		if (!ChildIndices)
		{
			ChildLinkIndicesByObjectId.Add(Link.SourceObjectId);
			ChildIndices = ChildLinkIndicesByObjectId.Find(Link.SourceObjectId);
		}

		if (ChildIndices)
		{
			ChildIndices->Add(LinkIndex);
		}

		TArray<int32>* ParentIndices = ParentLinkIndicesByObjectId.Find(Link.TargetObjectId);
		if (!ParentIndices)
		{
			ParentLinkIndicesByObjectId.Add(Link.TargetObjectId);
			ParentIndices = ParentLinkIndicesByObjectId.Find(Link.TargetObjectId);
		}

		if (ParentIndices)
		{
			ParentIndices->Add(LinkIndex);
		}
	}

	for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		const FAITCSCompiledTacticalGroup& Group = Groups[GroupIndex];
		GroupIndexById.Add(Group.GroupId, GroupIndex);
		ObjectIndicesByGroupId.Add(Group.GroupId, Group.MemberObjectIndices);
	}
}

void FAITCSCompiledTacticalGraph::CacheReferencedGroupAssets(const UAITCSTacticalGraphAsset* GraphAsset, const FAITCSCompiledTacticalGraphBuildOptions& Options)
{
	if (!GraphAsset)
	{
		return;
	}

	ReferencedGroupAssets = GraphAsset->CollectReferencedGroupAssets();

	if (!Options.bCacheInactiveGroupAssets)
	{
		return;
	}

	for (const TSoftObjectPtr<UAITCSTacticalGroupDataAsset>& GroupAsset : ReferencedGroupAssets)
	{
		UAITCSTacticalGroupDataAsset* ResolvedGroupAsset = ResolveGroupAsset(GroupAsset, Options);
		AddResolvedGroupAsset(ResolvedGroupAsset);
	}
}

void FAITCSCompiledTacticalGraph::BuildCompiledObject(const UAITCSTacticalGraphAsset* GraphAsset, const FAITCSTacticalObject& SourceObject, const FAITCSCompiledTacticalGraphBuildOptions& Options, TArray<FAITCSGraphValidationMessage>& OutMessages)
{
	FAITCSCompiledTacticalObject& CompiledObject = Objects.AddDefaulted_GetRef();
	CompiledObject.ObjectIndex = Objects.Num() - 1;
	CompiledObject.ObjectId = SourceObject.ObjectId;
	CompiledObject.DisplayName = SourceObject.DisplayName;
	CompiledObject.Attitude = SourceObject.Attitude;
	CompiledObject.RoleTag = SourceObject.RoleTag;
	CompiledObject.PawnClass = SourceObject.PawnClass;
	CompiledObject.AIControllerClass = SourceObject.AIControllerClass;
	CompiledObject.DesiredPlayerDistance = SourceObject.DesiredPlayerDistance;
	CompiledObject.AttackTargetDistance = SourceObject.AttackTargetDistance;
	CompiledObject.bUseAttackTargetDistanceAsFormationOverride = SourceObject.bUseAttackTargetDistanceAsFormationOverride;
	CompiledObject.PlayerEngagementRadius = SourceObject.PlayerEngagementRadius;
	CompiledObject.bIsSimpleObject = SourceObject.bIsSimpleObject;
	CompiledObject.bReturnToInitialLocationOutsideEngagementRadius = SourceObject.bReturnToInitialLocationOutsideEngagementRadius;
	CompiledObject.bIsPlayerObject = GraphAsset && GraphAsset->IsPlayerObject(SourceObject.ObjectId);
	CompiledObject.ActiveGroupIndex = SourceObject.ActiveGroupIndex;

	CompiledObject.DirectionSector.ObjectId = SourceObject.ObjectId;
	CompiledObject.DirectionSector.DirectionSpace = SourceObject.DirectionSpace;
	CompiledObject.DirectionSector.AuthoredCenterDegrees = SourceObject.PreferredDirectionDegrees;
	CompiledObject.DirectionSector.CompiledCenterDegrees = NormalizeDegrees(SourceObject.PreferredDirectionDegrees);
	CompiledObject.DirectionSector.bRequiresPlayerReference = SourceObject.DirectionSpace == EAITCSTacticalDirectionSpace::RelativeToPlayer && !CompiledObject.bIsPlayerObject;

	if (SourceObject.GroupAssets.IsValidIndex(SourceObject.ActiveGroupIndex))
	{
		CompiledObject.ActiveGroupAsset = SourceObject.GroupAssets[SourceObject.ActiveGroupIndex];
		CompiledObject.ResolvedActiveGroupAsset = ResolveGroupAsset(CompiledObject.ActiveGroupAsset, Options);
		AddResolvedGroupAsset(CompiledObject.ResolvedActiveGroupAsset);

		if (CompiledObject.ResolvedActiveGroupAsset)
		{
			CompiledObject.ActiveGroup = CompiledObject.ResolvedActiveGroupAsset->Group;
			CompiledObject.ActiveGroupId = CompiledObject.ActiveGroup.GroupId;
			CompiledObject.bHasActiveGroup = CompiledObject.ActiveGroupId.IsValid();

			if (!CompiledObject.bHasActiveGroup)
			{
				FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
				Message.Severity = EAITCSValidationSeverity::Warning;
				Message.Message = FText::Format(
					LOCTEXT("CompiledObjectInvalidGroupId", "Compiled object {0} references an active group asset without a valid GroupId."),
					FText::FromName(CompiledObject.DisplayName));
			}
		}
		else if (!CompiledObject.ActiveGroupAsset.IsNull())
		{
			FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
			Message.Severity = EAITCSValidationSeverity::Warning;
			Message.Message = FText::Format(
				LOCTEXT("CompiledObjectUnresolvedGroupAsset", "Compiled object {0} has an active group asset that could not be resolved."),
				FText::FromName(CompiledObject.DisplayName));
		}
	}

	ObjectIndexById.Add(CompiledObject.ObjectId, CompiledObject.ObjectIndex);
}

void FAITCSCompiledTacticalGraph::BuildCompiledLink(const FAITCSTacticalLink& SourceLink, TArray<FAITCSGraphValidationMessage>& OutMessages)
{
	const int32 SourceObjectIndex = GetObjectIndex(SourceLink.SourceObjectId);
	const int32 TargetObjectIndex = GetObjectIndex(SourceLink.TargetObjectId);
	if (!Objects.IsValidIndex(SourceObjectIndex) || !Objects.IsValidIndex(TargetObjectIndex))
	{
		return;
	}

	FAITCSCompiledTacticalLink& CompiledLink = Links.AddDefaulted_GetRef();
	CompiledLink.LinkIndex = Links.Num() - 1;
	CompiledLink.LinkId = SourceLink.LinkId;
	CompiledLink.SourceObjectId = SourceLink.SourceObjectId;
	CompiledLink.TargetObjectId = SourceLink.TargetObjectId;
	CompiledLink.SourceObjectIndex = SourceObjectIndex;
	CompiledLink.TargetObjectIndex = TargetObjectIndex;
	CompiledLink.DisplayName = SourceLink.DisplayName;

	CompiledLink.Contract.LinkId = SourceLink.LinkId;
	CompiledLink.Contract.SourceObjectId = SourceLink.SourceObjectId;
	CompiledLink.Contract.TargetObjectId = SourceLink.TargetObjectId;
	CompiledLink.Contract.RelationshipTag = SourceLink.RelationshipTag;
	CompiledLink.Contract.CoordinationTags = SourceLink.CoordinationTags;
	CompiledLink.Contract.Priority = SourceLink.Priority;
	CompiledLink.Contract.Distance.MinimumDistance = SourceLink.MinimumDistance;
	CompiledLink.Contract.Distance.MaximumDistance = SourceLink.MaximumDistance;
	CompiledLink.Contract.Distance.PreferredDistance = SourceLink.PreferredDistance;
	CompiledLink.Contract.bAllowFormationAffinity = SourceLink.bAllowFormationAffinity;
	CompiledLink.Contract.bSupportRelationship = SourceLink.bSupportRelationship;

	if (!CompiledLink.Contract.Distance.IsValidRange())
	{
		FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
		Message.Severity = EAITCSValidationSeverity::Warning;
		Message.Message = FText::Format(
			LOCTEXT("CompiledLinkInvalidDistanceRange", "Compiled link {0} has MinimumDistance greater than MaximumDistance."),
			FText::FromName(CompiledLink.DisplayName));
	}

	if (CompiledLink.Contract.Distance.PreferredDistance < CompiledLink.Contract.Distance.MinimumDistance || CompiledLink.Contract.Distance.PreferredDistance > CompiledLink.Contract.Distance.MaximumDistance)
	{
		FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
		Message.Severity = EAITCSValidationSeverity::Warning;
		Message.Message = FText::Format(
			LOCTEXT("CompiledLinkPreferredDistanceOutOfRange", "Compiled link {0} has PreferredDistance outside its minimum/maximum range."),
			FText::FromName(CompiledLink.DisplayName));
	}

	RelationshipContracts.Add(CompiledLink.Contract);

	FAITCSCompiledTacticalObject& SourceObject = Objects[SourceObjectIndex];
	SourceObject.ChildObjectIds.Add(SourceLink.TargetObjectId);
	SourceObject.ChildLinkIds.Add(SourceLink.LinkId);

	FAITCSCompiledTacticalObject& TargetObject = Objects[TargetObjectIndex];
	TargetObject.ParentObjectIds.Add(SourceLink.SourceObjectId);
	TargetObject.ParentLinkIds.Add(SourceLink.LinkId);
}

void FAITCSCompiledTacticalGraph::RegisterGroupMembership(const FAITCSCompiledTacticalObject& CompiledObject)
{
	if (!CompiledObject.bHasActiveGroup)
	{
		return;
	}

	const int32 GroupIndex = FindOrAddGroup(CompiledObject);
	if (!Groups.IsValidIndex(GroupIndex))
	{
		return;
	}

	FAITCSCompiledTacticalGroup& Group = Groups[GroupIndex];
	Group.MemberObjectIds.Add(CompiledObject.ObjectId);
	Group.MemberObjectIndices.Add(CompiledObject.ObjectIndex);
}

int32 FAITCSCompiledTacticalGraph::FindOrAddGroup(const FAITCSCompiledTacticalObject& CompiledObject)
{
	if (!CompiledObject.ActiveGroupId.IsValid())
	{
		return INDEX_NONE;
	}

	for (int32 GroupIndex = 0; GroupIndex < Groups.Num(); ++GroupIndex)
	{
		if (Groups[GroupIndex].GroupId == CompiledObject.ActiveGroupId)
		{
			return GroupIndex;
		}
	}

	FAITCSCompiledTacticalGroup& Group = Groups.AddDefaulted_GetRef();
	Group.GroupId = CompiledObject.ActiveGroupId;
	Group.DisplayName = CompiledObject.ActiveGroup.DisplayName;
	Group.GroupAsset = CompiledObject.ActiveGroupAsset;
	Group.ResolvedGroupAsset = CompiledObject.ResolvedActiveGroupAsset;
	Group.Group = CompiledObject.ActiveGroup;
	return Groups.Num() - 1;
}

UAITCSTacticalGroupDataAsset* FAITCSCompiledTacticalGraph::ResolveGroupAsset(const TSoftObjectPtr<UAITCSTacticalGroupDataAsset>& GroupAsset, const FAITCSCompiledTacticalGraphBuildOptions& Options) const
{
	if (GroupAsset.IsNull())
	{
		return nullptr;
	}

	UAITCSTacticalGroupDataAsset* ResolvedGroupAsset = GroupAsset.Get();
	if (ResolvedGroupAsset || !Options.bLoadGroupAssetsSynchronously)
	{
		return ResolvedGroupAsset;
	}

	TSoftObjectPtr<UAITCSTacticalGroupDataAsset> GroupAssetCopy = GroupAsset;
	return GroupAssetCopy.LoadSynchronous();
}

void FAITCSCompiledTacticalGraph::AddResolvedGroupAsset(UAITCSTacticalGroupDataAsset* GroupAsset)
{
	if (!GroupAsset)
	{
		return;
	}

	for (const TObjectPtr<UAITCSTacticalGroupDataAsset>& ExistingGroupAsset : ResolvedGroupAssets)
	{
		if (ExistingGroupAsset.Get() == GroupAsset)
		{
			return;
		}
	}

	ResolvedGroupAssets.Add(GroupAsset);
}

#undef LOCTEXT_NAMESPACE

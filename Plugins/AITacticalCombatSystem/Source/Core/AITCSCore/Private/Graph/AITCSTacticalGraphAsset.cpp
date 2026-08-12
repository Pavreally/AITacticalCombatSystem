// Pavel Gornostaev <https://github.com/Pavreally>

#include "Graph/AITCSTacticalGraphAsset.h"

#define LOCTEXT_NAMESPACE "AITCSTacticalGraphAsset"

const FName UAITCSTacticalGraphAsset::PlayerObjectName(TEXT("<Player>"));
const FName UAITCSTacticalGraphAsset::SimpleObjectNodeName(TEXT("Simple Object"));
const FName UAITCSTacticalGraphAsset::OtherObjectsNodeName(TEXT("Other objects"));

UAITCSTacticalGraphAsset::UAITCSTacticalGraphAsset()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		EnsurePlayerObject();
	}
}

void UAITCSTacticalGraphAsset::PostLoad()
{
	Super::PostLoad();
	EnsurePlayerObject();
	DeduplicateIds();
}

#if WITH_EDITOR
void UAITCSTacticalGraphAsset::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	EnsurePlayerObject();
	DeduplicateIds();
}
#endif

bool UAITCSTacticalGraphAsset::EnsurePlayerObject()
{
	bool bChanged = false;
	FAITCSTacticalObject* PlayerObject = nullptr;

	if (PlayerObjectId.IsValid())
	{
		PlayerObject = FindObject(PlayerObjectId);
	}

	if (!PlayerObject)
	{
		for (FAITCSTacticalObject& Object : Objects)
		{
			if (Object.DisplayName == PlayerObjectName)
			{
				PlayerObject = &Object;
				PlayerObjectId = Object.ObjectId;
				bChanged = true;
				break;
			}
		}
	}

	if (!PlayerObject)
	{
		PlayerObject = &Objects.AddDefaulted_GetRef();
		PlayerObject->ObjectId = FGuid::NewGuid();
		PlayerObject->DisplayName = PlayerObjectName;
		PlayerObject->Attitude = EAITCSTacticalObjectAttitude::Friendly;
		PlayerObject->Shape = EAITCSTacticalObjectShape::Diamond;
		PlayerObject->Color = FLinearColor(0.1f, 0.7f, 1.0f, 1.0f);
		PlayerObjectId = PlayerObject->ObjectId;
		bChanged = true;
	}

	if (PlayerObjectId != PlayerObject->ObjectId)
	{
		PlayerObjectId = PlayerObject->ObjectId;
		bChanged = true;
	}

	if (PlayerObject->DisplayName != PlayerObjectName)
	{
		PlayerObject->DisplayName = PlayerObjectName;
		bChanged = true;
	}

	for (FAITCSTacticalObject& Object : Objects)
	{
		if (&Object != PlayerObject && Object.DisplayName == PlayerObjectName)
		{
			Object.DisplayName = TEXT("Tactical Object");
			bChanged = true;
		}
	}

	if (GetEditorNodePosition(PlayerObjectId) != FVector2D::ZeroVector)
	{
		SetEditorNodePosition(PlayerObjectId, FVector2D::ZeroVector);
		bChanged = true;
	}

	return bChanged;
}

FAITCSTacticalObject UAITCSTacticalGraphAsset::AddTacticalObject(FName DisplayName)
{
	Modify();

	FAITCSTacticalObject NewObject;
	NewObject.DisplayName = DisplayName.IsNone() ? FName(TEXT("Tactical Object")) : DisplayName;
	NewObject.Color = FLinearColor(0.85f, 0.42f, 0.18f, 1.0f);

	Objects.Add(NewObject);
	MarkPackageDirty();
	return NewObject;
}

bool UAITCSTacticalGraphAsset::RemoveTacticalObject(const FGuid& ObjectId)
{
	const int32 ObjectIndex = FindObjectIndex(ObjectId);
	if (ObjectIndex == INDEX_NONE || IsPlayerObject(ObjectId))
	{
		return false;
	}

	Modify();
	Objects.RemoveAt(ObjectIndex);
	RemoveEditorNodePosition(ObjectId);
	Links.RemoveAll([&ObjectId](const FAITCSTacticalLink& Link)
	{
		return Link.SourceObjectId == ObjectId || Link.TargetObjectId == ObjectId;
	});
	MarkPackageDirty();
	return true;
}

FAITCSTacticalLink UAITCSTacticalGraphAsset::AddTacticalLink(const FGuid& SourceObjectId, const FGuid& TargetObjectId)
{
	if (!SourceObjectId.IsValid() || !TargetObjectId.IsValid() || SourceObjectId == TargetObjectId)
	{
		return FAITCSTacticalLink();
	}

	if (!HasObject(SourceObjectId) || !HasObject(TargetObjectId))
	{
		return FAITCSTacticalLink();
	}

	if (const FAITCSTacticalLink* ExistingLink = FindLinkBetween(SourceObjectId, TargetObjectId))
	{
		return *ExistingLink;
	}

	Modify();

	FAITCSTacticalLink NewLink;
	NewLink.SourceObjectId = SourceObjectId;
	NewLink.TargetObjectId = TargetObjectId;
	NewLink.DisplayName = TEXT("Relationship");
	Links.Add(NewLink);
	MarkPackageDirty();
	return NewLink;
}

bool UAITCSTacticalGraphAsset::RemoveTacticalLink(const FGuid& LinkId)
{
	const int32 LinkIndex = FindLinkIndex(LinkId);
	if (LinkIndex == INDEX_NONE)
	{
		return false;
	}

	Modify();
	Links.RemoveAt(LinkIndex);
	MarkPackageDirty();
	return true;
}

bool UAITCSTacticalGraphAsset::TryGetTacticalObject(const FGuid& ObjectId, FAITCSTacticalObject& OutObject) const
{
	if (const FAITCSTacticalObject* Object = FindObject(ObjectId))
	{
		OutObject = *Object;
		return true;
	}

	return false;
}

bool UAITCSTacticalGraphAsset::TryGetTacticalLink(const FGuid& LinkId, FAITCSTacticalLink& OutLink) const
{
	if (const FAITCSTacticalLink* Link = FindLink(LinkId))
	{
		OutLink = *Link;
		return true;
	}

	return false;
}

void UAITCSTacticalGraphAsset::ValidateGraph(TArray<FAITCSGraphValidationMessage>& OutMessages) const
{
	OutMessages.Reset();

	TSet<FGuid> ObjectIds;

	for (const FAITCSTacticalObject& Object : Objects)
	{
		if (!Object.ObjectId.IsValid())
		{
			FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
			Message.Severity = EAITCSValidationSeverity::Error;
			Message.Message = LOCTEXT("InvalidObjectId", "A tactical object has an invalid ObjectId.");
			continue;
		}

		if (ObjectIds.Contains(Object.ObjectId))
		{
			FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
			Message.Severity = EAITCSValidationSeverity::Error;
			Message.Message = FText::Format(LOCTEXT("DuplicateObjectId", "Duplicate tactical ObjectId: {0}."), FText::FromString(Object.ObjectId.ToString()));
		}

		ObjectIds.Add(Object.ObjectId);

		if (!Object.GroupAssets.IsEmpty() && !Object.GroupAssets.IsValidIndex(Object.ActiveGroupIndex))
		{
			FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
			Message.Severity = EAITCSValidationSeverity::Warning;
			Message.Message = FText::Format(
				LOCTEXT("InvalidActiveGroupIndex", "{0} has an active group index outside its group asset list."),
				FText::FromName(Object.DisplayName));
		}
	}

	if (!PlayerObjectId.IsValid() || !ObjectIds.Contains(PlayerObjectId))
	{
		FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
		Message.Severity = EAITCSValidationSeverity::Error;
		Message.Message = LOCTEXT("MissingPlayerObject", "A tactical graph must contain one persistent <Player> object.");
	}

	TSet<FGuid> LinkIds;
	for (const FAITCSTacticalLink& Link : Links)
	{
		if (!Link.LinkId.IsValid())
		{
			FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
			Message.Severity = EAITCSValidationSeverity::Error;
			Message.Message = LOCTEXT("InvalidLinkId", "A tactical link has an invalid LinkId.");
			continue;
		}

		if (LinkIds.Contains(Link.LinkId))
		{
			FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
			Message.Severity = EAITCSValidationSeverity::Error;
			Message.Message = FText::Format(LOCTEXT("DuplicateLinkId", "Duplicate tactical LinkId: {0}."), FText::FromString(Link.LinkId.ToString()));
		}

		if (!ObjectIds.Contains(Link.SourceObjectId) || !ObjectIds.Contains(Link.TargetObjectId))
		{
			FAITCSGraphValidationMessage& Message = OutMessages.AddDefaulted_GetRef();
			Message.Severity = EAITCSValidationSeverity::Error;
			Message.Message = LOCTEXT("DanglingLink", "A tactical link references an object that does not exist.");
		}

		LinkIds.Add(Link.LinkId);
	}
}

bool UAITCSTacticalGraphAsset::IsValidGraph() const
{
	TArray<FAITCSGraphValidationMessage> Messages;
	ValidateGraph(Messages);

	for (const FAITCSGraphValidationMessage& Message : Messages)
	{
		if (Message.Severity == EAITCSValidationSeverity::Error)
		{
			return false;
		}
	}

	return true;
}

bool UAITCSTacticalGraphAsset::IsPlayerObject(const FGuid& ObjectId) const
{
	return ObjectId.IsValid() && ObjectId == PlayerObjectId;
}

FVector2D UAITCSTacticalGraphAsset::GetEditorNodePosition(const FGuid& ObjectId) const
{
	if (IsPlayerObject(ObjectId))
	{
		return FVector2D::ZeroVector;
	}

	const FAITCSTacticalGraphNodePlacement* Placement = EditorNodePlacements.FindByPredicate([&ObjectId](const FAITCSTacticalGraphNodePlacement& Candidate)
	{
		return Candidate.ObjectId == ObjectId;
	});

	return Placement ? Placement->Position : FVector2D::ZeroVector;
}

void UAITCSTacticalGraphAsset::SetEditorNodePosition(const FGuid& ObjectId, const FVector2D& Position)
{
	if (!ObjectId.IsValid())
	{
		return;
	}

	const FVector2D EffectivePosition = IsPlayerObject(ObjectId) ? FVector2D::ZeroVector : Position;
	FAITCSTacticalGraphNodePlacement* Placement = EditorNodePlacements.FindByPredicate([&ObjectId](const FAITCSTacticalGraphNodePlacement& Candidate)
	{
		return Candidate.ObjectId == ObjectId;
	});

	if (Placement)
	{
		Placement->Position = EffectivePosition;
		return;
	}

	FAITCSTacticalGraphNodePlacement& NewPlacement = EditorNodePlacements.AddDefaulted_GetRef();
	NewPlacement.ObjectId = ObjectId;
	NewPlacement.Position = EffectivePosition;
}

void UAITCSTacticalGraphAsset::RemoveEditorNodePosition(const FGuid& ObjectId)
{
	EditorNodePlacements.RemoveAll([&ObjectId](const FAITCSTacticalGraphNodePlacement& Placement)
	{
		return Placement.ObjectId == ObjectId;
	});
}

TArray<TSoftObjectPtr<UAITCSTacticalGroupDataAsset>> UAITCSTacticalGraphAsset::CollectReferencedGroupAssets() const
{
	TArray<TSoftObjectPtr<UAITCSTacticalGroupDataAsset>> Result;
	TSet<FSoftObjectPath> SeenPaths;

	for (const FAITCSTacticalObject& Object : Objects)
	{
		for (const TSoftObjectPtr<UAITCSTacticalGroupDataAsset>& GroupAsset : Object.GroupAssets)
		{
			if (GroupAsset.IsNull())
			{
				continue;
			}

			const FSoftObjectPath Path = GroupAsset.ToSoftObjectPath();
			if (!SeenPaths.Contains(Path))
			{
				SeenPaths.Add(Path);
				Result.Add(GroupAsset);
			}
		}
	}

	return Result;
}

FAITCSTacticalObject* UAITCSTacticalGraphAsset::FindObject(const FGuid& ObjectId)
{
	const int32 ObjectIndex = FindObjectIndex(ObjectId);
	return ObjectIndex != INDEX_NONE ? &Objects[ObjectIndex] : nullptr;
}

FAITCSTacticalObject* UAITCSTacticalGraphAsset::FindSimpleObjectConfig()
{
	for (FAITCSTacticalObject& Object : Objects)
	{
		if (Object.bIsSimpleObject)
		{
			return &Object;
		}
	}

	for (FAITCSTacticalObject& Object : Objects)
	{
		if (Object.DisplayName == SimpleObjectNodeName)
		{
			return &Object;
		}
	}

	for (FAITCSTacticalObject& Object : Objects)
	{
		if (Object.DisplayName == OtherObjectsNodeName)
		{
			return &Object;
		}
	}

	return nullptr;
}

const FAITCSTacticalObject* UAITCSTacticalGraphAsset::FindSimpleObjectConfig() const
{
	for (const FAITCSTacticalObject& Object : Objects)
	{
		if (Object.bIsSimpleObject)
		{
			return &Object;
		}
	}

	for (const FAITCSTacticalObject& Object : Objects)
	{
		if (Object.DisplayName == SimpleObjectNodeName)
		{
			return &Object;
		}
	}

	for (const FAITCSTacticalObject& Object : Objects)
	{
		if (Object.DisplayName == OtherObjectsNodeName)
		{
			return &Object;
		}
	}

	return nullptr;
}

bool UAITCSTacticalGraphAsset::HasSimpleObjectConfig() const
{
	return FindSimpleObjectConfig() != nullptr;
}

const FAITCSTacticalObject* UAITCSTacticalGraphAsset::FindObject(const FGuid& ObjectId) const
{
	const int32 ObjectIndex = FindObjectIndex(ObjectId);
	return ObjectIndex != INDEX_NONE ? &Objects[ObjectIndex] : nullptr;
}

FAITCSTacticalLink* UAITCSTacticalGraphAsset::FindLink(const FGuid& LinkId)
{
	const int32 LinkIndex = FindLinkIndex(LinkId);
	return LinkIndex != INDEX_NONE ? &Links[LinkIndex] : nullptr;
}

const FAITCSTacticalLink* UAITCSTacticalGraphAsset::FindLink(const FGuid& LinkId) const
{
	const int32 LinkIndex = FindLinkIndex(LinkId);
	return LinkIndex != INDEX_NONE ? &Links[LinkIndex] : nullptr;
}

const FAITCSTacticalLink* UAITCSTacticalGraphAsset::FindLinkBetween(const FGuid& SourceObjectId, const FGuid& TargetObjectId) const
{
	return Links.FindByPredicate([&SourceObjectId, &TargetObjectId](const FAITCSTacticalLink& Link)
	{
		return Link.Connects(SourceObjectId, TargetObjectId);
	});
}

int32 UAITCSTacticalGraphAsset::FindObjectIndex(const FGuid& ObjectId) const
{
	return Objects.IndexOfByPredicate([&ObjectId](const FAITCSTacticalObject& Object)
	{
		return Object.ObjectId == ObjectId;
	});
}

int32 UAITCSTacticalGraphAsset::FindLinkIndex(const FGuid& LinkId) const
{
	return Links.IndexOfByPredicate([&LinkId](const FAITCSTacticalLink& Link)
	{
		return Link.LinkId == LinkId;
	});
}

bool UAITCSTacticalGraphAsset::HasObject(const FGuid& ObjectId) const
{
	return FindObjectIndex(ObjectId) != INDEX_NONE;
}

void UAITCSTacticalGraphAsset::DeduplicateIds()
{
	EnsurePlayerObject();

	TSet<FGuid> ObjectIds;
	for (FAITCSTacticalObject& Object : Objects)
	{
		const bool bWasPlayerObject = Object.ObjectId == PlayerObjectId;
		if (!Object.ObjectId.IsValid() || ObjectIds.Contains(Object.ObjectId))
		{
			Object.ObjectId = FGuid::NewGuid();
			if (bWasPlayerObject)
			{
				PlayerObjectId = Object.ObjectId;
			}
		}

		ObjectIds.Add(Object.ObjectId);
	}

	TSet<FGuid> LinkIds;
	for (FAITCSTacticalLink& Link : Links)
	{
		if (!Link.LinkId.IsValid() || LinkIds.Contains(Link.LinkId))
		{
			Link.LinkId = FGuid::NewGuid();
		}

		LinkIds.Add(Link.LinkId);
	}

	Links.RemoveAll([this](const FAITCSTacticalLink& Link)
	{
		return Link.SourceObjectId == Link.TargetObjectId || !HasObject(Link.SourceObjectId) || !HasObject(Link.TargetObjectId);
	});

	EditorNodePlacements.RemoveAll([this](const FAITCSTacticalGraphNodePlacement& Placement)
	{
		return !HasObject(Placement.ObjectId);
	});

	SetEditorNodePosition(PlayerObjectId, FVector2D::ZeroVector);
}

void UAITCSTacticalGraphAsset::DeduplicateObjectIds()
{
	DeduplicateIds();
}

#undef LOCTEXT_NAMESPACE

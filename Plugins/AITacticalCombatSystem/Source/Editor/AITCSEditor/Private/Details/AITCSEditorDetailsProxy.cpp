// Pavel Gornostaev <https://github.com/Pavreally>

#include "Details/AITCSEditorDetailsProxy.h"

#include "Graph/AITCSTacticalGraphAsset.h"
#include "EdGraphNode_Comment.h"
#include "UObject/UnrealType.h"

void UAITCSEditorObjectDetailsProxy::LoadFromObject(const FAITCSTacticalObject& InObject)
{
	ObjectData = InObject;
	SourceObjectId = InObject.ObjectId;
}

bool UAITCSEditorObjectDetailsProxy::ApplyToGraphAsset(UAITCSTacticalGraphAsset* GraphAsset) const
{
	if (!GraphAsset)
	{
		return false;
	}

	FAITCSTacticalObject* Object = GraphAsset->FindObject(SourceObjectId);
	if (!Object)
	{
		return false;
	}

	GraphAsset->Modify();
	const FGuid StableObjectId = Object->ObjectId;
	const bool bIsPlayerObject = GraphAsset->IsPlayerObject(SourceObjectId);

	*Object = ObjectData;
	Object->ObjectId = StableObjectId;
	if (Object->bIsSimpleObject)
	{
		Object->GroupAssets.Reset();
		Object->ActiveGroupIndex = INDEX_NONE;
	}
	else if (Object->GroupAssets.IsEmpty())
	{
		Object->ActiveGroupIndex = 0;
	}
	else
	{
		Object->ActiveGroupIndex = FMath::Clamp(Object->ActiveGroupIndex, 0, Object->GroupAssets.Num() - 1);
	}

	if (bIsPlayerObject)
	{
		Object->DisplayName = UAITCSTacticalGraphAsset::PlayerObjectName;
		Object->Attitude = EAITCSTacticalObjectAttitude::Friendly;
	}

	GraphAsset->EnsurePlayerObject();
	GraphAsset->MarkPackageDirty();
	return true;
}

void UAITCSEditorLinkDetailsProxy::LoadFromLink(const FAITCSTacticalLink& InLink, const UAITCSTacticalGraphAsset* GraphAsset)
{
	LinkData = InLink;
	SourceLinkId = InLink.LinkId;

	const auto BuildObjectLabel = [GraphAsset](const FGuid& ObjectId) -> FText
	{
		if (!GraphAsset)
		{
			return FText::FromString(ObjectId.ToString(EGuidFormats::Short));
		}

		if (const FAITCSTacticalObject* Object = GraphAsset->FindObject(ObjectId))
		{
			return FText::FromString(FString::Printf(
				TEXT("%s (%s)"),
				*Object->DisplayName.ToString(),
				*ObjectId.ToString(EGuidFormats::Short)));
		}

		return FText::FromString(ObjectId.ToString(EGuidFormats::Short));
	};

	ParentObject = BuildObjectLabel(InLink.SourceObjectId);
	ChildObject = BuildObjectLabel(InLink.TargetObjectId);
}

bool UAITCSEditorLinkDetailsProxy::ApplyToGraphAsset(UAITCSTacticalGraphAsset* GraphAsset) const
{
	if (!GraphAsset)
	{
		return false;
	}

	FAITCSTacticalLink* Link = GraphAsset->FindLink(SourceLinkId);
	if (!Link)
	{
		return false;
	}

	GraphAsset->Modify();
	const FGuid StableLinkId = Link->LinkId;
	const FGuid StableSourceObjectId = Link->SourceObjectId;
	const FGuid StableTargetObjectId = Link->TargetObjectId;

	*Link = LinkData;
	Link->LinkId = StableLinkId;
	Link->SourceObjectId = StableSourceObjectId;
	Link->TargetObjectId = StableTargetObjectId;

	GraphAsset->MarkPackageDirty();
	return true;
}

void UAITCSEditorCommentDetailsProxy::LoadFromCommentNode(const UEdGraphNode_Comment* InCommentNode)
{
	if (!InCommentNode)
	{
		return;
	}

	CommentText = FText::FromString(InCommentNode->NodeComment);
	CommentColor = InCommentNode->CommentColor;
	NodeDetails = InCommentNode->NodeDetails;
	SourceNodeGuid = InCommentNode->NodeGuid;
}

bool UAITCSEditorCommentDetailsProxy::ApplyToCommentNode(UEdGraphNode_Comment* CommentNode) const
{
	if (!CommentNode)
	{
		return false;
	}

	const FScopedTransaction Transaction(NSLOCTEXT("AITCSEditor", "EditNodeComment", "Edit Comment"));
	CommentNode->Modify();

	// Update NodeComment (base class property)
	FProperty* NodeCommentProperty = CommentNode->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UEdGraphNode, NodeComment));
	if (NodeCommentProperty != nullptr)
	{
		CommentNode->PreEditChange(NodeCommentProperty);
		CommentNode->NodeComment = CommentText.ToString();
		CommentNode->SetMakeCommentBubbleVisible(true);
		FPropertyChangedEvent NodeCommentPropertyChanged(NodeCommentProperty);
		CommentNode->PostEditChangeProperty(NodeCommentPropertyChanged);
	}

	// Update comment color
	FProperty* CommentColorProperty = CommentNode->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UEdGraphNode_Comment, CommentColor));
	if (CommentColorProperty != nullptr)
	{
		CommentNode->PreEditChange(CommentColorProperty);
		CommentNode->CommentColor = CommentColor;
		FPropertyChangedEvent CommentColorPropertyChanged(CommentColorProperty);
		CommentNode->PostEditChangeProperty(CommentColorPropertyChanged);
	}

	// Update NodeDetails
	FProperty* NodeDetailsProperty = CommentNode->GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UEdGraphNode_Comment, NodeDetails));
	if (NodeDetailsProperty != nullptr)
	{
		CommentNode->PreEditChange(NodeDetailsProperty);
		CommentNode->NodeDetails = NodeDetails;
		FPropertyChangedEvent NodeDetailsPropertyChanged(NodeDetailsProperty);
		CommentNode->PostEditChangeProperty(NodeDetailsPropertyChanged);
	}

	return true;
}

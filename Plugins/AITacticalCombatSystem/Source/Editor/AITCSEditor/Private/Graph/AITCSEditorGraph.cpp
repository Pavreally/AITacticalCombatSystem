// Pavel Gornostaev <https://github.com/Pavreally>

#include "Graph/AITCSEditorGraph.h"

#include "EdGraphNode_Comment.h"
#include "Graph/AITCSEditorGraphNode.h"
#include "Graph/AITCSEditorGraphSchema.h"
#include "Graph/AITCSTacticalGraphAsset.h"

namespace
{
	bool AreLinksEquivalent(const FAITCSTacticalLink &A, const FAITCSTacticalLink &B)
	{
		return A.LinkId == B.LinkId && A.SourceObjectId == B.SourceObjectId && A.TargetObjectId == B.TargetObjectId && A.DisplayName == B.DisplayName && A.RelationshipTag == B.RelationshipTag && A.CoordinationTags == B.CoordinationTags && A.Priority == B.Priority && FMath::IsNearlyEqual(A.MinimumDistance, B.MinimumDistance) && FMath::IsNearlyEqual(A.MaximumDistance, B.MaximumDistance) && FMath::IsNearlyEqual(A.PreferredDistance, B.PreferredDistance) && A.bAllowFormationAffinity == B.bAllowFormationAffinity && A.bSupportRelationship == B.bSupportRelationship && A.Notes == B.Notes;
	}

	bool AreLinkArraysEquivalent(const TArray<FAITCSTacticalLink> &A, const TArray<FAITCSTacticalLink> &B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!AreLinksEquivalent(A[Index], B[Index]))
			{
				return false;
			}
		}

		return true;
	}

	bool AreCommentsEquivalent(const FAITCSTacticalGraphComment &A, const FAITCSTacticalGraphComment &B)
	{
		return A.CommentId == B.CommentId && A.Text == B.Text && A.Position == B.Position && A.Size == B.Size && A.Color.Equals(B.Color);
	}

	bool AreCommentArraysEquivalent(const TArray<FAITCSTacticalGraphComment> &A, const TArray<FAITCSTacticalGraphComment> &B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (!AreCommentsEquivalent(A[Index], B[Index]))
			{
				return false;
			}
		}

		return true;
	}
}

void UAITCSEditorGraph::RebuildGraphFromAsset()
{
	if (!GraphAsset)
	{
		return;
	}

	Nodes.Reset();

	GraphAsset->EnsurePlayerObject();
	GraphAsset->DeduplicateObjectIds();

	TMap<FGuid, UAITCSEditorGraphNode *> NodeByObjectId;

	for (const FAITCSTacticalObject &Object : GraphAsset->Objects)
	{
		UAITCSEditorGraphNode *Node = CreateNodeForObject(Object.ObjectId, false, false);
		NodeByObjectId.Add(Object.ObjectId, Node);
	}

	for (const FAITCSTacticalLink &Link : GraphAsset->Links)
	{
		UAITCSEditorGraphNode *SourceNode = NodeByObjectId.FindRef(Link.SourceObjectId);
		UAITCSEditorGraphNode *TargetNode = NodeByObjectId.FindRef(Link.TargetObjectId);
		if (!SourceNode || !TargetNode || !SourceNode->GetOutputPin() || !TargetNode->GetInputPin())
		{
			continue;
		}

		SourceNode->GetOutputPin()->MakeLinkTo(TargetNode->GetInputPin());
	}

	for (const FAITCSTacticalGraphComment &Comment : GraphAsset->EditorComments)
	{
		UEdGraphNode_Comment *CommentNode = NewObject<UEdGraphNode_Comment>(this, NAME_None, RF_Transactional);
		CommentNode->NodeGuid = Comment.CommentId.IsValid() ? Comment.CommentId : FGuid::NewGuid();
		CommentNode->NodeComment = Comment.Text;
		CommentNode->NodePosX = static_cast<int32>(Comment.Position.X);
		CommentNode->NodePosY = static_cast<int32>(Comment.Position.Y);
		CommentNode->NodeWidth = static_cast<int32>(Comment.Size.X);
		CommentNode->NodeHeight = static_cast<int32>(Comment.Size.Y);
		CommentNode->CommentColor = Comment.Color;
		AddNode(CommentNode, false, false);
	}

	NotifyGraphChanged();
}

bool UAITCSEditorGraph::SyncAssetFromGraph(bool bMarkDirty)
{
	if (!GraphAsset)
	{
		return false;
	}

	bool bChanged = false;
	const auto MarkAssetChanged = [this, &bChanged]()
	{
		if (!bChanged)
		{
			GraphAsset->Modify();
			bChanged = true;
		}
	};

	TSet<FGuid> NodeObjectIds;
	for (UEdGraphNode *BaseNode : Nodes)
	{
		UAITCSEditorGraphNode *Node = Cast<UAITCSEditorGraphNode>(BaseNode);
		if (!Node || !Node->ObjectId.IsValid())
		{
			continue;
		}

		NodeObjectIds.Add(Node->ObjectId);
		if (GraphAsset->FindObject(Node->ObjectId))
		{
			if (GraphAsset->IsPlayerObject(Node->ObjectId) && (Node->NodePosX != 0 || Node->NodePosY != 0))
			{
				Node->NodePosX = 0;
				Node->NodePosY = 0;
			}

			const FVector2D Position = GraphAsset->IsPlayerObject(Node->ObjectId)
																		 ? FVector2D::ZeroVector
																		 : FVector2D(Node->NodePosX, Node->NodePosY);
			if (GraphAsset->GetEditorNodePosition(Node->ObjectId) != Position)
			{
				MarkAssetChanged();
				GraphAsset->SetEditorNodePosition(Node->ObjectId, Position);
			}
		}
	}

	const bool bHasRemovedObjects = GraphAsset->Objects.ContainsByPredicate([this, &NodeObjectIds](const FAITCSTacticalObject &Object)
																																					{ return !GraphAsset->IsPlayerObject(Object.ObjectId) && !NodeObjectIds.Contains(Object.ObjectId); });
	if (bHasRemovedObjects)
	{
		MarkAssetChanged();
		GraphAsset->Objects.RemoveAll([this, &NodeObjectIds](const FAITCSTacticalObject &Object)
																	{ return !GraphAsset->IsPlayerObject(Object.ObjectId) && !NodeObjectIds.Contains(Object.ObjectId); });
	}

	TArray<FAITCSTacticalLink> SyncedLinks;
	TSet<FString> SeenPairs;

	for (UEdGraphNode *BaseNode : Nodes)
	{
		UAITCSEditorGraphNode *SourceNode = Cast<UAITCSEditorGraphNode>(BaseNode);
		if (!SourceNode || !SourceNode->GetOutputPin())
		{
			continue;
		}

		for (UEdGraphPin *LinkedPin : SourceNode->GetOutputPin()->LinkedTo)
		{
			const UAITCSEditorGraphNode *TargetNode = LinkedPin ? Cast<UAITCSEditorGraphNode>(LinkedPin->GetOwningNode()) : nullptr;
			if (!TargetNode || SourceNode == TargetNode)
			{
				continue;
			}

			const FString PairKey = SourceNode->ObjectId.ToString() + TEXT("->") + TargetNode->ObjectId.ToString();
			if (SeenPairs.Contains(PairKey))
			{
				continue;
			}
			SeenPairs.Add(PairKey);

			if (const FAITCSTacticalLink *ExistingLink = GraphAsset->FindLinkBetween(SourceNode->ObjectId, TargetNode->ObjectId))
			{
				SyncedLinks.Add(*ExistingLink);
			}
			else
			{
				FAITCSTacticalLink NewLink;
				NewLink.SourceObjectId = SourceNode->ObjectId;
				NewLink.TargetObjectId = TargetNode->ObjectId;
				NewLink.DisplayName = TEXT("Relationship");
				SyncedLinks.Add(NewLink);
			}
		}
	}

	if (!AreLinkArraysEquivalent(GraphAsset->Links, SyncedLinks))
	{
		MarkAssetChanged();
		GraphAsset->Links = SyncedLinks;
	}

	TArray<FAITCSTacticalGraphComment> SyncedComments;
	for (UEdGraphNode *BaseNode : Nodes)
	{
		UEdGraphNode_Comment *CommentNode = Cast<UEdGraphNode_Comment>(BaseNode);
		if (!CommentNode)
		{
			continue;
		}

		FAITCSTacticalGraphComment Comment;
		if (!CommentNode->NodeGuid.IsValid())
		{
			CommentNode->CreateNewGuid();
		}
		Comment.CommentId = CommentNode->NodeGuid;
		Comment.Text = CommentNode->NodeComment;
		Comment.Position = FVector2D(CommentNode->NodePosX, CommentNode->NodePosY);
		Comment.Size = FVector2D(CommentNode->NodeWidth, CommentNode->NodeHeight);
		Comment.Color = CommentNode->CommentColor;
		SyncedComments.Add(Comment);
	}

	if (!AreCommentArraysEquivalent(GraphAsset->EditorComments, SyncedComments))
	{
		MarkAssetChanged();
		GraphAsset->EditorComments = SyncedComments;
	}

	if (GraphAsset->EnsurePlayerObject())
	{
		MarkAssetChanged();
	}

	if (bChanged && bMarkDirty)
	{
		GraphAsset->MarkPackageDirty();
	}

	return bChanged;
}

UAITCSEditorGraphNode *UAITCSEditorGraph::CreateNodeForObject(const FGuid &ObjectId, bool bSelectNewNode, bool bUserAction)
{
	UAITCSEditorGraphNode *Node = NewObject<UAITCSEditorGraphNode>(this, NAME_None, RF_Transactional);
	Node->ObjectId = ObjectId;

	if (GraphAsset && GraphAsset->FindObject(ObjectId))
	{
		const FVector2D Position = GraphAsset->GetEditorNodePosition(ObjectId);
		Node->NodePosX = static_cast<int32>(Position.X);
		Node->NodePosY = static_cast<int32>(Position.Y);
	}

	AddNode(Node, bUserAction, bSelectNewNode);
	Node->CreateNewGuid();
	Node->AllocateDefaultPins();
	Node->AutowireNewNode(nullptr);
	return Node;
}

UAITCSEditorGraphNode *UAITCSEditorGraph::AddTacticalObjectNode(const FVector2D &GraphPosition, bool bSelectNewNode)
{
	if (!GraphAsset)
	{
		return nullptr;
	}

	const FAITCSTacticalObject Object = GraphAsset->AddTacticalObject(TEXT("Tactical Object"));
	GraphAsset->SetEditorNodePosition(Object.ObjectId, GraphPosition);
	UAITCSEditorGraphNode *Node = CreateNodeForObject(Object.ObjectId, bSelectNewNode);
	NotifyGraphChanged();
	return Node;
}

UAITCSEditorGraphNode *UAITCSEditorGraph::AddSimpleObjectNode(const FVector2D &GraphPosition, bool bSelectNewNode)
{
	if (!GraphAsset)
	{
		return nullptr;
	}

	FName SimpleObjectName = UAITCSTacticalGraphAsset::SimpleObjectNodeName;
	int32 Suffix = 2;
	while (GraphAsset->Objects.ContainsByPredicate([SimpleObjectName](const FAITCSTacticalObject &Object)
	{
		return Object.DisplayName == SimpleObjectName;
	}))
	{
		SimpleObjectName = FName(*FString::Printf(TEXT("%s %d"), *UAITCSTacticalGraphAsset::SimpleObjectNodeName.ToString(), Suffix++));
	}

	FAITCSTacticalObject Object = GraphAsset->AddTacticalObject(SimpleObjectName);
	if (FAITCSTacticalObject *AddedObject = GraphAsset->FindObject(Object.ObjectId))
	{
		AddedObject->bIsSimpleObject = true;
		AddedObject->GroupAssets.Reset();
		AddedObject->ActiveGroupIndex = INDEX_NONE;
	}
	GraphAsset->SetEditorNodePosition(Object.ObjectId, GraphPosition);
	UAITCSEditorGraphNode *Node = CreateNodeForObject(Object.ObjectId, bSelectNewNode);
	NotifyGraphChanged();
	return Node;
}

UAITCSEditorGraphNode *UAITCSEditorGraph::FindNodeByObjectId(const FGuid &ObjectId) const
{
	for (UEdGraphNode *BaseNode : Nodes)
	{
		UAITCSEditorGraphNode *Node = Cast<UAITCSEditorGraphNode>(BaseNode);
		if (Node && Node->ObjectId == ObjectId)
		{
			return Node;
		}
	}

	return nullptr;
}

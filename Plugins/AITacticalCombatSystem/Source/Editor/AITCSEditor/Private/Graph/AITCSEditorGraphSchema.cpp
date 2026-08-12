// Pavel Gornostaev <https://github.com/Pavreally>

#include "Graph/AITCSEditorGraphSchema.h"

#include "Editor/AITCSTacticalGraphAssetEditor.h"
#include "Graph/AITCSEditorGraph.h"
#include "Graph/AITCSEditorGraphNode.h"
#include "Graph/AITCSTacticalGraphAsset.h"
#include "Graph/AITCSEditorConnectionDrawingPolicy.h"
#include "ScopedTransaction.h"

#define LOCTEXT_NAMESPACE "AITCSEditorGraphSchema"

const FName UAITCSEditorGraphSchema::PC_Relationship(TEXT("AITCSRelationship"));

struct FAITCSEditorGraphSchemaAction_NewObject final : public FEdGraphSchemaAction
{
	FAITCSEditorGraphSchemaAction_NewObject()
		: FEdGraphSchemaAction(
			LOCTEXT("AITCSCategory", "AITCS"),
			LOCTEXT("AddTacticalObject", "Tactical Object"),
			LOCTEXT("AddTacticalObjectTooltip", "Adds a tactical object to the topology graph."),
			0)
	{
	}

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override
	{
		UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(ParentGraph);
		if (!EditorGraph)
		{
			return nullptr;
		}

		const FScopedTransaction Transaction(LOCTEXT("AddTacticalObjectContextTransaction", "Add AITCS Tactical Object"));
		UAITCSEditorGraphNode* NewNode = EditorGraph->AddTacticalObjectNode(Location, bSelectNewNode);
		if (FromPin && NewNode)
		{
			UEdGraphPin* TargetPin = FromPin->Direction == EGPD_Output ? NewNode->GetInputPin() : NewNode->GetOutputPin();
			ParentGraph->GetSchema()->TryCreateConnection(FromPin, TargetPin);
		}

		return NewNode;
	}
};

struct FAITCSEditorGraphSchemaAction_NewSimpleObject final : public FEdGraphSchemaAction
{
	FAITCSEditorGraphSchemaAction_NewSimpleObject()
		: FEdGraphSchemaAction(
			LOCTEXT("AITCSCategory", "AITCS"),
			LOCTEXT("AddSimpleObject", "Simple Object"),
			LOCTEXT("AddSimpleObjectTooltip", "Adds a Simple Object free pool for ungrouped NPC settings."),
			0)
	{
	}

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override
	{
		UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(ParentGraph);
		if (!EditorGraph)
		{
			return nullptr;
		}

		const FScopedTransaction Transaction(LOCTEXT("AddSimpleObjectContextTransaction", "Add AITCS Simple Object"));
		UAITCSEditorGraphNode* NewNode = EditorGraph->AddSimpleObjectNode(Location, bSelectNewNode);
		if (FromPin && NewNode)
		{
			UEdGraphPin* TargetPin = FromPin->Direction == EGPD_Output ? NewNode->GetInputPin() : NewNode->GetOutputPin();
			ParentGraph->GetSchema()->TryCreateConnection(FromPin, TargetPin);
		}

		return NewNode;
	}
};

void UAITCSEditorGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	ContextMenuBuilder.AddAction(MakeShared<FAITCSEditorGraphSchemaAction_NewObject>());
	ContextMenuBuilder.AddAction(MakeShared<FAITCSEditorGraphSchemaAction_NewSimpleObject>());

	if (ContextMenuBuilder.SelectedObjects.Num() > 0)
	{
		for (UObject* SelectedNode : ContextMenuBuilder.SelectedObjects)
		{
			if (UAITCSEditorGraphNode* Node = Cast<UAITCSEditorGraphNode>(SelectedNode))
			{
				if (Node->CanUserDeleteNode())
				{
					struct FAITCSEditorGraphSchemaAction_DeleteObject final : public FEdGraphSchemaAction
					{
						FAITCSEditorGraphSchemaAction_DeleteObject(UAITCSEditorGraphNode* InNode)
							: FEdGraphSchemaAction(
								LOCTEXT("AITCSCategory", "AITCS"),
								LOCTEXT("DeleteTacticalObject", "Delete Tactical Object"),
								LOCTEXT("DeleteTacticalObjectTooltip", "Deletes the selected tactical object."),
								0)
							, NodeToDelete(InNode)
						{
						}

						virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override
						{
							(void)ParentGraph;
							(void)FromPin;
							(void)Location;
							(void)bSelectNewNode;

							if (NodeToDelete && NodeToDelete->GetGraph())
							{
								if (UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(NodeToDelete->GetGraph()))
								{
									if (EditorGraph->GraphAsset)
									{
										const FScopedTransaction Transaction(LOCTEXT("DeleteTacticalObjectContextTransaction", "Delete AITCS Tactical Object"));
										bool bHasAnotherNodeWithObjectId = false;
										for (UEdGraphNode* OtherNode : EditorGraph->Nodes)
										{
											if (OtherNode != NodeToDelete)
											{
												if (const UAITCSEditorGraphNode* OtherTacticalNode = Cast<UAITCSEditorGraphNode>(OtherNode))
												{
													bHasAnotherNodeWithObjectId = OtherTacticalNode->ObjectId == NodeToDelete->ObjectId;
													if (bHasAnotherNodeWithObjectId)
													{
														break;
													}
												}
											}
										}

										if (!bHasAnotherNodeWithObjectId)
										{
											EditorGraph->GraphAsset->RemoveTacticalObject(NodeToDelete->ObjectId);
										}
										EditorGraph->RemoveNode(NodeToDelete);
										EditorGraph->SyncAssetFromGraph();
										EditorGraph->NotifyGraphChanged();
									}
								}
							}
							return nullptr;
						}

						UAITCSEditorGraphNode* NodeToDelete = nullptr;
					};

					ContextMenuBuilder.AddAction(MakeShared<FAITCSEditorGraphSchemaAction_DeleteObject>(Node));
					break;
				}
			}
		}
	}
}

const FPinConnectionResponse UAITCSEditorGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (!A || !B)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("InvalidPins", "Invalid pins."));
	}

	if (A->GetOwningNode() == B->GetOwningNode())
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameNode", "Tactical objects cannot link to themselves."));
	}

	if (A->Direction == B->Direction)
	{
		return FPinConnectionResponse(CONNECT_RESPONSE_DISALLOW, LOCTEXT("SameDirection", "Use an output pin and an input pin."));
	}

	return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, LOCTEXT("MakeRelationship", "Create tactical relationship."));
}

bool UAITCSEditorGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	const bool bMadeConnection = Super::TryCreateConnection(A, B);
	if (bMadeConnection)
	{
		if (UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(A ? A->GetOwningNode()->GetGraph() : nullptr))
		{
			EditorGraph->SyncAssetFromGraph();
		}
	}

	return bMadeConnection;
}

FLinearColor UAITCSEditorGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	(void)PinType;
	return FLinearColor(0.35f, 0.7f, 1.0f, 1.0f);
}

FConnectionDrawingPolicy* UAITCSEditorGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const
{
	return new FAITCSConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

#undef LOCTEXT_NAMESPACE

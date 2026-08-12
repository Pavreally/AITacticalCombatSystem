// Pavel Gornostaev <https://github.com/Pavreally>

#include "Graph/AITCSEditorGraphNode.h"

#include "Graph/AITCSEditorGraph.h"
#include "Graph/AITCSEditorGraphSchema.h"
#include "Graph/AITCSTacticalGraphAsset.h"

void UAITCSEditorGraphNode::AllocateDefaultPins()
{
	const UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(GetGraph());

	if (EditorGraph && EditorGraph->GraphAsset && EditorGraph->GraphAsset->IsPlayerObject(ObjectId))
	{
		return;
	}

	FEdGraphPinType PinType;
	PinType.PinCategory = UAITCSEditorGraphSchema::PC_Relationship;

	CreatePin(EGPD_Input, PinType, TEXT("In"));
	CreatePin(EGPD_Output, PinType, TEXT("Out"));
}

FText UAITCSEditorGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	(void)TitleType;

	const UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(GetGraph());
	const UAITCSTacticalGraphAsset* GraphAsset = EditorGraph ? EditorGraph->GraphAsset : nullptr;
	if (const FAITCSTacticalObject* Object = GraphAsset ? GraphAsset->FindObject(ObjectId) : nullptr)
	{
		return FText::FromName(Object->DisplayName);
	}

	return FText::FromString(TEXT("Tactical Object"));
}

FLinearColor UAITCSEditorGraphNode::GetNodeTitleColor() const
{
	const UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(GetGraph());
	const UAITCSTacticalGraphAsset* GraphAsset = EditorGraph ? EditorGraph->GraphAsset : nullptr;
	if (const FAITCSTacticalObject* Object = GraphAsset ? GraphAsset->FindObject(ObjectId) : nullptr)
	{
		return Object->Color;
	}

	return FLinearColor(0.2f, 0.55f, 1.0f, 1.0f);
}

FText UAITCSEditorGraphNode::GetTooltipText() const
{
	return FText::FromString(TEXT("AITCS tactical object. Links describe spatial relationship contracts, not execution flow."));
}

bool UAITCSEditorGraphNode::CanUserDeleteNode() const
{
	const UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(GetGraph());
	const UAITCSTacticalGraphAsset* GraphAsset = EditorGraph ? EditorGraph->GraphAsset : nullptr;
	return GraphAsset ? !GraphAsset->IsPlayerObject(ObjectId) : true;
}

UEdGraphPin* UAITCSEditorGraphNode::GetInputPin() const
{
	return Pins.Num() > 0 ? Pins[0] : nullptr;
}

UEdGraphPin* UAITCSEditorGraphNode::GetOutputPin() const
{
	return Pins.Num() > 1 ? Pins[1] : nullptr;
}

bool UAITCSEditorGraphNode::CanDuplicateNode() const
{
	return !IsPlayerNode();
}

bool UAITCSEditorGraphNode::IsPlayerNode() const
{
	const UAITCSEditorGraph* EditorGraph = Cast<UAITCSEditorGraph>(GetGraph());
	const UAITCSTacticalGraphAsset* GraphAsset = EditorGraph ? EditorGraph->GraphAsset : nullptr;
	return GraphAsset ? GraphAsset->IsPlayerObject(ObjectId) : false;
}

// Pavel Gornostaev <https://github.com/Pavreally>

#include "Graph/AITCSEditorGraphNodeFactory.h"

#include "Graph/AITCSEditorGraphNode.h"
#include "Graph/SAITCSEditorGraphNode.h"

TSharedPtr<SGraphNode> FAITCSEditorGraphNodeFactory::CreateNode(UEdGraphNode* Node) const
{
	if (UAITCSEditorGraphNode* TacticalNode = Cast<UAITCSEditorGraphNode>(Node))
	{
		return SNew(SAITCSEditorGraphNode, TacticalNode);
	}

	return nullptr;
}

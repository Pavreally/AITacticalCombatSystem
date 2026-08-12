#include "Graph/AITCSEditorConnectionDrawingPolicy.h"

#include "Graph/AITCSEditorGraph.h"
#include "Graph/AITCSEditorGraphNode.h"
#include "Graph/AITCSTacticalGraphAsset.h"
#include "Styling/AppStyle.h"
#include "Widgets/SWidget.h"

FAITCSConnectionDrawingPolicy::FAITCSConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect &InClippingRect, FSlateWindowElementList &InDrawElements, UEdGraph *InGraphObj)
		: FConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements), GraphObj(InGraphObj)
{
	// Use a generic arrow/midpoint brush so a glyph is rendered at the spline midpoint
	ArrowImage = FAppStyle::GetBrush(TEXT("Graph.Arrow"));
	MidpointImage = ArrowImage;

	if (MidpointImage)
	{
		MidpointRadius = FVector2f(MidpointImage->ImageSize.X * 0.5f, MidpointImage->ImageSize.Y * 0.5f) * ZoomFactor;
	}
}

void FAITCSConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin *OutputPin, UEdGraphPin *InputPin, FConnectionParams &Params)
{
	FConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

	// If this connection corresponds to the editor-selected link, highlight it
	if (!GraphObj)
	{
		return;
	}

	if (UAITCSEditorGraph *EditorGraph = Cast<UAITCSEditorGraph>(GraphObj))
	{
		if (!EditorGraph->GraphAsset)
		{
			return;
		}

		if (!OutputPin || !InputPin)
		{
			return;
		}

		UEdGraphNode *NodeA = OutputPin->GetOwningNode();
		UEdGraphNode *NodeB = InputPin->GetOwningNode();

		if (!NodeA || !NodeB)
		{
			return;
		}

		UAITCSEditorGraphNode *ANode = Cast<UAITCSEditorGraphNode>(NodeA);
		UAITCSEditorGraphNode *BNode = Cast<UAITCSEditorGraphNode>(NodeB);
		if (!ANode || !BNode)
		{
			return;
		}

		const FAITCSTacticalLink *FoundLink = EditorGraph->GraphAsset->FindLinkBetween(ANode->ObjectId, BNode->ObjectId);
		if (!FoundLink)
		{
			FoundLink = EditorGraph->GraphAsset->FindLinkBetween(BNode->ObjectId, ANode->ObjectId);
		}

		if (FoundLink && EditorGraph->SelectedLinkId.IsValid() && EditorGraph->SelectedLinkId == FoundLink->LinkId)
		{
			Params.WireColor = FLinearColor::Yellow;
			Params.WireThickness = Params.WireThickness * 2.0f;
		}
	}
}

void FAITCSConnectionDrawingPolicy::DrawSplineWithArrow(const FGeometry &StartGeom, const FGeometry &EndGeom, const FConnectionParams &Params)
{
	// Use base drawing (includes midpoint image if set). Additional custom drawing
	// for selected links is handled in DetermineWiringStyle above.
	FConnectionDrawingPolicy::DrawSplineWithArrow(StartGeom, EndGeom, Params);
}

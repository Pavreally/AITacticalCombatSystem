// Simple connection drawing policy for AITCS tactical links
#pragma once

#include "ConnectionDrawingPolicy.h"
#include "Graph/AITCSEditorGraph.h"

class FAITCSConnectionDrawingPolicy : public FConnectionDrawingPolicy
{
public:
	FAITCSConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect &InClippingRect, FSlateWindowElementList &InDrawElements, UEdGraph *InGraphObj);

	virtual void DetermineWiringStyle(UEdGraphPin *OutputPin, UEdGraphPin *InputPin, FConnectionParams &Params) override;
	virtual void DrawSplineWithArrow(const FGeometry &StartGeom, const FGeometry &EndGeom, const FConnectionParams &Params) override;

private:
	UEdGraph *GraphObj = nullptr;
};

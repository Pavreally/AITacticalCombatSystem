// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "Data/AITCSCoreData.h"

class UAITCSEditorGraphNode;

class SAITCSEditorGraphNode : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SAITCSEditorGraphNode) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UAITCSEditorGraphNode* InNode);

	virtual void UpdateGraphNode() override;
	virtual void AddPin(const TSharedRef<SGraphPin>& PinToAdd) override;

private:
	const FAITCSTacticalObject* GetTacticalObject() const;
	EAITCSTacticalObjectShape GetObjectShape() const;
	FLinearColor GetObjectColor() const;

	float GetPreferredDirection() const;

	TSharedPtr<SVerticalBox> LeftNodeBox;
	TSharedPtr<SVerticalBox> RightNodeBox;
};

class SAITCSTacticalShape : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAITCSTacticalShape)
		: _Shape(EAITCSTacticalObjectShape::Circle)
		, _Color(FLinearColor::White)
		, _DesiredSize(FVector2D(96.0f, 72.0f))
	{}
		SLATE_ARGUMENT(EAITCSTacticalObjectShape, Shape)
		SLATE_ARGUMENT(FLinearColor, Color)
		SLATE_ARGUMENT(FVector2D, DesiredSize)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	void BuildShapePoints(const FGeometry& AllottedGeometry, TArray<FVector2D>& OutPoints) const;

	EAITCSTacticalObjectShape Shape = EAITCSTacticalObjectShape::Circle;
	FLinearColor Color = FLinearColor::White;
	FVector2D DesiredSize = FVector2D(96.0f, 72.0f);
};

class SAITCSTacticalDirection : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SAITCSTacticalDirection)
		: _Angle(0.f)
		, _DesiredSize(FVector2D(36.0f, 36.0f))
		, _CircleColor(FLinearColor::Gray)
		, _PointColor(FLinearColor::Yellow)
	{}

		SLATE_ATTRIBUTE(float, Angle)
		SLATE_ARGUMENT(FVector2D, DesiredSize)
		SLATE_ARGUMENT(FLinearColor, CircleColor)
		SLATE_ARGUMENT(FLinearColor, PointColor)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	virtual FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override;

private:
	TAttribute<float> Angle;
	FVector2D DesiredSize = FVector2D(36.0f, 36.0f);
	FLinearColor CircleColor = FLinearColor::Gray;
	FLinearColor PointColor = FLinearColor::Yellow;
};

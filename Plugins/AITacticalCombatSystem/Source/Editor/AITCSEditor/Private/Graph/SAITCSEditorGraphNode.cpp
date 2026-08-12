// Pavel Gornostaev <https://github.com/Pavreally>

#include "Graph/SAITCSEditorGraphNode.h"

#include "Graph/AITCSEditorGraph.h"
#include "Graph/AITCSEditorGraphNode.h"
#include "Graph/AITCSTacticalGraphAsset.h"
#include "SGraphPin.h"
#include "Styling/AppStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

void SAITCSEditorGraphNode::Construct(const FArguments& InArgs, UAITCSEditorGraphNode* InNode)
{
	(void)InArgs;

	GraphNode = InNode;
	SetCursor(EMouseCursor::CardinalCross);
	UpdateGraphNode();
}

void SAITCSEditorGraphNode::UpdateGraphNode()
{
	InputPins.Empty();
	OutputPins.Empty();
	LeftNodeBox.Reset();
	RightNodeBox.Reset();

	const FText NodeTitle = GraphNode ? GraphNode->GetNodeTitle(ENodeTitleType::FullTitle) : FText::GetEmpty();
	const FVector2D ShapeSize = GetObjectShape() == EAITCSTacticalObjectShape::Rectangle
		? FVector2D(128.0f, 64.0f)
		: FVector2D(92.0f, 92.0f);

	this->ContentScale.Bind(this, &SGraphNode::GetContentScale);

	GetOrAddSlot(ENodeZone::Center)
	.HAlign(HAlign_Center)
	.VAlign(VAlign_Center)
	[
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("NoBorder"))
		.Padding(FMargin(6.0f))
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(LeftNodeBox, SVerticalBox)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(6.0f, 0.0f)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
					[
						SNew(SVerticalBox)
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(32.0f)
							.HeightOverride(32.0f)
							[
								SNew(SAITCSTacticalDirection)
								.Angle(this, &SAITCSEditorGraphNode::GetPreferredDirection)
								.DesiredSize(FVector2D(32.0f, 32.0f))
								.CircleColor(FLinearColor::Gray)
								.PointColor(FLinearColor::Yellow)
							]
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(SBox)
							.WidthOverride(ShapeSize.X)
							.HeightOverride(ShapeSize.Y)
							[
								SNew(SAITCSTacticalShape)
								.Shape(GetObjectShape())
								.Color(GetObjectColor())
								.DesiredSize(ShapeSize)
							]
						]
					]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.Padding(0.0f, 6.0f, 0.0f, 0.0f)
				[
					SNew(STextBlock)
					.Text(NodeTitle)
					.Justification(ETextJustify::Center)
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SAssignNew(RightNodeBox, SVerticalBox)
			]
		]
	];

	CreatePinWidgets();

	// NOTE: clickable link markers are intentionally not added to node widgets.
	// Link widgets should be anchored to the connection itself (between nodes),
	// not rendered as child widgets of object nodes. The previous implementation
	// added SButton widgets into the node's RightNodeBox which incorrectly
	// anchored link controls to nodes; this block has been removed.
}

void SAITCSEditorGraphNode::AddPin(const TSharedRef<SGraphPin>& PinToAdd)
{
	PinToAdd->SetOwner(SharedThis(this));

	if (PinToAdd->GetDirection() == EGPD_Input)
	{
		LeftNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(0.0f, 2.0f)
		[
			PinToAdd
		];
		InputPins.Add(PinToAdd);
	}
	else
	{
		RightNodeBox->AddSlot()
		.AutoHeight()
		.HAlign(HAlign_Right)
		.VAlign(VAlign_Center)
		.Padding(0.0f, 2.0f)
		[
			PinToAdd
		];
		OutputPins.Add(PinToAdd);
	}
}

const FAITCSTacticalObject* SAITCSEditorGraphNode::GetTacticalObject() const
{
	const UAITCSEditorGraphNode* TypedNode = Cast<UAITCSEditorGraphNode>(GraphNode);
	const UAITCSEditorGraph* EditorGraph = TypedNode ? Cast<UAITCSEditorGraph>(TypedNode->GetGraph()) : nullptr;
	const UAITCSTacticalGraphAsset* GraphAsset = EditorGraph ? EditorGraph->GraphAsset : nullptr;
	return (TypedNode && GraphAsset) ? GraphAsset->FindObject(TypedNode->ObjectId) : nullptr;
}

EAITCSTacticalObjectShape SAITCSEditorGraphNode::GetObjectShape() const
{
	if (const FAITCSTacticalObject* Object = GetTacticalObject())
	{
		return Object->Shape;
	}

	return EAITCSTacticalObjectShape::Circle;
}

FLinearColor SAITCSEditorGraphNode::GetObjectColor() const
{
	if (const FAITCSTacticalObject* Object = GetTacticalObject())
	{
		return Object->Color;
	}

	return FLinearColor(0.2f, 0.55f, 1.0f, 1.0f);
}

void SAITCSTacticalShape::Construct(const FArguments& InArgs)
{
	Shape = InArgs._Shape;
	Color = InArgs._Color;
	DesiredSize = InArgs._DesiredSize;
}

int32 SAITCSTacticalShape::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	(void)Args;
	(void)MyCullingRect;
	(void)bParentEnabled;

	TArray<FVector2D> Points;
	BuildShapePoints(AllottedGeometry, Points);

	const FLinearColor FinalColor = InWidgetStyle.GetColorAndOpacityTint() * Color;
	if (Points.Num() > 1)
	{
		const FVector2D FirstPoint = Points[0];
		Points.Add(FirstPoint);
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			LayerId + 1,
			AllottedGeometry.ToPaintGeometry(),
			Points,
			ESlateDrawEffect::None,
			FinalColor,
			true,
			3.0f
		);
	}

	return LayerId + 2;
}

FVector2D SAITCSTacticalShape::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	(void)LayoutScaleMultiplier;
	return DesiredSize;
}

void SAITCSTacticalShape::BuildShapePoints(const FGeometry& AllottedGeometry, TArray<FVector2D>& OutPoints) const
{
	const FVector2D Size = AllottedGeometry.GetLocalSize();
	const FVector2D Center = Size * 0.5f;
	const float Padding = 8.0f;
	const float Left = Padding;
	const float Right = FMath::Max(Padding, Size.X - Padding);
	const float Top = Padding;
	const float Bottom = FMath::Max(Padding, Size.Y - Padding);
	const float Radius = FMath::Max(4.0f, FMath::Min(Size.X, Size.Y) * 0.5f - Padding);

	switch (Shape)
	{
	case EAITCSTacticalObjectShape::Circle:
		for (int32 Index = 0; Index < 40; ++Index)
		{
			const float Angle = 2.0f * PI * static_cast<float>(Index) / 40.0f;
			OutPoints.Add(Center + FVector2D(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius));
		}
		break;

	case EAITCSTacticalObjectShape::Square:
		OutPoints.Add(FVector2D(Center.X - Radius, Center.Y - Radius));
		OutPoints.Add(FVector2D(Center.X + Radius, Center.Y - Radius));
		OutPoints.Add(FVector2D(Center.X + Radius, Center.Y + Radius));
		OutPoints.Add(FVector2D(Center.X - Radius, Center.Y + Radius));
		break;

	case EAITCSTacticalObjectShape::Diamond:
		OutPoints.Add(FVector2D(Center.X, Top));
		OutPoints.Add(FVector2D(Right, Center.Y));
		OutPoints.Add(FVector2D(Center.X, Bottom));
		OutPoints.Add(FVector2D(Left, Center.Y));
		break;

	case EAITCSTacticalObjectShape::Triangle:
		OutPoints.Add(FVector2D(Center.X, Top));
		OutPoints.Add(FVector2D(Right, Bottom));
		OutPoints.Add(FVector2D(Left, Bottom));
		break;

	case EAITCSTacticalObjectShape::Rectangle:
	default:
		OutPoints.Add(FVector2D(Left, Top));
		OutPoints.Add(FVector2D(Right, Top));
		OutPoints.Add(FVector2D(Right, Bottom));
		OutPoints.Add(FVector2D(Left, Bottom));
		break;
	}
}

void SAITCSTacticalDirection::Construct(const FArguments& InArgs)
{
	Angle = InArgs._Angle;
	DesiredSize = InArgs._DesiredSize;
	CircleColor = InArgs._CircleColor;
	PointColor = InArgs._PointColor;
}

int32 SAITCSTacticalDirection::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	(void)Args;
	(void)MyCullingRect;
	(void)bParentEnabled;

	const FVector2D SizeLocal = AllottedGeometry.GetLocalSize();
	const FVector2D Center = SizeLocal * 0.5f;
	const float Radius = FMath::Max(0.f, FMath::Min(Center.X, Center.Y) - 2.0f);

	// draw circle
	const int32 Segments = 32;
	TArray<FVector2D> CirclePts;
	CirclePts.Reserve(Segments + 1);
	for (int32 i = 0; i <= Segments; ++i)
	{
		const float Theta = 2.f * PI * float(i) / float(Segments);
		CirclePts.Add(Center + FVector2D(FMath::Cos(Theta), FMath::Sin(Theta)) * Radius);
	}

	const FLinearColor FinalCircleColor = InWidgetStyle.GetColorAndOpacityTint() * CircleColor;
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 1, AllottedGeometry.ToPaintGeometry(), CirclePts, ESlateDrawEffect::None, FinalCircleColor, true, 1.5f);

	// direction line and dot
	const float AngleDeg = Angle.Get();
	const float Rad = FMath::DegreesToRadians(AngleDeg);
	const FVector2D Dir = FVector2D(FMath::Sin(Rad), FMath::Cos(Rad)); // 0° -> down
	const FVector2D DotPos = Center + Dir * (Radius - 4.0f);

	TArray<FVector2D> LinePts;
	LinePts.Add(Center);
	LinePts.Add(DotPos);
	const FLinearColor FinalPointColor = InWidgetStyle.GetColorAndOpacityTint() * PointColor;
	FSlateDrawElement::MakeLines(OutDrawElements, LayerId + 2, AllottedGeometry.ToPaintGeometry(), LinePts, ESlateDrawEffect::None, FinalPointColor, true, 1.75f);

	// no filled dot; only the radial line is drawn
	return LayerId + 3;
}

FVector2D SAITCSTacticalDirection::ComputeDesiredSize(float LayoutScaleMultiplier) const
{
	(void)LayoutScaleMultiplier;
	return DesiredSize;
}

float SAITCSEditorGraphNode::GetPreferredDirection() const
{
	if (const FAITCSTacticalObject* Object = GetTacticalObject())
	{
		return Object->PreferredDirectionDegrees;
	}

	return 0.0f;
}

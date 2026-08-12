// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"

#include "AITCSEditorGraphNode.generated.h"

UCLASS()
class UAITCSEditorGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FGuid ObjectId;

	virtual void AllocateDefaultPins() override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual FLinearColor GetNodeTitleColor() const override;
	virtual FText GetTooltipText() const override;
	virtual bool CanUserDeleteNode() const override;
	virtual bool CanDuplicateNode() const override;

	UEdGraphPin* GetInputPin() const;
	UEdGraphPin* GetOutputPin() const;
	bool IsPlayerNode() const;
};

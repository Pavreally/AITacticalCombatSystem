// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Data/AITCSCoreData.h"

#include "AITCSEditorDetailsProxy.generated.h"

class UAITCSTacticalGraphAsset;
class UEdGraphNode_Comment;

UCLASS()
class UAITCSEditorObjectDetailsProxy : public UObject
{
	GENERATED_BODY()

public:
	void LoadFromObject(const FAITCSTacticalObject& InObject);
	bool ApplyToGraphAsset(UAITCSTacticalGraphAsset* GraphAsset) const;

	UPROPERTY(EditAnywhere, Category = "AITCS|Object")
	FAITCSTacticalObject ObjectData;

private:
	FGuid SourceObjectId;
};

UCLASS()
class UAITCSEditorLinkDetailsProxy : public UObject
{
	GENERATED_BODY()

public:
	void LoadFromLink(const FAITCSTacticalLink& InLink, const UAITCSTacticalGraphAsset* GraphAsset);
	bool ApplyToGraphAsset(UAITCSTacticalGraphAsset* GraphAsset) const;

	UPROPERTY(VisibleAnywhere, Category = "AITCS|Link Context")
	FText ParentObject;

	UPROPERTY(VisibleAnywhere, Category = "AITCS|Link Context")
	FText ChildObject;

	UPROPERTY(EditAnywhere, Category = "AITCS|Link")
	FAITCSTacticalLink LinkData;

private:
	FGuid SourceLinkId;
};

UCLASS()
class UAITCSEditorCommentDetailsProxy : public UObject
{
	GENERATED_BODY()

public:
	void LoadFromCommentNode(const UEdGraphNode_Comment* InCommentNode);
	bool ApplyToCommentNode(UEdGraphNode_Comment* CommentNode) const;

	UPROPERTY(EditAnywhere, Category = "AITCS|Comment", meta=(MultiLine=true))
	FText CommentText;

	UPROPERTY(EditAnywhere, Category = "AITCS|Comment")
	FLinearColor CommentColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category = "AITCS|Comment", meta=(MultiLine=true))
	FText NodeDetails;

private:
	FGuid SourceNodeGuid;
};

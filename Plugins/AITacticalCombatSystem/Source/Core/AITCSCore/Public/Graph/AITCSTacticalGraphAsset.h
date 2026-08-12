// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/AITCSCoreData.h"

#include "AITCSTacticalGraphAsset.generated.h"

/** Position metadata used by the editor to remember where a graph node was placed. */
USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalGraphNodePlacement
{
	GENERATED_BODY()

	/** Graph object identifier this placement belongs to. */
	UPROPERTY()
	FGuid ObjectId;

	/** Node position in editor canvas coordinates. */
	UPROPERTY()
	FVector2D Position = FVector2D::ZeroVector;
};

/** Free-form comment data displayed on the tactical graph editor canvas. */
USTRUCT(BlueprintType)
struct AITCSCORE_API FAITCSTacticalGraphComment
{
	GENERATED_BODY()

	/** Unique identifier of the comment block. */
	UPROPERTY()
	FGuid CommentId;

	/** Text content shown in the comment area. */
	UPROPERTY()
	FString Text;

	/** Top-left position of the comment block in editor canvas coordinates. */
	UPROPERTY()
	FVector2D Position = FVector2D::ZeroVector;

	/** Size of the comment block in editor canvas coordinates. */
	UPROPERTY()
	FVector2D Size = FVector2D(400.0f, 220.0f);

	/** Background color used to highlight the comment block. */
	UPROPERTY()
	FLinearColor Color = FLinearColor(1.0f, 1.0f, 1.0f, 0.35f);
};

/** Asset container for the tactical graph data used by AITCS at edit time and runtime. */
UCLASS(BlueprintType)
class AITCSCORE_API UAITCSTacticalGraphAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	static const FName PlayerObjectName;
	static const FName SimpleObjectNodeName;
	static const FName OtherObjectsNodeName;

	UAITCSTacticalGraphAsset();

	/** Resolve or create the player object node in this graph. */
	virtual void PostLoad() override;

#if WITH_EDITOR
	/** Handle property changes in the editor to keep graph state consistent. */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	/** Ensure the graph contains a valid player object. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	bool EnsurePlayerObject();

	/** Add a new tactical object with the specified display name. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	FAITCSTacticalObject AddTacticalObject(FName DisplayName);

	/** Remove a tactical object by its runtime identifier. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	bool RemoveTacticalObject(const FGuid& ObjectId);

	/** Add a new link between two tactical objects. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	FAITCSTacticalLink AddTacticalLink(const FGuid& SourceObjectId, const FGuid& TargetObjectId);

	/** Remove a tactical link by its runtime identifier. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	bool RemoveTacticalLink(const FGuid& LinkId);

	/** Query a tactical object by its identifier. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	bool TryGetTacticalObject(const FGuid& ObjectId, FAITCSTacticalObject& OutObject) const;

	/** Query a tactical link by its identifier. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	bool TryGetTacticalLink(const FGuid& LinkId, FAITCSTacticalLink& OutLink) const;

	/** Validate the asset data and collect any graph warnings or errors. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	void ValidateGraph(TArray<FAITCSGraphValidationMessage>& OutMessages) const;

	/** Check whether the graph is currently valid for compilation. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Graph")
	bool IsValidGraph() const;

	/** Ensure object and link IDs are unique; used by editor tools. */
	void DeduplicateObjectIds();

	/** Check whether a specific object is the designated player object. */
	bool IsPlayerObject(const FGuid& ObjectId) const;

	/** Get the node placement for a specific object ID. */
	FVector2D GetEditorNodePosition(const FGuid& ObjectId) const;

	/** Persist the node placement for a specific object. */
	void SetEditorNodePosition(const FGuid& ObjectId, const FVector2D& Position);

	/** Remove saved editor placement data for the specified object. */
	void RemoveEditorNodePosition(const FGuid& ObjectId);

	/** Gather the group assets referenced from this graph for load-time compilation. */
	TArray<TSoftObjectPtr<UAITCSTacticalGroupDataAsset>> CollectReferencedGroupAssets() const;

	/** Find a tactical object instance by identifier. */
	FAITCSTacticalObject* FindObject(const FGuid& ObjectId);

	/** Find a const tactical object instance by identifier. */
	const FAITCSTacticalObject* FindObject(const FGuid& ObjectId) const;

	/** Find the first simple object configuration, if any. */
	FAITCSTacticalObject* FindSimpleObjectConfig();

	/** Find the first simple object configuration, if any (const). */
	const FAITCSTacticalObject* FindSimpleObjectConfig() const;

	/** Check whether a simple object configuration exists in the graph. */
	bool HasSimpleObjectConfig() const;

	/** Find a tactical link instance by identifier. */
	FAITCSTacticalLink* FindLink(const FGuid& LinkId);

	/** Find a const tactical link instance by identifier. */
	const FAITCSTacticalLink* FindLink(const FGuid& LinkId) const;

	/** Find a direct link between two object identifiers. */
	const FAITCSTacticalLink* FindLinkBetween(const FGuid& SourceObjectId, const FGuid& TargetObjectId) const;

	/** Tactical objects authored in the graph asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Graph")
	TArray<FAITCSTacticalObject> Objects;

	/** Tactical links authored in the graph asset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Graph")
	TArray<FAITCSTacticalLink> Links;

	/** Identifier of the player object within the graph. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AITCS|Graph")
	FGuid PlayerObjectId;

	/** Saved editor positions for node placement. */
	UPROPERTY()
	TArray<FAITCSTacticalGraphNodePlacement> EditorNodePlacements;

	/** Saved editor comments attached to the tactical graph. */
	UPROPERTY()
	TArray<FAITCSTacticalGraphComment> EditorComments;

private:
	int32 FindObjectIndex(const FGuid& ObjectId) const;
	int32 FindLinkIndex(const FGuid& LinkId) const;
	bool HasObject(const FGuid& ObjectId) const;
	void DeduplicateIds();
};

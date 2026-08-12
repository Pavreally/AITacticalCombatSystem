// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "Delegates/Delegate.h"
#include "Graph/AITCSTacticalGraphAsset.h"

#include "AITCSEditorGraph.generated.h"

class UAITCSEditorGraphNode;

UCLASS()
class UAITCSEditorGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UAITCSTacticalGraphAsset> GraphAsset = nullptr;

	// Editor-only: currently selected link id in the graph UI
	FGuid SelectedLinkId;

	// Broadcast when editor-side link selection changes (editor-only)
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnSelectedLinkChanged, const FGuid& /*LinkId*/);
	FOnSelectedLinkChanged OnSelectedLinkChanged;

	void RebuildGraphFromAsset();
	bool SyncAssetFromGraph(bool bMarkDirty = true);
	UAITCSEditorGraphNode* CreateNodeForObject(const FGuid& ObjectId, bool bSelectNewNode, bool bUserAction = true);
	UAITCSEditorGraphNode* AddTacticalObjectNode(const FVector2D& GraphPosition, bool bSelectNewNode);
	UAITCSEditorGraphNode* AddSimpleObjectNode(const FVector2D& GraphPosition, bool bSelectNewNode);
	UAITCSEditorGraphNode* FindNodeByObjectId(const FGuid& ObjectId) const;
};

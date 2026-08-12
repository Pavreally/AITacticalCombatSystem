// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Data/AITCSCoreData.h"
#include "Framework/Commands/InputChord.h"
#include "Toolkits/AssetEditorToolkit.h"
#include "UObject/GCObject.h"
#include "Widgets/Views/SListView.h"
#include "GraphEditor.h"

class IDetailsView;
class FUICommandList;
class SGraphEditor;
class STableViewBase;
class UAITCSEditorGraph;
class UAITCSEditorGraphNode;
class UAITCSEditorLinkDetailsProxy;
class UAITCSEditorObjectDetailsProxy;
class UAITCSEditorCommentDetailsProxy;
class UAITCSTacticalGraphAsset;
class UEdGraphNode_Comment;

class FAITCSTacticalGraphAssetEditor : public FAssetEditorToolkit, public FGCObject
{
public:
	void InitTacticalGraphAssetEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UAITCSTacticalGraphAsset* InGraphAsset);

	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FString GetWorldCentricTabPrefix() const override;
	virtual FLinearColor GetWorldCentricTabColorScale() const override;
	virtual void RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager) override;
	virtual void SaveAsset_Execute() override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override;

private:
	static const FName GraphTabId;
	static const FName DetailsTabId;
	static const FName LinksTabId;

	TSharedRef<SDockTab> SpawnGraphTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnDetailsTab(const FSpawnTabArgs& Args);
	TSharedRef<SDockTab> SpawnLinksTab(const FSpawnTabArgs& Args);
	TSharedRef<SWidget> CreateGraphEditorWidget();
	TSharedRef<SWidget> CreateDetailsWidget();
	TSharedRef<SWidget> CreateLinksWidget();
	void ExtendToolbar();
	void BindGraphCommands();

	void AddTacticalObject();
	void DeleteSelectedObject();
	void DuplicateSelectedObject();
	void CreateCommentForSelection();
	void ValidateGraph();
	void HandleGraphChanged(const FEdGraphEditAction& Action);
	void HandleGraphSelectionChanged(const TSet<UObject*>& SelectedObjects);
	FReply HandleSpawnNodeByShortcut(FInputChord InputChord, const FVector2f& GraphLocation);
	void HandleObjectDetailsChanged(const FPropertyChangedEvent& PropertyChangedEvent);
	void HandleLinkDetailsChanged(const FPropertyChangedEvent& PropertyChangedEvent);
	FReply HandleGraphPanelMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent);
	void HandleGraphLinkSelected(const FGuid& LinkId);

	// Copy / Paste support
	void CopySelection();
	void PasteSelection();


	void SelectObjectNode(UAITCSEditorGraphNode* Node);
	void RebuildSelectedLinks(const FGuid& ObjectId);
	TSharedRef<ITableRow> GenerateLinkRow(TSharedPtr<FGuid> LinkId, const TSharedRef<STableViewBase>& OwnerTable);
	void HandleLinkSelectionChanged(TSharedPtr<FGuid> LinkId, ESelectInfo::Type SelectInfo);
	FText GetValidationText() const;
	FText GetValidateButtonLabel() const;
	FText GetValidateButtonToolTip() const;

	// Toolbar widgets
	TSharedRef<SWidget> CreateValidateToolbarWidget();
	FSlateColor GetValidateButtonBackgroundColor() const;
	const FSlateBrush* GetValidateButtonIcon() const;
	FSlateColor GetValidateIconTint() const;
	FText GetValidateOverlayText() const;

	TObjectPtr<UAITCSTacticalGraphAsset> GraphAsset = nullptr;
	TObjectPtr<UAITCSEditorGraph> EditorGraph = nullptr;
	TObjectPtr<UAITCSEditorObjectDetailsProxy> ObjectProxy = nullptr;
	TObjectPtr<UAITCSEditorLinkDetailsProxy> LinkProxy = nullptr;
	TObjectPtr<UAITCSEditorCommentDetailsProxy> CommentProxy = nullptr;

	TSharedPtr<SGraphEditor> GraphEditorWidget;
	TSharedPtr<FUICommandList> GraphEditorCommands;
	TSharedPtr<IDetailsView> ObjectDetailsView;
	TSharedPtr<IDetailsView> LinkDetailsView;
	TSharedPtr<SListView<TSharedPtr<FGuid>>> LinkListView;
	TArray<TSharedPtr<FGuid>> LinkItems;
	TArray<FText> LastValidationMessages;
	FDelegateHandle GraphChangedDelegateHandle;
	bool bObjectProxyActive = false;
	bool bLinkProxyActive = false;
	bool bValidationHasRun = false;
	bool bLastValidationHadErrors = false;
	bool bCommentActive = false;
	TWeakObjectPtr<UEdGraphNode_Comment> SelectedCommentNode;

	// Copy buffer (store indices for objects and lightweight comment copies)
	TArray<int32> CopiedObjectIndices;
	struct FCopiedComment
	{
		FString Text;
		FVector2D Size = FVector2D::ZeroVector;
		FLinearColor Color = FLinearColor::White;
		FVector2D Position = FVector2D::ZeroVector;
	};
	TArray<FCopiedComment> CopiedComments;
	FVector2D CopiedCenter = FVector2D::ZeroVector;
	TArray<FVector2D> CopiedObjectPositions;
	TArray<FVector2D> CopiedCommentPositions;

	// Last graph click location used for paste positioning
	FVector2D LastGraphClickLocation = FVector2D::ZeroVector;
	bool bHasLastGraphClickLocation = false;

	void RequestRenameSelection();
	void HandleNodeTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* Node);
};



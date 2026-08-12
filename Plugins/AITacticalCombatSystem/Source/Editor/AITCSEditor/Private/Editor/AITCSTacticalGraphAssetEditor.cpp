// Pavel Gornostaev <https://github.com/Pavreally>

#include "Editor/AITCSTacticalGraphAssetEditor.h"

#include "Details/AITCSEditorDetailsProxy.h"
#include "Graph/AITCSEditorGraph.h"
#include "Graph/AITCSEditorGraphNode.h"
#include "Graph/AITCSEditorGraphSchema.h"
#include "Graph/AITCSTacticalGraphAsset.h"

#include "EdGraph/EdGraph.h"
#include "EdGraphNode_Comment.h"
#include "EdGraph/EdGraphSchema.h"
#include "Framework/Commands/GenericCommands.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "GraphEditorActions.h"
#include "GraphEditor.h"
#include "SGraphPanel.h"
#include "IDetailsView.h"
#include "InputCoreTypes.h"
#include "Framework/Application/SlateApplication.h"
#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"
#include "ScopedTransaction.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/SOverlay.h"
#include "EditorStyleSet.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"
#include "Brushes/SlateImageBrush.h"

#define LOCTEXT_NAMESPACE "AITCSTacticalGraphAssetEditor"

const FName FAITCSTacticalGraphAssetEditor::GraphTabId(TEXT("AITCS_TacticalGraph_Graph"));
const FName FAITCSTacticalGraphAssetEditor::DetailsTabId(TEXT("AITCS_TacticalGraph_Details"));
const FName FAITCSTacticalGraphAssetEditor::LinksTabId(TEXT("AITCS_TacticalGraph_Links"));

void FAITCSTacticalGraphAssetEditor::InitTacticalGraphAssetEditor(EToolkitMode::Type Mode, const TSharedPtr<IToolkitHost>& InitToolkitHost, UAITCSTacticalGraphAsset* InGraphAsset)
{
	GraphAsset = InGraphAsset;
	check(GraphAsset);

	EditorGraph = NewObject<UAITCSEditorGraph>(GetTransientPackage(), NAME_None, RF_Transactional | RF_Transient);
	EditorGraph->Schema = UAITCSEditorGraphSchema::StaticClass();
	EditorGraph->GraphAsset = GraphAsset;
	EditorGraph->RebuildGraphFromAsset();
	GraphChangedDelegateHandle = EditorGraph->AddOnGraphChangedHandler(FOnGraphChanged::FDelegate::CreateSP(this, &FAITCSTacticalGraphAssetEditor::HandleGraphChanged));
	// Bind to editor-only link selection notifications coming from nodes
	EditorGraph->OnSelectedLinkChanged.AddRaw(this, &FAITCSTacticalGraphAssetEditor::HandleGraphLinkSelected);
	BindGraphCommands();

	ObjectProxy = NewObject<UAITCSEditorObjectDetailsProxy>(GetTransientPackage(), NAME_None, RF_Transactional);
	LinkProxy = NewObject<UAITCSEditorLinkDetailsProxy>(GetTransientPackage(), NAME_None, RF_Transactional);
	CommentProxy = NewObject<UAITCSEditorCommentDetailsProxy>(GetTransientPackage(), NAME_None, RF_Transactional);

	FPropertyEditorModule& PropertyEditorModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs DetailsArgs;
	DetailsArgs.bAllowSearch = true;
	DetailsArgs.bHideSelectionTip = true;

	ObjectDetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
	ObjectDetailsView->OnFinishedChangingProperties().AddSP(this, &FAITCSTacticalGraphAssetEditor::HandleObjectDetailsChanged);
	ObjectDetailsView->SetObject(nullptr);
	bObjectProxyActive = false;

	LinkDetailsView = PropertyEditorModule.CreateDetailView(DetailsArgs);
	LinkDetailsView->OnFinishedChangingProperties().AddSP(this, &FAITCSTacticalGraphAssetEditor::HandleLinkDetailsChanged);
	LinkDetailsView->SetObject(nullptr);
	bLinkProxyActive = false;

	// Create link list view (used in Links tab)
	LinkListView = SNew(SListView<TSharedPtr<FGuid>>)
		.ListItemsSource(&LinkItems)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &FAITCSTacticalGraphAssetEditor::GenerateLinkRow)
		.OnSelectionChanged(this, &FAITCSTacticalGraphAssetEditor::HandleLinkSelectionChanged);

	const TSharedRef<FTabManager::FLayout> Layout = FTabManager::NewLayout("AITCS_TacticalGraphAssetEditor_Layout_v3")
		->AddArea
		(
			FTabManager::NewPrimaryArea()->SetOrientation(Orient_Horizontal)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.72f)
				->AddTab(GraphTabId, ETabState::OpenedTab)
			)
			->Split
			(
				FTabManager::NewStack()
				->SetSizeCoefficient(0.28f)
				->AddTab(DetailsTabId, ETabState::OpenedTab)
				->AddTab(LinksTabId, ETabState::OpenedTab)
			)
		);

	InitAssetEditor(Mode, InitToolkitHost, TEXT("AITCSTacticalGraphAssetEditor"), Layout, true, true, GraphAsset);
	// Temporarily consume Ctrl+Z/Ctrl+Y. The graph is a transient projection of
	// the asset, and its transaction restoration needs a dedicated redesign.
	GetToolkitCommands()->MapAction(
		FGenericCommands::Get().Undo,
		FExecuteAction::CreateLambda([]() {}));
	GetToolkitCommands()->MapAction(
		FGenericCommands::Get().Redo,
		FExecuteAction::CreateLambda([]() {}));
	ExtendToolbar();
	RegenerateMenusAndToolbars();
}

FName FAITCSTacticalGraphAssetEditor::GetToolkitFName() const
{
	return FName(TEXT("AITCSTacticalGraphAssetEditor"));
}

FText FAITCSTacticalGraphAssetEditor::GetBaseToolkitName() const
{
	return LOCTEXT("ToolkitName", "AITCS Tactical Graph");
}

FString FAITCSTacticalGraphAssetEditor::GetWorldCentricTabPrefix() const
{
	return TEXT("AITCS Tactical Graph");
}

FLinearColor FAITCSTacticalGraphAssetEditor::GetWorldCentricTabColorScale() const
{
	return FLinearColor(0.08f, 0.28f, 0.45f, 1.0f);
}

void FAITCSTacticalGraphAssetEditor::RegisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	WorkspaceMenuCategory = InTabManager->AddLocalWorkspaceMenuCategory(LOCTEXT("WorkspaceCategory", "AITCS"));
	const TSharedRef<FWorkspaceItem> WorkspaceCategoryRef = WorkspaceMenuCategory.ToSharedRef();

	FAssetEditorToolkit::RegisterTabSpawners(InTabManager);

	InTabManager->RegisterTabSpawner(GraphTabId, FOnSpawnTab::CreateSP(this, &FAITCSTacticalGraphAssetEditor::SpawnGraphTab))
		.SetDisplayName(LOCTEXT("GraphTab", "Graph"))
		.SetGroup(WorkspaceCategoryRef);

	InTabManager->RegisterTabSpawner(DetailsTabId, FOnSpawnTab::CreateSP(this, &FAITCSTacticalGraphAssetEditor::SpawnDetailsTab))
		.SetDisplayName(LOCTEXT("DetailsTab", "Details"))
		.SetGroup(WorkspaceCategoryRef);

	InTabManager->RegisterTabSpawner(LinksTabId, FOnSpawnTab::CreateSP(this, &FAITCSTacticalGraphAssetEditor::SpawnLinksTab))
		.SetDisplayName(LOCTEXT("LinksTab", "Links"))
		.SetGroup(WorkspaceCategoryRef);
}

void FAITCSTacticalGraphAssetEditor::UnregisterTabSpawners(const TSharedRef<FTabManager>& InTabManager)
{
	FAssetEditorToolkit::UnregisterTabSpawners(InTabManager);
	InTabManager->UnregisterTabSpawner(GraphTabId);
	InTabManager->UnregisterTabSpawner(DetailsTabId);
	InTabManager->UnregisterTabSpawner(LinksTabId);
}

void FAITCSTacticalGraphAssetEditor::SaveAsset_Execute()
{
	if (EditorGraph)
	{
		EditorGraph->SyncAssetFromGraph();
	}

	if (GraphAsset)
	{
		if (GraphAsset->EnsurePlayerObject())
		{
			GraphAsset->MarkPackageDirty();
		}
	}

	FAssetEditorToolkit::SaveAsset_Execute();
}

void FAITCSTacticalGraphAssetEditor::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(GraphAsset);
	Collector.AddReferencedObject(EditorGraph);
	Collector.AddReferencedObject(ObjectProxy);
	Collector.AddReferencedObject(LinkProxy);
	Collector.AddReferencedObject(CommentProxy);
}

FString FAITCSTacticalGraphAssetEditor::GetReferencerName() const
{
	return TEXT("FAITCSTacticalGraphAssetEditor");
}

TSharedRef<SDockTab> FAITCSTacticalGraphAssetEditor::SpawnGraphTab(const FSpawnTabArgs& Args)
{
	(void)Args;

	return SNew(SDockTab)
		.Label(LOCTEXT("GraphTabLabel", "Graph"))
		[
			CreateGraphEditorWidget()
		];
}

TSharedRef<SDockTab> FAITCSTacticalGraphAssetEditor::SpawnDetailsTab(const FSpawnTabArgs& Args)
{
	(void)Args;

	return SNew(SDockTab)
		.Label(LOCTEXT("DetailsTabLabel", "Details"))
		[
			CreateDetailsWidget()
		];
}

TSharedRef<SWidget> FAITCSTacticalGraphAssetEditor::CreateGraphEditorWidget()
{
	FGraphAppearanceInfo AppearanceInfo;
	AppearanceInfo.CornerText = LOCTEXT("CornerText", "AI Tactical Combat System");

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnSelectionChanged = SGraphEditor::FOnSelectionChanged::CreateSP(this, &FAITCSTacticalGraphAssetEditor::HandleGraphSelectionChanged);
	GraphEvents.OnSpawnNodeByShortcutAtLocation = SGraphEditor::FOnSpawnNodeByShortcutAtLocation::CreateSP(this, &FAITCSTacticalGraphAssetEditor::HandleSpawnNodeByShortcut);
	// Handle inline text commits (rename / comment edits)
	GraphEvents.OnTextCommitted = FOnNodeTextCommitted::CreateSP(this, &FAITCSTacticalGraphAssetEditor::HandleNodeTextCommitted);
	// Handle clicks on the graph panel (used to select links by clicking splines)
	GraphEvents.OnMouseButtonDown = SGraphEditor::FOnMouseButtonDown::CreateSP(this, &FAITCSTacticalGraphAssetEditor::HandleGraphPanelMouseButtonDown);

	GraphEditorWidget = SNew(SGraphEditor)
		.IsEditable(true)
		.Appearance(AppearanceInfo)
		.AdditionalCommands(GraphEditorCommands)
		.GraphToEdit(EditorGraph)
		.GraphEvents(GraphEvents);

	return GraphEditorWidget.ToSharedRef();
}

TSharedRef<SWidget> FAITCSTacticalGraphAssetEditor::CreateDetailsWidget()
{
	return SNew(SBorder)
		.Padding(6.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				ObjectDetailsView.ToSharedRef()
			]
		];
}

TSharedRef<SDockTab> FAITCSTacticalGraphAssetEditor::SpawnLinksTab(const FSpawnTabArgs& Args)
{
	(void)Args;

	return SNew(SDockTab)
		.Label(LOCTEXT("LinksTabLabel", "Links"))
		[
			CreateLinksWidget()
		];
}

TSharedRef<SWidget> FAITCSTacticalGraphAssetEditor::CreateLinksWidget()
{
	return SNew(SBorder)
		.Padding(6.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				LinkListView.ToSharedRef()
			]
		];
}

void FAITCSTacticalGraphAssetEditor::ExtendToolbar()
{
	TSharedPtr<FExtender> ToolbarExtender = MakeShared<FExtender>();
	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& Builder)
		{
			Builder.AddWidget(CreateValidateToolbarWidget());
			// Temporarily hide object manipulation toolbar buttons. Actions are still available via hotkeys.
			// Builder.AddToolBarButton(
			// 	FUIAction(FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::AddTacticalObject)),
			// 	NAME_None,
			// 	LOCTEXT("ToolbarAddObject", "Add Object"),
			// 	LOCTEXT("ToolbarAddObjectTooltip", "Add a tactical object to the graph. Hotkey: (N + LMB)"),
			// 	FSlateIcon(TEXT("AITCSEditorStyle"), TEXT("AITCSEditor.Toolbar.AddObject"))
			// );

			// Builder.AddToolBarButton(
			// 	FUIAction(FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::DuplicateSelectedObject)),
			// 	NAME_None,
			// 	LOCTEXT("ToolbarDuplicateObject", "Duplicate"),
			// 	LOCTEXT("ToolbarDuplicateObjectTooltip", "Duplicate selected tactical object(s). Hotkey: (Ctrl + D)"),
			// 	FSlateIcon(TEXT("AITCSEditorStyle"), TEXT("AITCSEditor.Toolbar.Duplicate"))
			// );

			// Builder.AddToolBarButton(
			// 	FUIAction(FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::DeleteSelectedObject)),
			// 	NAME_None,
			// 	LOCTEXT("ToolbarDeleteObject", "Delete Object"),
			// 	LOCTEXT("ToolbarDeleteObjectTooltip", "Delete selected tactical object. Hotkey: (Delete)"),
			// 	FSlateIcon(TEXT("AITCSEditorStyle"), TEXT("AITCSEditor.Toolbar.DeleteObject"))
			// );

		})
	);

	AddToolbarExtender(ToolbarExtender);
}

void FAITCSTacticalGraphAssetEditor::BindGraphCommands()
{
	GraphEditorCommands = MakeShared<FUICommandList>();
	// Disabled intentionally: see the matching toolkit bindings above.
	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Undo,
		FExecuteAction::CreateLambda([]() {}));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Redo,
		FExecuteAction::CreateLambda([]() {}));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::DeleteSelectedObject));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Duplicate,
		FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::DuplicateSelectedObject));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::CopySelection));

	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::PasteSelection));

	GraphEditorCommands->MapAction(
		FGraphEditorCommands::Get().CreateComment,
		FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::CreateCommentForSelection));

	// Rename (F2) should trigger inline rename for selected comment nodes
	GraphEditorCommands->MapAction(
		FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &FAITCSTacticalGraphAssetEditor::RequestRenameSelection));
}

TSharedRef<SWidget> FAITCSTacticalGraphAssetEditor::CreateValidateToolbarWidget()
{
	return SNew(SBox)
	.HeightOverride(28.0f)
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.AutoWidth()
		.VAlign(VAlign_Center)
		[
			SNew(SButton)
			.ToolTipText(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FAITCSTacticalGraphAssetEditor::GetValidateButtonToolTip)))
			.OnClicked_Lambda([this]() -> FReply { ValidateGraph(); return FReply::Handled(); })
				.ButtonStyle(FAppStyle::Get(), "NoBorder")
			.ContentPadding(FMargin(4.0f, 0.0f))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(this, &FAITCSTacticalGraphAssetEditor::GetValidateButtonIcon)
					.ColorAndOpacity(this, &FAITCSTacticalGraphAssetEditor::GetValidateIconTint)
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(FMargin(6.0f, 0.0f, 6.0f, 0.0f))
				[
					SNew(STextBlock)
					.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateSP(this, &FAITCSTacticalGraphAssetEditor::GetValidateButtonLabel)))
					.ColorAndOpacity(FSlateColor::UseForeground())
				]
			]
		]
	];
}

FSlateColor FAITCSTacticalGraphAssetEditor::GetValidateButtonBackgroundColor() const
{
	if (!GraphAsset)
	{
		return FLinearColor::White;
	}

	const UPackage* Package = GraphAsset->GetOutermost();
	const bool bDirty = Package ? Package->IsDirty() : false;

	if (bLastValidationHadErrors)
	{
		return FLinearColor(1.0f, 0.25f, 0.25f, 1.0f); // red
	}
	if (bDirty)
	{
		return FLinearColor(1.0f, 0.85f, 0.2f, 1.0f); // yellow
	}
	if (bValidationHasRun && !bLastValidationHadErrors)
	{
		return FLinearColor(0.25f, 0.8f, 0.25f, 1.0f); // green
	}

	return FLinearColor::White;
}

const FSlateBrush* FAITCSTacticalGraphAssetEditor::GetValidateButtonIcon() const
{
	static TUniquePtr<FSlateVectorImageBrush> ValidateBrush;
	if (!ValidateBrush.IsValid())
	{
		TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("AITacticalCombatSystem"));
		if (Plugin.IsValid())
		{
			const FString IconPath = FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/Toolbar/Validate.svg"));
			ValidateBrush.Reset(new FSlateVectorImageBrush(IconPath, FVector2D(16.0f, 16.0f)));
		}
	}

	if (ValidateBrush.IsValid())
	{
		return ValidateBrush.Get();
	}

	// Fallback to editor brush
	const FSlateBrush* Brush = FEditorStyle::GetBrush(TEXT("Kismet.RecompileBlueprint"));
	if (Brush)
	{
		return Brush;
	}

	return FAppStyle::Get().GetBrush(TEXT("Icons.Warning"));
}

FSlateColor FAITCSTacticalGraphAssetEditor::GetValidateIconTint() const
{
	if (!GraphAsset)
	{
		return FLinearColor::White;
	}

	const UPackage* Package = GraphAsset->GetOutermost();
	const bool bDirty = Package ? Package->IsDirty() : false;

	if (bLastValidationHadErrors)
	{
		return FLinearColor(1.0f, 0.25f, 0.25f, 1.0f); // red
	}
	if (bDirty)
	{
		return FLinearColor(1.0f, 0.85f, 0.2f, 1.0f); // yellow
	}
	if (bValidationHasRun && !bLastValidationHadErrors)
	{
		return FLinearColor(0.25f, 0.8f, 0.25f, 1.0f); // green
	}

	return FLinearColor::White;
}

FText FAITCSTacticalGraphAssetEditor::GetValidateOverlayText() const
{
	if (!GraphAsset)
	{
		return FText::GetEmpty();
	}

	const UPackage* Package = GraphAsset->GetOutermost();
	const bool bDirty = Package ? Package->IsDirty() : false;

	if (bLastValidationHadErrors)
	{
		return FText::FromString(TEXT("✖"));
	}
	if (bDirty)
	{
		return FText::FromString(TEXT("?"));
	}
	if (bValidationHasRun && !bLastValidationHadErrors)
	{
		return FText::FromString(TEXT("✔"));
	}

	return FText::GetEmpty();
}

void FAITCSTacticalGraphAssetEditor::AddTacticalObject()
{
	if (!EditorGraph)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddTacticalObjectTransaction", "Add AITCS Tactical Object"));
	const FVector2D Location(240.0f, 120.0f + EditorGraph->Nodes.Num() * 80.0f);
	UAITCSEditorGraphNode* NewNode = EditorGraph->AddTacticalObjectNode(Location, true);
	if (GraphEditorWidget && NewNode)
	{
		GraphEditorWidget->ClearSelectionSet();
		GraphEditorWidget->SetNodeSelection(NewNode, true);
	}
}

void FAITCSTacticalGraphAssetEditor::DuplicateSelectedObject()
{
	if (!GraphEditorWidget || !GraphAsset || !EditorGraph)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DuplicateTacticalObjectTransaction", "Duplicate AITCS Tactical Object"));

	TSet<UObject*> SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	for (UObject* SelectedObject : SelectedNodes)
	{
		if (UAITCSEditorGraphNode* Node = Cast<UAITCSEditorGraphNode>(SelectedObject))
		{
			if (!Node->CanDuplicateNode())
			{
				continue;
			}

			if (const FAITCSTacticalObject* SourceObject = GraphAsset->FindObject(Node->ObjectId))
			{
				GraphAsset->Modify();
				FAITCSTacticalObject NewObject = *SourceObject;
				NewObject.ObjectId = FGuid::NewGuid();
				GraphAsset->Objects.Add(NewObject);
				GraphAsset->SetEditorNodePosition(NewObject.ObjectId, FVector2D(Node->NodePosX + 40.0f, Node->NodePosY + 40.0f));
				GraphAsset->MarkPackageDirty();

				if (EditorGraph)
				{
					UAITCSEditorGraphNode* NewNode = EditorGraph->CreateNodeForObject(NewObject.ObjectId, true);
					if (GraphEditorWidget && NewNode)
					{
						GraphEditorWidget->ClearSelectionSet();
						GraphEditorWidget->SetNodeSelection(NewNode, true);
					}
					EditorGraph->NotifyGraphChanged();
				}
			}
		}
	}
}

void FAITCSTacticalGraphAssetEditor::DeleteSelectedObject()
{
	if (!GraphEditorWidget || !GraphAsset)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("DeleteTacticalObjectTransaction", "Delete AITCS Tactical Object"));

	TSet<UObject*> SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	for (UObject* SelectedObject : SelectedNodes)
	{
		if (UAITCSEditorGraphNode* Node = Cast<UAITCSEditorGraphNode>(SelectedObject))
		{
			if (Node->CanUserDeleteNode())
			{
				// Older graphs could contain two visual nodes referencing the same
				// object (the old Simple Object action reused an existing ObjectId).
				// Keep the asset object while another node still references it.
				bool bHasAnotherNodeWithObjectId = false;
				if (EditorGraph)
				{
					for (UEdGraphNode* OtherNode : EditorGraph->Nodes)
					{
						if (OtherNode != Node)
						{
							if (const UAITCSEditorGraphNode* OtherTacticalNode = Cast<UAITCSEditorGraphNode>(OtherNode))
							{
								bHasAnotherNodeWithObjectId = OtherTacticalNode->ObjectId == Node->ObjectId;
								if (bHasAnotherNodeWithObjectId)
								{
									break;
								}
							}
						}
					}
				}

				if (!bHasAnotherNodeWithObjectId)
				{
					GraphAsset->RemoveTacticalObject(Node->ObjectId);
				}
				EditorGraph->RemoveNode(Node);
			}
		}
		else if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(SelectedObject))
		{
			// Remove comment nodes from the editor graph; SyncAssetFromGraph will update GraphAsset->EditorComments
			EditorGraph->RemoveNode(CommentNode);
		}
	}

	if (EditorGraph)
	{
		EditorGraph->SyncAssetFromGraph();
		EditorGraph->NotifyGraphChanged();
	}

	ObjectDetailsView->SetObject(nullptr);
	bObjectProxyActive = false;
	LinkItems.Reset();
	if (LinkListView)
	{
		LinkListView->RequestListRefresh();
	}
	LinkDetailsView->SetObject(nullptr);
	bLinkProxyActive = false;
}

void FAITCSTacticalGraphAssetEditor::CreateCommentForSelection()
{
	if (!EditorGraph)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("CreateCommentTransaction", "Add AITCS Graph Comment"));
	EditorGraph->Modify();

	FVector2D MinPosition(0.0f, 0.0f);
	FVector2D MaxPosition(420.0f, 240.0f);
	bool bHasSelectedNodes = false;

	if (GraphEditorWidget)
	{
		const TSet<UObject*> SelectedNodes = GraphEditorWidget->GetSelectedNodes();
		for (UObject* SelectedObject : SelectedNodes)
		{
			const UEdGraphNode* SelectedNode = Cast<UEdGraphNode>(SelectedObject);
			if (!SelectedNode || Cast<UEdGraphNode_Comment>(SelectedNode))
			{
				continue;
			}

			const FVector2D NodeMin(SelectedNode->NodePosX, SelectedNode->NodePosY);
			const FVector2D NodeMax(SelectedNode->NodePosX + FMath::Max(SelectedNode->NodeWidth, 120), SelectedNode->NodePosY + FMath::Max(SelectedNode->NodeHeight, 80));
			if (!bHasSelectedNodes)
			{
				MinPosition = NodeMin;
				MaxPosition = NodeMax;
				bHasSelectedNodes = true;
			}
			else
			{
				MinPosition.X = FMath::Min(MinPosition.X, NodeMin.X);
				MinPosition.Y = FMath::Min(MinPosition.Y, NodeMin.Y);
				MaxPosition.X = FMath::Max(MaxPosition.X, NodeMax.X);
				MaxPosition.Y = FMath::Max(MaxPosition.Y, NodeMax.Y);
			}
		}
	}

	constexpr float Padding = 48.0f;
	UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(EditorGraph, NAME_None, RF_Transactional);
	CommentNode->CreateNewGuid();
	CommentNode->NodeComment = TEXT("Comment");
	CommentNode->NodePosX = static_cast<int32>(MinPosition.X - Padding);
	CommentNode->NodePosY = static_cast<int32>(MinPosition.Y - Padding);
	CommentNode->NodeWidth = static_cast<int32>(FMath::Max(MaxPosition.X - MinPosition.X + Padding * 2.0f, 360.0f));
	CommentNode->NodeHeight = static_cast<int32>(FMath::Max(MaxPosition.Y - MinPosition.Y + Padding * 2.0f, 180.0f));
	CommentNode->CommentColor = FLinearColor(0.35f, 0.35f, 0.35f, 0.35f);

	EditorGraph->AddNode(CommentNode, true, true);
	EditorGraph->NotifyGraphChanged();
	if (GraphEditorWidget)
	{
		GraphEditorWidget->ClearSelectionSet();
		GraphEditorWidget->SetNodeSelection(CommentNode, true);
	}
}

void FAITCSTacticalGraphAssetEditor::CopySelection()
{
	CopiedObjectIndices.Reset();
	CopiedComments.Reset();
	CopiedObjectPositions.Reset();
	CopiedCommentPositions.Reset();

	if (!GraphEditorWidget || !GraphAsset)
	{
		return;
	}

	const TSet<UObject*> SelectedNodes = GraphEditorWidget->GetSelectedNodes();
	if (SelectedNodes.Num() == 0)
	{
		return;
	}

	FVector2D MinPos(FLT_MAX, FLT_MAX);
	FVector2D MaxPos(-FLT_MAX, -FLT_MAX);
	bool bHasAny = false;

	for (UObject* SelectedObject : SelectedNodes)
	{
		if (UAITCSEditorGraphNode* Node = Cast<UAITCSEditorGraphNode>(SelectedObject))
		{
			if (const FAITCSTacticalObject* Obj = GraphAsset->FindObject(Node->ObjectId))
			{
				// Find index of the object in the asset array
				int32 ObjectIndex = INDEX_NONE;
				for (int32 i = 0; i < GraphAsset->Objects.Num(); ++i)
				{
					if (GraphAsset->Objects[i].ObjectId == Node->ObjectId)
					{
						ObjectIndex = i;
						break;
					}
				}
				if (ObjectIndex != INDEX_NONE)
				{
					CopiedObjectIndices.Add(ObjectIndex);
					const FVector2D Pos(static_cast<double>(Node->NodePosX), static_cast<double>(Node->NodePosY));
					CopiedObjectPositions.Add(Pos);

					if (!bHasAny)
					{
						MinPos = Pos;
						MaxPos = Pos;
						bHasAny = true;
					}
					else
					{
						MinPos.X = FMath::Min(MinPos.X, Pos.X);
						MinPos.Y = FMath::Min(MinPos.Y, Pos.Y);
						MaxPos.X = FMath::Max(MaxPos.X, Pos.X);
						MaxPos.Y = FMath::Max(MaxPos.Y, Pos.Y);
					}
				}
			}
		}
		else if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(SelectedObject))
		{

			FCopiedComment Comment;
			Comment.Text = CommentNode->NodeComment;
			Comment.Position = FVector2D(CommentNode->NodePosX, CommentNode->NodePosY);
			Comment.Size = FVector2D(CommentNode->NodeWidth, CommentNode->NodeHeight);
			Comment.Color = CommentNode->CommentColor;
			CopiedComments.Add(Comment);
			const FVector2D Pos = Comment.Position;
			CopiedCommentPositions.Add(Pos);

			if (!bHasAny)
			{
				MinPos = Pos;
				MaxPos = Pos;
				bHasAny = true;
			}
			else
			{
				MinPos.X = FMath::Min(MinPos.X, Pos.X);
				MinPos.Y = FMath::Min(MinPos.Y, Pos.Y);
				MaxPos.X = FMath::Max(MaxPos.X, Pos.X);
				MaxPos.Y = FMath::Max(MaxPos.Y, Pos.Y);
			}
		}
	}

	if (bHasAny)
	{
		CopiedCenter = (MinPos + MaxPos) * 0.5f;
	}
	else
	{
		CopiedCenter = FVector2D::ZeroVector;
	}
}

void FAITCSTacticalGraphAssetEditor::PasteSelection()
{
	if ((!CopiedObjectIndices.Num() && !CopiedComments.Num()) || !EditorGraph || !GraphAsset)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("PasteTacticalObjects", "Paste AITCS Graph Items"));
	EditorGraph->Modify();
	GraphAsset->Modify();

	const FVector2D PasteCenter = bHasLastGraphClickLocation ? LastGraphClickLocation : (CopiedCenter + FVector2D(40.0f, 40.0f));

	TArray<UEdGraphNode*> NewCreatedNodes;

	// Paste objects
	for (int32 Index = 0; Index < CopiedObjectIndices.Num(); ++Index)
	{
		const int32 SrcIndex = CopiedObjectIndices[Index];
		if (!GraphAsset->Objects.IsValidIndex(SrcIndex))
		{
			continue;
		}

		const FAITCSTacticalObject& Src = GraphAsset->Objects[SrcIndex];
		const FVector2D SrcPos = CopiedObjectPositions.IsValidIndex(Index) ? CopiedObjectPositions[Index] : FVector2D::ZeroVector;
		const FVector2D Delta = SrcPos - CopiedCenter;
		const FVector2D NewPos = PasteCenter + Delta;

		FAITCSTacticalObject NewObject = Src;
		NewObject.ObjectId = FGuid::NewGuid();

		GraphAsset->Objects.Add(NewObject);
		GraphAsset->SetEditorNodePosition(NewObject.ObjectId, NewPos);
		GraphAsset->MarkPackageDirty();

		if (EditorGraph)
		{
			UAITCSEditorGraphNode* NewNode = EditorGraph->CreateNodeForObject(NewObject.ObjectId, true);
			if (NewNode)
			{
				NewCreatedNodes.Add(NewNode);
			}
		}
	}

	// Paste comments
	for (int32 Index = 0; Index < CopiedComments.Num(); ++Index)
	{
		const FCopiedComment& Src = CopiedComments[Index];
		const FVector2D SrcPos = CopiedCommentPositions.IsValidIndex(Index) ? CopiedCommentPositions[Index] : Src.Position;
		const FVector2D Delta = SrcPos - CopiedCenter;
		const FVector2D NewPos = PasteCenter + Delta;

		FAITCSTacticalGraphComment NewComment;
		NewComment.CommentId = FGuid::NewGuid();
		NewComment.Text = Src.Text;
		NewComment.Position = NewPos;
		NewComment.Size = Src.Size;
		NewComment.Color = Src.Color;

		GraphAsset->EditorComments.Add(NewComment);
		GraphAsset->MarkPackageDirty();

		UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(EditorGraph, NAME_None, RF_Transactional);
		CommentNode->NodeGuid = NewComment.CommentId;
		CommentNode->NodeComment = NewComment.Text;
		CommentNode->NodePosX = static_cast<int32>(NewComment.Position.X);
		CommentNode->NodePosY = static_cast<int32>(NewComment.Position.Y);
		CommentNode->NodeWidth = static_cast<int32>(NewComment.Size.X);
		CommentNode->NodeHeight = static_cast<int32>(NewComment.Size.Y);
		CommentNode->CommentColor = NewComment.Color;
		EditorGraph->AddNode(CommentNode, true, true);
		NewCreatedNodes.Add(CommentNode);
	}

	if (EditorGraph)
	{
		EditorGraph->NotifyGraphChanged();
		EditorGraph->SyncAssetFromGraph();
	}

	if (GraphEditorWidget)
	{
		GraphEditorWidget->ClearSelectionSet();
		for (UEdGraphNode* Node : NewCreatedNodes)
		{
			if (Node)
			{
				GraphEditorWidget->SetNodeSelection(Node, true);
			}
		}
	}

	// Keep the last click location for subsequent pastes
}

void FAITCSTacticalGraphAssetEditor::ValidateGraph()
{
	LastValidationMessages.Reset();
	bValidationHasRun = true;
	bLastValidationHadErrors = false;

	if (!GraphAsset)
	{
		bLastValidationHadErrors = true;
		RegenerateMenusAndToolbars();
		return;
	}

	if (EditorGraph)
	{
		EditorGraph->SyncAssetFromGraph(false);
	}

	TArray<FAITCSGraphValidationMessage> Messages;
	GraphAsset->ValidateGraph(Messages);

	if (Messages.IsEmpty())
	{
		LastValidationMessages.Add(LOCTEXT("ValidationOk", "Validation passed."));
		RegenerateMenusAndToolbars();
		return;
	}

	for (const FAITCSGraphValidationMessage& Message : Messages)
	{
		if (Message.Severity == EAITCSValidationSeverity::Error)
		{
			bLastValidationHadErrors = true;
		}
		LastValidationMessages.Add(Message.Message);
	}

	RegenerateMenusAndToolbars();
}

void FAITCSTacticalGraphAssetEditor::HandleGraphChanged(const FEdGraphEditAction& Action)
{
	(void)Action;
	if (EditorGraph)
	{
		EditorGraph->SyncAssetFromGraph();
	}
}

void FAITCSTacticalGraphAssetEditor::HandleGraphSelectionChanged(const TSet<UObject*>& SelectedObjects)
{
	for (UObject* SelectedObject : SelectedObjects)
	{
		if (UAITCSEditorGraphNode* Node = Cast<UAITCSEditorGraphNode>(SelectedObject))
		{
			// Select tactical object
			bCommentActive = false;
			SelectedCommentNode = nullptr;
			SelectObjectNode(Node);
			return;
		}

		if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(SelectedObject))
		{
			// Show comment properties in the Details view using a proxy (NodeComment is not EditAnywhere)
			bObjectProxyActive = false;
			if (CommentProxy)
			{
				CommentProxy->LoadFromCommentNode(CommentNode);
				ObjectDetailsView->SetObject(CommentProxy);
			}
			else
			{
				ObjectDetailsView->SetObject(nullptr);
			}
			bCommentActive = true;
			SelectedCommentNode = CommentNode;

			// Clear link selection
			bLinkProxyActive = false;
			if (EditorGraph)
			{
				EditorGraph->SelectedLinkId = FGuid();
			}

			LinkItems.Reset();
			if (LinkListView)
			{
				LinkListView->RequestListRefresh();
			}
			LinkDetailsView->SetObject(nullptr);
			return;
		}
	}

	// Nothing selected
	ObjectDetailsView->SetObject(nullptr);
	bObjectProxyActive = false;
	bCommentActive = false;
	SelectedCommentNode = nullptr;

	// Clear link selection
	if (EditorGraph)
	{
		EditorGraph->SelectedLinkId = FGuid();
	}

	LinkItems.Reset();
	if (LinkListView)
	{
		LinkListView->RequestListRefresh();
	}
	LinkDetailsView->SetObject(nullptr);
	bLinkProxyActive = false;
}

FReply FAITCSTacticalGraphAssetEditor::HandleSpawnNodeByShortcut(FInputChord InputChord, const FVector2f& GraphLocation)
{
	if (InputChord.Key != EKeys::N || InputChord.bCtrl || InputChord.bAlt || InputChord.bShift || InputChord.bCmd || !EditorGraph)
	{
		return FReply::Unhandled();
	}

	const FScopedTransaction Transaction(LOCTEXT("AddTacticalObjectShortcutTransaction", "Add AITCS Tactical Object"));
	const FVector2D Location(static_cast<double>(GraphLocation.X), static_cast<double>(GraphLocation.Y));
	UAITCSEditorGraphNode* NewNode = EditorGraph->AddTacticalObjectNode(Location, true);
	if (GraphEditorWidget && NewNode)
	{
		GraphEditorWidget->ClearSelectionSet();
		GraphEditorWidget->SetNodeSelection(NewNode, true);
	}

	return NewNode ? FReply::Handled() : FReply::Unhandled();
}



void FAITCSTacticalGraphAssetEditor::HandleGraphLinkSelected(const FGuid& LinkId)
{
	// Clear node selection when a link is selected from the graph
	if (GraphEditorWidget)
	{
		GraphEditorWidget->ClearSelectionSet();
	}

	// Load link details and show in the main details view
	if (!GraphAsset || !LinkProxy)
	{
		ObjectDetailsView->SetObject(nullptr);
		bLinkProxyActive = false;
		return;
	}

	if (const FAITCSTacticalLink* Link = GraphAsset->FindLink(LinkId))
	{
		LinkProxy->LoadFromLink(*Link, GraphAsset);
		ObjectDetailsView->SetObject(LinkProxy);
		bLinkProxyActive = true;
		bObjectProxyActive = false;
	}

	if (EditorGraph)
	{
		EditorGraph->NotifyGraphChanged();
	}
}

void FAITCSTacticalGraphAssetEditor::HandleObjectDetailsChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	(void)PropertyChangedEvent;

	if (bCommentActive && SelectedCommentNode.IsValid())
	{
		// Apply changes from the comment-details proxy back to the comment node
		if (CommentProxy && SelectedCommentNode.IsValid())
		{
			CommentProxy->ApplyToCommentNode(SelectedCommentNode.Get());
		}

		if (EditorGraph)
		{
			EditorGraph->SyncAssetFromGraph(true);
			EditorGraph->NotifyGraphChanged();
		}
		return;
	}

	if (ObjectProxy && bObjectProxyActive)
	{
		ObjectProxy->ApplyToGraphAsset(GraphAsset);
		if (EditorGraph)
		{
			EditorGraph->NotifyGraphChanged();
		}
	}
}

void FAITCSTacticalGraphAssetEditor::HandleLinkDetailsChanged(const FPropertyChangedEvent& PropertyChangedEvent)
{
	(void)PropertyChangedEvent;

	if (LinkProxy && bLinkProxyActive)
	{
		LinkProxy->ApplyToGraphAsset(GraphAsset);
	}
}

void FAITCSTacticalGraphAssetEditor::SelectObjectNode(UAITCSEditorGraphNode* Node)
{
	if (!Node || !GraphAsset || !ObjectProxy)
	{
		return;
	}

	// Clear link selection when a node is selected
	bLinkProxyActive = false;
	if (EditorGraph)
	{
		EditorGraph->SelectedLinkId = FGuid();
	}

	if (const FAITCSTacticalObject* Object = GraphAsset->FindObject(Node->ObjectId))
	{
		ObjectProxy->LoadFromObject(*Object);
		ObjectDetailsView->SetObject(ObjectProxy);
		bObjectProxyActive = true;
		RebuildSelectedLinks(Object->ObjectId);
	}
}



FReply FAITCSTacticalGraphAssetEditor::HandleGraphPanelMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	if (!GraphEditorWidget)
	{
		return FReply::Unhandled();
	}

	SGraphPanel* GraphPanel = GraphEditorWidget->GetGraphPanel();
	if (!GraphPanel)
	{
		return FReply::Unhandled();
	}

	// Record last graph click location (in graph coordinates) for paste positioning
	const FVector2D LocalClick = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	// PanelCoordToGraphCoord converts panel/local coords to graph coords (accounts for pan/zoom)
	LastGraphClickLocation = GraphPanel->PanelCoordToGraphCoord(LocalClick);
	bHasLastGraphClickLocation = true;

	const FGraphSplineOverlapResult& Overlap = GraphPanel->GetPreviousFrameSplineOverlap();
	if (!Overlap.IsValid() || !Overlap.GetCloseToSpline())
	{
		return FReply::Unhandled();
	}

	UEdGraphPin* Pin1 = nullptr;
	UEdGraphPin* Pin2 = nullptr;
	if (Overlap.GetPins(*GraphPanel, Pin1, Pin2))
	{
		UEdGraphNode* NodeA = Pin1 ? Pin1->GetOwningNode() : nullptr;
		UEdGraphNode* NodeB = Pin2 ? Pin2->GetOwningNode() : nullptr;
		UAITCSEditorGraphNode* ANode = Cast<UAITCSEditorGraphNode>(NodeA);
		UAITCSEditorGraphNode* BNode = Cast<UAITCSEditorGraphNode>(NodeB);
		if (ANode && BNode && GraphAsset && EditorGraph)
		{
			const FAITCSTacticalLink* FoundLink = GraphAsset->FindLinkBetween(ANode->ObjectId, BNode->ObjectId);
			if (!FoundLink)
			{
				FoundLink = GraphAsset->FindLinkBetween(BNode->ObjectId, ANode->ObjectId);
			}

			if (FoundLink)
			{
				EditorGraph->SelectedLinkId = FoundLink->LinkId;
				EditorGraph->OnSelectedLinkChanged.Broadcast(FoundLink->LinkId);
				EditorGraph->NotifyGraphChanged();
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}

void FAITCSTacticalGraphAssetEditor::RebuildSelectedLinks(const FGuid& ObjectId)
{
	LinkItems.Reset();

	if (GraphAsset)
	{
		for (const FAITCSTacticalLink& Link : GraphAsset->Links)
		{
			if (Link.SourceObjectId == ObjectId)
			{
				LinkItems.Add(MakeShared<FGuid>(Link.LinkId));
			}
		}
	}

	// Clear selection in link list
	if (LinkListView)
	{
		LinkListView->ClearSelection();
		LinkListView->RequestListRefresh();
	}
}

TSharedRef<ITableRow> FAITCSTacticalGraphAssetEditor::GenerateLinkRow(TSharedPtr<FGuid> LinkId, const TSharedRef<STableViewBase>& OwnerTable)
{
	FText RowText = LOCTEXT("UnknownLink", "Unknown Link");

	if (GraphAsset && LinkId.IsValid())
	{
		if (const FAITCSTacticalLink* Link = GraphAsset->FindLink(*LinkId))
		{
			const FAITCSTacticalObject* ParentObject = GraphAsset->FindObject(Link->SourceObjectId);
			const FAITCSTacticalObject* ChildObject = GraphAsset->FindObject(Link->TargetObjectId);
			const FString ParentLabel = ParentObject
				? ParentObject->DisplayName.ToString()
				: Link->SourceObjectId.ToString(EGuidFormats::Short);
			const FString ChildLabel = ChildObject
				? ChildObject->DisplayName.ToString()
				: Link->TargetObjectId.ToString(EGuidFormats::Short);

			RowText = FText::FromString(FString::Printf(TEXT("%s -> %s"), *ParentLabel, *ChildLabel));
		}
	}

	return SNew(STableRow<TSharedPtr<FGuid>>, OwnerTable)
		[
			SNew(STextBlock)
			.Text(RowText)
		];
}

void FAITCSTacticalGraphAssetEditor::HandleLinkSelectionChanged(TSharedPtr<FGuid> LinkId, ESelectInfo::Type SelectInfo)
{
	(void)SelectInfo;

	if (!LinkId.IsValid() || !GraphAsset || !LinkProxy)
	{
		ObjectDetailsView->SetObject(nullptr);
		bLinkProxyActive = false;
		
		// Clear link selection in graph
		if (EditorGraph)
		{
			EditorGraph->SelectedLinkId = FGuid();
			EditorGraph->NotifyGraphChanged();
		}
		return;
	}

	if (const FAITCSTacticalLink* Link = GraphAsset->FindLink(*LinkId))
	{
		LinkProxy->LoadFromLink(*Link, GraphAsset);
		ObjectDetailsView->SetObject(LinkProxy);
		bLinkProxyActive = true;
		bObjectProxyActive = false;

		// Highlight the link in the graph
		if (EditorGraph)
		{
			EditorGraph->SelectedLinkId = *LinkId;
			EditorGraph->NotifyGraphChanged();
		}
	}
}

FText FAITCSTacticalGraphAssetEditor::GetValidationText() const
{
	if (LastValidationMessages.IsEmpty())
	{
		return LOCTEXT("ValidationIdle", "Validation has not run.");
	}

	FString Combined;
	for (const FText& Message : LastValidationMessages)
	{
		if (!Combined.IsEmpty())
		{
			Combined += LINE_TERMINATOR;
		}
		Combined += Message.ToString();
	}

	return FText::FromString(Combined);
}

FText FAITCSTacticalGraphAssetEditor::GetValidateButtonLabel() const
{
	if (!bValidationHasRun)
	{
		return LOCTEXT("ToolbarValidate", "Validate");
	}

	// If the asset was modified since last validation, show 'Validate' prompt
	if (GraphAsset)
	{
		const UPackage* Package = GraphAsset->GetOutermost();
		if (Package && Package->IsDirty())
		{
			return LOCTEXT("ToolbarValidateDirty", "Validate?");
		}
	}

	// On validation error we keep the action as "Validate" so user can re-run
	if (bLastValidationHadErrors)
	{
		return LOCTEXT("ToolbarValidate", "Validate");
	}

	return LOCTEXT("ToolbarValidateOk", "Validated");
}

FText FAITCSTacticalGraphAssetEditor::GetValidateButtonToolTip() const
{
	if (!bValidationHasRun)
	{
		return LOCTEXT("ToolbarValidateTooltip", "Validate player object, ids, links and selected group index values.");
	}

	return GetValidationText();
}

void FAITCSTacticalGraphAssetEditor::RequestRenameSelection()
{
	if (!GraphEditorWidget)
	{
		return;
	}

	UEdGraphNode* SelectedNode = GraphEditorWidget->GetSingleSelectedNode();
	if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(SelectedNode))
	{
		GraphEditorWidget->JumpToNode(CommentNode, true, true);
	}
}

void FAITCSTacticalGraphAssetEditor::HandleNodeTextCommitted(const FText& NewText, ETextCommit::Type CommitInfo, UEdGraphNode* Node)
{
	if (!Node)
	{
		return;
	}

	if (UEdGraphNode_Comment* CommentNode = Cast<UEdGraphNode_Comment>(Node))
	{
		const FString NewString = NewText.ToString();
		if (!CommentNode->NodeComment.Equals(NewString, ESearchCase::CaseSensitive))
		{
			const FScopedTransaction Transaction(LOCTEXT("EditNodeComment", "Change Node Comment"));
			CommentNode->Modify();

			FProperty* NodeCommentProperty = FindFProperty<FProperty>(CommentNode->GetClass(), TEXT("NodeComment"));
			if (NodeCommentProperty != nullptr)
			{
				CommentNode->PreEditChange(NodeCommentProperty);
				CommentNode->NodeComment = NewString;
				CommentNode->SetMakeCommentBubbleVisible(true);
				FPropertyChangedEvent NodeCommentPropertyChanged(NodeCommentProperty);
				CommentNode->PostEditChangeProperty(NodeCommentPropertyChanged);
			}
		}

		// If the details panel is showing the proxy for this comment, refresh it
		if (bCommentActive && SelectedCommentNode.IsValid() && SelectedCommentNode.Get() == CommentNode && CommentProxy)
		{
			CommentProxy->LoadFromCommentNode(CommentNode);
			ObjectDetailsView->SetObject(CommentProxy);
		}

		if (EditorGraph)
		{
			EditorGraph->SyncAssetFromGraph(true);
			EditorGraph->NotifyGraphChanged();
		}

		if (CommitInfo != ETextCommit::Default)
		{
			FSlateApplication::Get().DismissAllMenus();
		}
	}
}

#undef LOCTEXT_NAMESPACE

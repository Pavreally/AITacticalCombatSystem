// Pavel Gornostaev <https://github.com/Pavreally>

#include "Director/AITCSDirectorSubsystem.h"

#include "Components/AITCSTacticalUnitComponent.h"
#include "Features/AITCSTacticalEvaluator.h"
#include "Graph/AITCSTacticalGraphAsset.h"
#include "Interfaces/AITCSTacticalDecisionProvider.h"
#include "AIController.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/PlatformTime.h"
#include "Stats/Stats.h"

namespace
{
	struct FAITCSRankedTacticalOrderProposal
	{
		FAITCSTacticalOrderProposal Proposal;
		float WeightedScore = 0.0f;
		int32 EvaluatorSortPriority = 0;
		int32 Sequence = 0;
		int32 ReportIndex = INDEX_NONE;
	};

	struct FAITCSRankedTacticalOrderProposalSort
	{
		bool operator()(const FAITCSRankedTacticalOrderProposal& Left, const FAITCSRankedTacticalOrderProposal& Right) const
		{
			if (Left.EvaluatorSortPriority != Right.EvaluatorSortPriority)
			{
				return Left.EvaluatorSortPriority > Right.EvaluatorSortPriority;
			}

			if (!FMath::IsNearlyEqual(Left.WeightedScore, Right.WeightedScore))
			{
				return Left.WeightedScore > Right.WeightedScore;
			}

			return Left.Sequence < Right.Sequence;
		}
	};

	template<typename TRequiredBaseClass>
	UClass* ResolveAITCSSoftClass(const TSoftClassPtr<TRequiredBaseClass>& SoftClass)
	{
		if (SoftClass.IsNull())
		{
			return nullptr;
		}

		if (UClass* ResolvedClass = SoftClass.Get())
		{
			return ResolvedClass;
		}

		return SoftClass.LoadSynchronous();
	}

	template<typename TRequiredBaseClass>
	bool DoesAITCSRuntimeClassMatch(const TSoftClassPtr<TRequiredBaseClass>& RequiredClass, const TSubclassOf<TRequiredBaseClass>& RuntimeClass)
	{
		UClass* RequiredResolvedClass = ResolveAITCSSoftClass(RequiredClass);
		if (!RequiredResolvedClass)
		{
			return true;
		}

		UClass* RuntimeResolvedClass = RuntimeClass.Get();
		return RuntimeResolvedClass && RuntimeResolvedClass->IsChildOf(RequiredResolvedClass);
	}
}

void UAITCSDirectorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	RegisteredUnitComponents.Reset();
	RegisteredAgentStates.Reset();
	RuntimeGroupStatesByObjectId.Reset();
	LastOrdersByAgent.Reset();
	PendingOrders.Reset();
	CompiledGraph.Reset();
	LastGraphCompileMessages.Reset();
	TacticalEvaluators.Reset();
	LastEvaluatorReports.Reset();
	LastEvaluatorProposals.Reset();
	LastDecisionProviderPassTime = 0.0f;
	LastEvaluatorPassTime = 0.0f;
	EvaluationFrameCounter = 0;
}

void UAITCSDirectorSubsystem::Deinitialize()
{
	LastEvaluatorProposals.Reset();
	LastEvaluatorReports.Reset();
	TacticalEvaluators.Reset();
	LastGraphCompileMessages.Reset();
	CompiledGraph.Reset();
	PendingOrders.Reset();
	LastOrdersByAgent.Reset();
	RuntimeGroupStatesByObjectId.Reset();
	RegisteredAgentStates.Reset();
	RegisteredUnitComponents.Reset();
	DecisionProviderObject = nullptr;
	ActiveGraphAsset = nullptr;

	Super::Deinitialize();
}

void UAITCSDirectorSubsystem::Tick(float DeltaTime)
{
	RefreshComponentAgentStates();

	UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : FMath::Max(LastDecisionProviderPassTime, LastEvaluatorPassTime) + DeltaTime;

	if (bRunEvaluatorsOnTick)
	{
		if (CurrentTime - LastEvaluatorPassTime >= EvaluatorTickInterval)
		{
			LastEvaluatorPassTime = CurrentTime;
			RunEvaluatorPass(DeltaTime);
		}
	}

	if (bRunDecisionProviderOnTick && DecisionProviderObject)
	{
		if (CurrentTime - LastDecisionProviderPassTime >= DecisionProviderTickInterval)
		{
			LastDecisionProviderPassTime = CurrentTime;
			RequestDecisionProviderPass();
		}
	}

	FlushPendingOrders();
}

TStatId UAITCSDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UAITCSDirectorSubsystem, STATGROUP_Tickables);
}

bool UAITCSDirectorSubsystem::IsTickable() const
{
	return !IsTemplate();
}

bool UAITCSDirectorSubsystem::SetActiveTacticalGraph(UAITCSTacticalGraphAsset* InGraphAsset)
{
	ActiveGraphAsset = InGraphAsset;
	if (ActiveGraphAsset)
	{
		ActiveGraphAsset->EnsurePlayerObject();
		return CompileActiveTacticalGraph();
	}

	CompiledGraph.Reset();
	LastGraphCompileMessages.Reset();
	RuntimeGroupStatesByObjectId.Reset();
	return false;
}

UAITCSTacticalGraphAsset* UAITCSDirectorSubsystem::GetActiveTacticalGraph() const
{
	return ActiveGraphAsset;
}

bool UAITCSDirectorSubsystem::CompileActiveTacticalGraph()
{
	const bool bCompiled = CompiledGraph.CompileFromGraph(ActiveGraphAsset, GraphCompileOptions, LastGraphCompileMessages);
	if (bCompiled)
	{
		RebuildRuntimeGroupStates();
	}
	else
	{
		RuntimeGroupStatesByObjectId.Reset();
	}

	return bCompiled;
}

bool UAITCSDirectorSubsystem::IsTacticalGraphCompiled() const
{
	return CompiledGraph.IsCompiled();
}

FAITCSCompiledTacticalGraph UAITCSDirectorSubsystem::GetCompiledTacticalGraphCopy() const
{
	return CompiledGraph;
}

const FAITCSCompiledTacticalGraph& UAITCSDirectorSubsystem::GetCompiledTacticalGraph() const
{
	return CompiledGraph;
}

TArray<FAITCSGraphValidationMessage> UAITCSDirectorSubsystem::GetLastGraphCompileMessages() const
{
	return LastGraphCompileMessages;
}

bool UAITCSDirectorSubsystem::SetDecisionProvider(UObject* InDecisionProvider)
{
	if (!InDecisionProvider)
	{
		DecisionProviderObject = nullptr;
		return true;
	}

	if (!InDecisionProvider->GetClass()->ImplementsInterface(UAITCSTacticalDecisionProvider::StaticClass()))
	{
		return false;
	}

	DecisionProviderObject = InDecisionProvider;
	return true;
}

UObject* UAITCSDirectorSubsystem::GetDecisionProvider() const
{
	return DecisionProviderObject;
}

bool UAITCSDirectorSubsystem::RegisterTacticalUnit(UAITCSTacticalUnitComponent* UnitComponent, const FAITCSTacticalAgentRegistration& Registration, FAITCSTacticalAgentHandle& OutHandle)
{
	if (!UnitComponent)
	{
		OutHandle = FAITCSTacticalAgentHandle();
		return false;
	}

	FAITCSTacticalAgentRegistration EffectiveRegistration = Registration;
	EffectiveRegistration.AgentKind = EAITCSAgentKind::ActorComponent;
	PopulateRegistrationRuntimeClasses(EffectiveRegistration, UnitComponent->GetOwner());
	if (!ResolveRegistrationAssignment(EffectiveRegistration))
	{
		EffectiveRegistration.ObjectId = FGuid();
		EffectiveRegistration.GroupId = FGameplayTag();
	}
	OutHandle = MakeHandleFromRegistration(EffectiveRegistration);

	FAITCSTacticalAgentRuntimeState RuntimeState;
	RuntimeState.Handle = OutHandle;
	RuntimeState.AgentTags = EffectiveRegistration.AgentTags;
	RuntimeState.Location = EffectiveRegistration.Location;
	RuntimeState.PawnClass = EffectiveRegistration.PawnClass;
	RuntimeState.AIControllerClass = EffectiveRegistration.AIControllerClass;
	RuntimeState.bActive = true;
	RuntimeState.Actor = UnitComponent->GetOwner();

	if (const UWorld* World = GetWorld())
	{
		RuntimeState.LastUpdateTime = World->GetTimeSeconds();
	}

	RegisteredUnitComponents.Add(OutHandle.AgentId, UnitComponent);
	RegisteredAgentStates.Add(OutHandle.AgentId, RuntimeState);
	RebuildRuntimeGroupStates();
	return true;
}

bool UAITCSDirectorSubsystem::UnregisterTacticalUnit(const FGuid& AgentId)
{
	const bool bHadComponent = RegisteredUnitComponents.Remove(AgentId) > 0;
	const bool bHadState = RegisteredAgentStates.Remove(AgentId) > 0;
	LastOrdersByAgent.Remove(AgentId);
	RebuildRuntimeGroupStates();
	return bHadComponent || bHadState;
}

bool UAITCSDirectorSubsystem::RegisterVirtualTacticalAgent(const FAITCSTacticalAgentRegistration& Registration, FAITCSTacticalAgentHandle& OutHandle)
{
	FAITCSTacticalAgentRegistration EffectiveRegistration = Registration;
	EffectiveRegistration.AgentKind = EAITCSAgentKind::VirtualAgent;
	if (!ResolveRegistrationAssignment(EffectiveRegistration))
	{
		EffectiveRegistration.ObjectId = FGuid();
		EffectiveRegistration.GroupId = FGameplayTag();
	}
	OutHandle = MakeHandleFromRegistration(EffectiveRegistration);

	FAITCSTacticalAgentRuntimeState RuntimeState;
	RuntimeState.Handle = OutHandle;
	RuntimeState.AgentTags = EffectiveRegistration.AgentTags;
	RuntimeState.Location = EffectiveRegistration.Location;
	RuntimeState.PawnClass = EffectiveRegistration.PawnClass;
	RuntimeState.AIControllerClass = EffectiveRegistration.AIControllerClass;
	RuntimeState.bActive = true;
	RuntimeState.Actor = nullptr;

	if (const UWorld* World = GetWorld())
	{
		RuntimeState.LastUpdateTime = World->GetTimeSeconds();
	}

	RegisteredAgentStates.Add(OutHandle.AgentId, RuntimeState);
	RebuildRuntimeGroupStates();
	return true;
}

bool UAITCSDirectorSubsystem::UpdateVirtualTacticalAgent(const FGuid& AgentId, const FAITCSTacticalAgentRegistration& Registration)
{
	FAITCSTacticalAgentRuntimeState* RuntimeState = RegisteredAgentStates.Find(AgentId);
	if (!RuntimeState || RuntimeState->Handle.AgentKind != EAITCSAgentKind::VirtualAgent)
	{
		return false;
	}

	FAITCSTacticalAgentRegistration EffectiveRegistration = Registration;
	EffectiveRegistration.AgentKind = EAITCSAgentKind::VirtualAgent;
	if (!ResolveRegistrationAssignment(EffectiveRegistration))
	{
		EffectiveRegistration.ObjectId = FGuid();
		EffectiveRegistration.GroupId = FGameplayTag();
	}

	RuntimeState->Handle.ObjectId = EffectiveRegistration.ObjectId;
	RuntimeState->Handle.GroupId = EffectiveRegistration.GroupId;
	RuntimeState->Handle.DebugName = EffectiveRegistration.DebugName;
	RuntimeState->AgentTags = EffectiveRegistration.AgentTags;
	RuntimeState->Location = EffectiveRegistration.Location;
	RuntimeState->PawnClass = EffectiveRegistration.PawnClass;
	RuntimeState->AIControllerClass = EffectiveRegistration.AIControllerClass;
	RuntimeState->bActive = true;

	if (const UWorld* World = GetWorld())
	{
		RuntimeState->LastUpdateTime = World->GetTimeSeconds();
	}

	RebuildRuntimeGroupStates();
	return true;
}

bool UAITCSDirectorSubsystem::UnregisterVirtualTacticalAgent(const FGuid& AgentId)
{
	FAITCSTacticalAgentRuntimeState* RuntimeState = RegisteredAgentStates.Find(AgentId);
	if (!RuntimeState || RuntimeState->Handle.AgentKind != EAITCSAgentKind::VirtualAgent)
	{
		return false;
	}

	RegisteredAgentStates.Remove(AgentId);
	LastOrdersByAgent.Remove(AgentId);
	RebuildRuntimeGroupStates();
	return true;
}

FAITCSTacticalContextSnapshot UAITCSDirectorSubsystem::BuildContextSnapshot() const
{
	RefreshComponentAgentStates();

	FAITCSTacticalContextSnapshot Snapshot;
	if (ActiveGraphAsset)
	{
		Snapshot.ActiveGraphName = ActiveGraphAsset->GetFName();
		Snapshot.Objects = ActiveGraphAsset->Objects;
		Snapshot.Links = ActiveGraphAsset->Links;
		Snapshot.GroupAssets = ActiveGraphAsset->CollectReferencedGroupAssets();
	}

	RegisteredAgentStates.GenerateValueArray(Snapshot.Agents);
	return Snapshot;
}

TArray<FAITCSTacticalAgentRuntimeState> UAITCSDirectorSubsystem::GetRegisteredAgentStates() const
{
	TArray<FAITCSTacticalAgentRuntimeState> States;
	RegisteredAgentStates.GenerateValueArray(States);
	return States;
}

TArray<FAITCSTacticalRuntimeGroupState> UAITCSDirectorSubsystem::GetRuntimeGroupStates() const
{
	TArray<FAITCSTacticalRuntimeGroupState> States;
	RuntimeGroupStatesByObjectId.GenerateValueArray(States);
	return States;
}

void UAITCSDirectorSubsystem::RebuildRuntimeGroupStates()
{
	RuntimeGroupStatesByObjectId.Reset();

	if (!CompiledGraph.IsCompiled())
	{
		return;
	}

	for (const FAITCSCompiledTacticalObject& CompiledObject : CompiledGraph.Objects)
	{
		BuildRuntimeGroupStateFromCompiledObject(CompiledObject);
	}

	for (const TPair<FGuid, FAITCSTacticalAgentRuntimeState>& Pair : RegisteredAgentStates)
	{
		AddAgentToRuntimeGroupState(Pair.Value);
	}
}

int32 UAITCSDirectorSubsystem::ResolveRegisteredAgentAssignments(bool bOverwriteExistingAssignments)
{
	if (!CompiledGraph.IsCompiled())
	{
		if (!CompileActiveTacticalGraph())
		{
			return 0;
		}
	}

	RebuildRuntimeGroupStates();

	int32 ResolvedCount = 0;
	for (TPair<FGuid, FAITCSTacticalAgentRuntimeState>& Pair : RegisteredAgentStates)
	{
		FAITCSTacticalAgentRuntimeState& RuntimeState = Pair.Value;
		const FAITCSCompiledTacticalObject* AssignedObject = RuntimeState.Handle.ObjectId.IsValid()
			? CompiledGraph.FindObject(RuntimeState.Handle.ObjectId)
			: nullptr;
		const bool bHasCompleteAssignment = RuntimeState.Handle.ObjectId.IsValid()
			&& (RuntimeState.Handle.GroupId.IsValid() || (AssignedObject && AssignedObject->bIsSimpleObject));
		if (!bOverwriteExistingAssignments && bHasCompleteAssignment)
		{
			continue;
		}

		FAITCSTacticalAgentRegistration Registration;
		Registration.PreferredAgentId = RuntimeState.Handle.AgentId;
		Registration.ObjectId = bOverwriteExistingAssignments ? FGuid() : RuntimeState.Handle.ObjectId;
		Registration.GroupId = RuntimeState.Handle.GroupId;
		Registration.DebugName = RuntimeState.Handle.DebugName;
		Registration.AgentTags = RuntimeState.AgentTags;
		Registration.Location = RuntimeState.Location;
		Registration.PawnClass = RuntimeState.PawnClass;
		Registration.AIControllerClass = RuntimeState.AIControllerClass;
		Registration.AgentKind = RuntimeState.Handle.AgentKind;
		PopulateRegistrationRuntimeClasses(Registration, RuntimeState.Actor);

		if (!ResolveRegistrationAssignment(Registration))
		{
			continue;
		}

		RuntimeState.Handle.ObjectId = Registration.ObjectId;
		RuntimeState.Handle.GroupId = Registration.GroupId;
		RuntimeState.Handle.DebugName = Registration.DebugName;
		AddAgentToRuntimeGroupState(RuntimeState);

		if (RuntimeState.Handle.AgentKind == EAITCSAgentKind::ActorComponent)
		{
			if (TWeakObjectPtr<UAITCSTacticalUnitComponent>* ComponentPtr = RegisteredUnitComponents.Find(RuntimeState.Handle.AgentId))
			{
				if (UAITCSTacticalUnitComponent* Component = ComponentPtr->Get())
				{
					Component->ApplyTacticalAgentHandle(RuntimeState.Handle);
				}
			}
		}

		ResolvedCount++;
	}

	RebuildRuntimeGroupStates();
	return ResolvedCount;
}

bool UAITCSDirectorSubsystem::ResolveFormationMoveDestination(UAITCSTacticalUnitComponent* UnitComponent, AActor* TargetActor, FVector& OutDestination, float DefaultFollowDistance, float MemberSpacing, bool bApplyMemberSlotOffset) const
{
	float IgnoredFollowDistance = FMath::Max(0.0f, DefaultFollowDistance);
	bool bIgnoredUsesSimpleObjectSettings = false;
	return ResolveFormationMoveDestinationDetailed(
		UnitComponent,
		TargetActor,
		OutDestination,
		IgnoredFollowDistance,
		bIgnoredUsesSimpleObjectSettings,
		DefaultFollowDistance,
		MemberSpacing,
		bApplyMemberSlotOffset);
}

bool UAITCSDirectorSubsystem::ResolveFormationMoveDestinationDetailed(UAITCSTacticalUnitComponent* UnitComponent, AActor* TargetActor, FVector& OutDestination, float& OutFollowDistance, bool& bOutUsesSimpleObjectSettings, float DefaultFollowDistance, float MemberSpacing, bool bApplyMemberSlotOffset) const
{
	OutDestination = FVector::ZeroVector;
	OutFollowDistance = FMath::Max(0.0f, DefaultFollowDistance);
	bOutUsesSimpleObjectSettings = false;

	if (!UnitComponent || !TargetActor)
	{
		return false;
	}

	const FAITCSResolvedTacticalObjectData ObjectData = ResolveTacticalObjectDataForUnit(UnitComponent, DefaultFollowDistance);
	if (!ObjectData.bIsValid)
	{
		return false;
	}

	const FAITCSCompiledTacticalObject* CompiledObject = CompiledGraph.FindObject(ObjectData.TacticalObjectId);
	if (!CompiledObject)
	{
		return false;
	}

	const FAITCSCompiledTacticalObject* EffectiveObject = CompiledObject;
	bOutUsesSimpleObjectSettings = ObjectData.bIsSimpleObject;
	OutFollowDistance = ObjectData.FollowDistance;

	const AActor* UnitOwner = UnitComponent->GetOwner();
	if (UnitOwner && EffectiveObject->PlayerEngagementRadius > 0.0f)
	{
		const float EngagementRadiusSquared = FMath::Square(EffectiveObject->PlayerEngagementRadius);
		const float DistanceToTargetSquared = FVector::DistSquared2D(UnitOwner->GetActorLocation(), TargetActor->GetActorLocation());
		if (DistanceToTargetSquared > EngagementRadiusSquared)
		{
			OutDestination = EffectiveObject->bReturnToInitialLocationOutsideEngagementRadius ? UnitComponent->GetInitialLocation() : UnitOwner->GetActorLocation();
			return true;
		}
	}

	const float FollowDistance = ResolveFollowDistanceForObject(*EffectiveObject, DefaultFollowDistance);
	OutFollowDistance = FollowDistance;
	const FVector Direction = ResolveDirectionForObject(*EffectiveObject, TargetActor);
	OutDestination = TargetActor->GetActorLocation() + Direction * FollowDistance;

	if (bApplyMemberSlotOffset)
	{
		if (const FAITCSTacticalRuntimeGroupState* GroupState = RuntimeGroupStatesByObjectId.Find(ObjectData.TacticalObjectId))
		{
			// The Simple Object free pool is a membership/logging container, not a formation.
			if (!GroupState->bIsSimpleObjectFreePool)
			{
				const float ResolvedMemberSpacing = ResolveMemberSpacingForObject(*CompiledObject, MemberSpacing);
				OutDestination += ResolveMemberSlotOffset(*GroupState, UnitComponent->GetTacticalAgentId(), ResolvedMemberSpacing);
			}
		}
	}

	return true;
}

void UAITCSDirectorSubsystem::EnqueueTacticalOrder(const FAITCSTacticalOrder& Order)
{
	FAITCSTacticalOrder QueuedOrder = Order;
	if (!QueuedOrder.OrderId.IsValid())
	{
		QueuedOrder.OrderId = FGuid::NewGuid();
	}

	QueuedOrder.State = EAITCSOrderState::Pending;
	PendingOrders.Add(QueuedOrder);
}

int32 UAITCSDirectorSubsystem::DispatchTacticalOrder(const FAITCSTacticalOrder& Order)
{
	RefreshComponentAgentStates();

	FAITCSTacticalOrder DispatchedOrder = Order;
	if (!DispatchedOrder.OrderId.IsValid())
	{
		DispatchedOrder.OrderId = FGuid::NewGuid();
	}
	DispatchedOrder.State = EAITCSOrderState::Dispatched;

	if (DispatchedOrder.TargetAgentId.IsValid())
	{
		return DeliverOrderToAgent(DispatchedOrder.TargetAgentId, DispatchedOrder);
	}

	int32 DeliveredCount = 0;
	for (const TPair<FGuid, FAITCSTacticalAgentRuntimeState>& Pair : RegisteredAgentStates)
	{
		if (DoesOrderTargetAgent(DispatchedOrder, Pair.Value))
		{
			DeliveredCount += DeliverOrderToAgent(Pair.Key, DispatchedOrder);
		}
	}

	return DeliveredCount;
}

int32 UAITCSDirectorSubsystem::FlushPendingOrders()
{
	int32 DeliveredCount = 0;
	for (const FAITCSTacticalOrder& Order : PendingOrders)
	{
		DeliveredCount += DispatchTacticalOrder(Order);
	}

	PendingOrders.Reset();
	return DeliveredCount;
}

int32 UAITCSDirectorSubsystem::RequestDecisionProviderPass()
{
	if (!DecisionProviderObject || !DecisionProviderObject->GetClass()->ImplementsInterface(UAITCSTacticalDecisionProvider::StaticClass()))
	{
		return 0;
	}

	TArray<FAITCSTacticalOrder> ProviderOrders;
	const FAITCSTacticalContextSnapshot Snapshot = BuildContextSnapshot();
	IAITCSTacticalDecisionProvider::Execute_BuildTacticalOrders(DecisionProviderObject, Snapshot, ProviderOrders);

	for (const FAITCSTacticalOrder& Order : ProviderOrders)
	{
		EnqueueTacticalOrder(Order);
	}

	return ProviderOrders.Num();
}

bool UAITCSDirectorSubsystem::RegisterTacticalEvaluator(UObject* InEvaluator, const FAITCSTacticalEvaluatorRegistration& Registration)
{
	if (!IsValidEvaluatorObject(InEvaluator))
	{
		return false;
	}

	FAITCSTacticalEvaluatorRegistration EffectiveRegistration = Registration;
	EffectiveRegistration.EvaluatorName = ResolveEvaluatorName(InEvaluator, EffectiveRegistration);

	const int32 ExistingIndex = FindTacticalEvaluatorIndex(InEvaluator);
	if (TacticalEvaluators.IsValidIndex(ExistingIndex))
	{
		TacticalEvaluators[ExistingIndex].Registration = EffectiveRegistration;
		return true;
	}

	FAITCSTacticalRegisteredEvaluator& RegisteredEvaluator = TacticalEvaluators.AddDefaulted_GetRef();
	RegisteredEvaluator.EvaluatorObject = InEvaluator;
	RegisteredEvaluator.Registration = EffectiveRegistration;
	return true;
}

bool UAITCSDirectorSubsystem::UnregisterTacticalEvaluator(UObject* InEvaluator)
{
	const int32 ExistingIndex = FindTacticalEvaluatorIndex(InEvaluator);
	if (!TacticalEvaluators.IsValidIndex(ExistingIndex))
	{
		return false;
	}

	TacticalEvaluators.RemoveAt(ExistingIndex);
	return true;
}

bool UAITCSDirectorSubsystem::SetTacticalEvaluatorEnabled(UObject* InEvaluator, bool bEnabled)
{
	const int32 ExistingIndex = FindTacticalEvaluatorIndex(InEvaluator);
	if (!TacticalEvaluators.IsValidIndex(ExistingIndex))
	{
		return false;
	}

	TacticalEvaluators[ExistingIndex].Registration.bEnabled = bEnabled;
	return true;
}

void UAITCSDirectorSubsystem::ClearTacticalEvaluators()
{
	TacticalEvaluators.Reset();
	LastEvaluatorReports.Reset();
	LastEvaluatorProposals.Reset();
}

int32 UAITCSDirectorSubsystem::RequestEvaluatorPass()
{
	return RunEvaluatorPass(0.0f);
}

TArray<FAITCSTacticalEvaluatorReport> UAITCSDirectorSubsystem::GetLastEvaluatorReports() const
{
	return LastEvaluatorReports;
}

TArray<FAITCSTacticalOrderProposal> UAITCSDirectorSubsystem::GetLastEvaluatorProposals() const
{
	return LastEvaluatorProposals;
}

FAITCSTacticalAgentHandle UAITCSDirectorSubsystem::MakeHandleFromRegistration(const FAITCSTacticalAgentRegistration& Registration) const
{
	FAITCSTacticalAgentHandle Handle;
	Handle.AgentId = Registration.PreferredAgentId.IsValid() ? Registration.PreferredAgentId : FGuid::NewGuid();
	Handle.ObjectId = Registration.ObjectId;
	Handle.GroupId = Registration.GroupId;
	Handle.DebugName = Registration.DebugName;
	Handle.AgentKind = Registration.AgentKind;
	return Handle;
}

void UAITCSDirectorSubsystem::RefreshComponentAgentStates() const
{
	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;

	for (const TPair<FGuid, TWeakObjectPtr<UAITCSTacticalUnitComponent>>& Pair : RegisteredUnitComponents)
	{
		FAITCSTacticalAgentRuntimeState* RuntimeState = RegisteredAgentStates.Find(Pair.Key);
		const UAITCSTacticalUnitComponent* Component = Pair.Value.Get();
		if (!RuntimeState || !Component)
		{
			continue;
		}

		const AActor* Owner = Component->GetOwner();
		RuntimeState->Location = Owner ? Owner->GetActorLocation() : RuntimeState->Location;
		RuntimeState->Actor = const_cast<AActor*>(Owner);
		RuntimeState->bActive = Owner != nullptr;
		RuntimeState->LastUpdateTime = CurrentTime;

		FAITCSTacticalAgentRegistration RuntimeRegistration;
		RuntimeRegistration.PawnClass = RuntimeState->PawnClass;
		RuntimeRegistration.AIControllerClass = RuntimeState->AIControllerClass;
		PopulateRegistrationRuntimeClasses(RuntimeRegistration, Owner);
		RuntimeState->PawnClass = RuntimeRegistration.PawnClass;
		RuntimeState->AIControllerClass = RuntimeRegistration.AIControllerClass;
	}
}

int32 UAITCSDirectorSubsystem::DeliverOrderToAgent(const FGuid& AgentId, FAITCSTacticalOrder& Order)
{
	FAITCSTacticalAgentRuntimeState* RuntimeState = RegisteredAgentStates.Find(AgentId);
	if (!RuntimeState)
	{
		return 0;
	}

	LastOrdersByAgent.Add(AgentId, Order);

	if (RuntimeState->Handle.AgentKind == EAITCSAgentKind::ActorComponent)
	{
		if (TWeakObjectPtr<UAITCSTacticalUnitComponent>* ComponentPtr = RegisteredUnitComponents.Find(AgentId))
		{
			if (UAITCSTacticalUnitComponent* Component = ComponentPtr->Get())
			{
				Component->ReceiveTacticalOrder(Order);
				return 1;
			}
		}

		return 0;
	}

	return 1;
}

bool UAITCSDirectorSubsystem::DoesOrderTargetAgent(const FAITCSTacticalOrder& Order, const FAITCSTacticalAgentRuntimeState& AgentState) const
{
	if (Order.TargetObjectId.IsValid() && AgentState.Handle.ObjectId != Order.TargetObjectId)
	{
		return false;
	}

	if (Order.TargetGroupId.IsValid() && AgentState.Handle.GroupId != Order.TargetGroupId)
	{
		return false;
	}

	if (!Order.TargetTags.IsEmpty() && !AgentState.AgentTags.HasAny(Order.TargetTags))
	{
		return false;
	}

	return Order.TargetObjectId.IsValid() || Order.TargetGroupId.IsValid() || !Order.TargetTags.IsEmpty() || !Order.HasExplicitTarget();
}

int32 UAITCSDirectorSubsystem::RunEvaluatorPass(float DeltaSeconds)
{
	LastEvaluatorReports.Reset();
	LastEvaluatorProposals.Reset();

	if (!CompiledGraph.IsCompiled())
	{
		if (!CompileActiveTacticalGraph())
		{
			return 0;
		}
	}

	FAITCSTacticalEvaluationContext Context = BuildEvaluationContext(DeltaSeconds);
	EvaluationFrameCounter++;
	Context.EvaluationFrame = EvaluationFrameCounter;

	TArray<FAITCSRankedTacticalOrderProposal> RankedProposals;
	int32 ProposalSequence = 0;

	for (const FAITCSTacticalRegisteredEvaluator& RegisteredEvaluator : TacticalEvaluators)
	{
		UObject* EvaluatorObject = RegisteredEvaluator.EvaluatorObject.Get();

		FAITCSTacticalEvaluatorReport Report;
		Report.EvaluatorName = RegisteredEvaluator.Registration.EvaluatorName;
		Report.FeatureTag = RegisteredEvaluator.Registration.FeatureTag;
		Report.bEnabled = RegisteredEvaluator.Registration.bEnabled;

		const int32 ReportIndex = LastEvaluatorReports.Add(Report);

		if (!EvaluatorObject)
		{
			LastEvaluatorReports[ReportIndex].DebugSummary = TEXT("Evaluator object is invalid.");
			continue;
		}

		if (!IsValidEvaluatorObject(EvaluatorObject))
		{
			LastEvaluatorReports[ReportIndex].DebugSummary = TEXT("Evaluator object does not implement AITCSTacticalEvaluator.");
			continue;
		}

		LastEvaluatorReports[ReportIndex].EvaluatorName = ResolveEvaluatorName(EvaluatorObject, RegisteredEvaluator.Registration);

		bool bEvaluatorEnabled = RegisteredEvaluator.Registration.bEnabled;
		if (bEvaluatorEnabled)
		{
			bEvaluatorEnabled = IAITCSTacticalEvaluator::Execute_IsTacticalEvaluatorEnabled(EvaluatorObject);
		}

		LastEvaluatorReports[ReportIndex].bEnabled = bEvaluatorEnabled;
		if (!bEvaluatorEnabled)
		{
			LastEvaluatorReports[ReportIndex].DebugSummary = TEXT("Evaluator is disabled.");
			continue;
		}

		TArray<FAITCSTacticalOrderProposal> EvaluatorProposals;
		const double StartTimeSeconds = FPlatformTime::Seconds();
		IAITCSTacticalEvaluator::Execute_EvaluateTacticalState(EvaluatorObject, Context, EvaluatorProposals);
		const double EndTimeSeconds = FPlatformTime::Seconds();

		LastEvaluatorReports[ReportIndex].EvaluationTimeSeconds = static_cast<float>(EndTimeSeconds - StartTimeSeconds);
		LastEvaluatorReports[ReportIndex].ProposalsGenerated = EvaluatorProposals.Num();

		for (FAITCSTacticalOrderProposal& Proposal : EvaluatorProposals)
		{
			if (Proposal.EvaluatorName.IsNone())
			{
				Proposal.EvaluatorName = LastEvaluatorReports[ReportIndex].EvaluatorName;
			}

			if (!Proposal.FeatureTag.IsValid())
			{
				Proposal.FeatureTag = RegisteredEvaluator.Registration.FeatureTag;
			}

			if (!Proposal.IsDispatchable())
			{
				continue;
			}

			if (Proposal.Score < MinimumEvaluatorProposalScore)
			{
				continue;
			}

			FAITCSRankedTacticalOrderProposal& RankedProposal = RankedProposals.AddDefaulted_GetRef();
			RankedProposal.Proposal = Proposal;
			RankedProposal.WeightedScore = Proposal.GetWeightedScore(RegisteredEvaluator.Registration.Weight);
			RankedProposal.EvaluatorSortPriority = RegisteredEvaluator.Registration.SortPriority;
			RankedProposal.Sequence = ProposalSequence;
			RankedProposal.ReportIndex = ReportIndex;
			ProposalSequence++;
		}
	}

	RankedProposals.Sort(FAITCSRankedTacticalOrderProposalSort());

	int32 AcceptedCount = 0;
	for (FAITCSRankedTacticalOrderProposal& RankedProposal : RankedProposals)
	{
		if (MaxEvaluatorOrdersPerPass > 0 && AcceptedCount >= MaxEvaluatorOrdersPerPass)
		{
			break;
		}

		RankedProposal.Proposal.PrepareForDispatch();
		LastEvaluatorProposals.Add(RankedProposal.Proposal);
		EnqueueTacticalOrder(RankedProposal.Proposal.Order);
		AcceptedCount++;

		if (LastEvaluatorReports.IsValidIndex(RankedProposal.ReportIndex))
		{
			LastEvaluatorReports[RankedProposal.ReportIndex].ProposalsAccepted++;
		}
	}

	return AcceptedCount;
}

FAITCSTacticalEvaluationContext UAITCSDirectorSubsystem::BuildEvaluationContext(float DeltaSeconds) const
{
	FAITCSTacticalEvaluationContext Context;
	Context.CompiledGraph = &CompiledGraph;
	Context.Snapshot = BuildContextSnapshot();
	Context.DeltaSeconds = DeltaSeconds;

	if (const UWorld* World = GetWorld())
	{
		Context.WorldTimeSeconds = World->GetTimeSeconds();
	}

	return Context;
}

int32 UAITCSDirectorSubsystem::FindTacticalEvaluatorIndex(const UObject* InEvaluator) const
{
	if (!InEvaluator)
	{
		return INDEX_NONE;
	}

	for (int32 EvaluatorIndex = 0; EvaluatorIndex < TacticalEvaluators.Num(); ++EvaluatorIndex)
	{
		if (TacticalEvaluators[EvaluatorIndex].EvaluatorObject.Get() == InEvaluator)
		{
			return EvaluatorIndex;
		}
	}

	return INDEX_NONE;
}

bool UAITCSDirectorSubsystem::IsValidEvaluatorObject(const UObject* InEvaluator) const
{
	return InEvaluator && InEvaluator->GetClass() && InEvaluator->GetClass()->ImplementsInterface(UAITCSTacticalEvaluator::StaticClass());
}

FName UAITCSDirectorSubsystem::ResolveEvaluatorName(UObject* InEvaluator, const FAITCSTacticalEvaluatorRegistration& Registration) const
{
	if (!Registration.EvaluatorName.IsNone())
	{
		return Registration.EvaluatorName;
	}

	if (IsValidEvaluatorObject(InEvaluator))
	{
		const FName InterfaceName = IAITCSTacticalEvaluator::Execute_GetTacticalEvaluatorName(InEvaluator);
		if (!InterfaceName.IsNone())
		{
			return InterfaceName;
		}
	}

	return InEvaluator ? InEvaluator->GetFName() : NAME_None;
}

bool UAITCSDirectorSubsystem::ResolveRegistrationAssignment(FAITCSTacticalAgentRegistration& InOutRegistration)
{
	if (!CompiledGraph.IsCompiled() && ActiveGraphAsset)
	{
		CompileActiveTacticalGraph();
	}

	if (!CompiledGraph.IsCompiled())
	{
		return false;
	}

	if (InOutRegistration.ObjectId.IsValid())
	{
		const FAITCSCompiledTacticalObject* CompiledObject = CompiledGraph.FindObject(InOutRegistration.ObjectId);
		if (!CompiledObject || !CanCompiledObjectAcceptRegistration(*CompiledObject, InOutRegistration))
		{
			return false;
		}

		if (CompiledObject && !InOutRegistration.GroupId.IsValid() && CompiledObject->bHasActiveGroup)
		{
			InOutRegistration.GroupId = CompiledObject->ActiveGroupId;
		}

		return true;
	}

	FGuid ResolvedObjectId;
	FGameplayTag ResolvedGroupId;
	if (TryFindBestRuntimeGroupForRegistration(InOutRegistration, ResolvedObjectId, ResolvedGroupId))
	{
		InOutRegistration.ObjectId = ResolvedObjectId;
		if (!InOutRegistration.GroupId.IsValid())
		{
			InOutRegistration.GroupId = ResolvedGroupId;
		}

		return true;
	}

	if (TryFindSimpleObjectFallbackForRegistration(InOutRegistration, ResolvedObjectId))
	{
		InOutRegistration.ObjectId = ResolvedObjectId;
		InOutRegistration.GroupId = FGameplayTag();
		return true;
	}

	return false;
}

bool UAITCSDirectorSubsystem::TryFindBestRuntimeGroupForRegistration(const FAITCSTacticalAgentRegistration& Registration, FGuid& OutObjectId, FGameplayTag& OutGroupId) const
{
	OutObjectId = FGuid();
	OutGroupId = FGameplayTag();

	if (!CompiledGraph.IsCompiled())
	{
		return false;
	}

	const bool bAllowOnlyUniqueFallback = !Registration.GroupId.IsValid() && !Registration.ObjectId.IsValid();
	int32 CandidateCount = 0;
	int32 BestMemberCount = MAX_int32;
	int32 BestFreeSlots = -1;
	FGuid BestObjectId;
	FGameplayTag BestGroupId;

	for (const FAITCSCompiledTacticalObject& CompiledObject : CompiledGraph.Objects)
	{
		const FAITCSTacticalRuntimeGroupState* GroupState = RuntimeGroupStatesByObjectId.Find(CompiledObject.ObjectId);
		if (!GroupState || GroupState->bIsSimpleObjectFreePool)
		{
			continue;
		}

		if (!CanRuntimeGroupAcceptRegistration(*GroupState, Registration))
		{
			continue;
		}

		CandidateCount++;
		const int32 MemberCount = GroupState->MemberAgentIds.Num();
		const int32 FreeSlots = GroupState->GetFreeSlotCount();
		const bool bIsBetterMemberCount = MemberCount < BestMemberCount;
		const bool bIsBetterFreeSlots = MemberCount == BestMemberCount && FreeSlots > BestFreeSlots;

		if (!BestObjectId.IsValid() || bIsBetterMemberCount || bIsBetterFreeSlots)
		{
			BestObjectId = GroupState->TacticalObjectId;
			BestGroupId = GroupState->GroupId;
			BestMemberCount = MemberCount;
			BestFreeSlots = FreeSlots;
		}
	}

	if (bAllowOnlyUniqueFallback && CandidateCount != 1)
	{
		return false;
	}

	if (!BestObjectId.IsValid())
	{
		return false;
	}

	OutObjectId = BestObjectId;
	OutGroupId = BestGroupId;
	return true;
}

bool UAITCSDirectorSubsystem::TryFindSimpleObjectFallbackForRegistration(const FAITCSTacticalAgentRegistration& Registration, FGuid& OutObjectId) const
{
	OutObjectId = FGuid();

	if (!CompiledGraph.IsCompiled() || Registration.GroupId.IsValid())
	{
		return false;
	}

	const FAITCSCompiledTacticalObject* BestSimpleObject = nullptr;
	int32 BestMemberCount = MAX_int32;
	for (const FAITCSCompiledTacticalObject& CompiledObject : CompiledGraph.Objects)
	{
		if (!CompiledObject.bIsSimpleObject || !CanCompiledObjectAcceptRegistration(CompiledObject, Registration))
		{
			continue;
		}

		int32 MemberCount = 0;
		if (const FAITCSTacticalRuntimeGroupState* GroupState = RuntimeGroupStatesByObjectId.Find(CompiledObject.ObjectId))
		{
			MemberCount = GroupState->MemberAgentIds.Num();
		}

		if (!BestSimpleObject || MemberCount < BestMemberCount)
		{
			BestSimpleObject = &CompiledObject;
			BestMemberCount = MemberCount;
		}
	}

	if (!BestSimpleObject)
	{
		return false;
	}

	OutObjectId = BestSimpleObject->ObjectId;
	return OutObjectId.IsValid();
}

bool UAITCSDirectorSubsystem::CanRuntimeGroupAcceptRegistration(const FAITCSTacticalRuntimeGroupState& GroupState, const FAITCSTacticalAgentRegistration& Registration) const
{
	if (!GroupState.HasFreeSlot())
	{
		return false;
	}

	if (Registration.ObjectId.IsValid() && Registration.ObjectId != GroupState.TacticalObjectId)
	{
		return false;
	}

	if (Registration.GroupId.IsValid() && Registration.GroupId != GroupState.GroupId)
	{
		return false;
	}

	const FAITCSCompiledTacticalObject* CompiledObject = CompiledGraph.FindObject(GroupState.TacticalObjectId);
	if (!CompiledObject || !CanCompiledObjectAcceptRegistration(*CompiledObject, Registration))
	{
		return false;
	}

	return GroupState.TacticalObjectId.IsValid()
		&& (GroupState.GroupId.IsValid() || GroupState.bIsSimpleObjectFreePool);
}

bool UAITCSDirectorSubsystem::CanCompiledObjectAcceptRegistration(const FAITCSCompiledTacticalObject& CompiledObject, const FAITCSTacticalAgentRegistration& Registration) const
{
	return DoesAITCSRuntimeClassMatch(CompiledObject.PawnClass, Registration.PawnClass)
		&& DoesAITCSRuntimeClassMatch(CompiledObject.AIControllerClass, Registration.AIControllerClass);
}

void UAITCSDirectorSubsystem::PopulateRegistrationRuntimeClasses(FAITCSTacticalAgentRegistration& InOutRegistration, const AActor* AgentActor) const
{
	if (!AgentActor)
	{
		return;
	}

	if (const AAIController* AIController = Cast<AAIController>(AgentActor))
	{
		InOutRegistration.AIControllerClass = AIController->GetClass();
		if (const APawn* ControlledPawn = AIController->GetPawn())
		{
			InOutRegistration.PawnClass = ControlledPawn->GetClass();
		}
		return;
	}

	if (const APawn* Pawn = Cast<APawn>(AgentActor))
	{
		InOutRegistration.PawnClass = Pawn->GetClass();

		if (const AAIController* AIController = Cast<AAIController>(Pawn->GetController()))
		{
			InOutRegistration.AIControllerClass = AIController->GetClass();
		}
		else if (UClass* ConfiguredControllerClass = Pawn->AIControllerClass.Get())
		{
			if (ConfiguredControllerClass->IsChildOf(AAIController::StaticClass()))
			{
				InOutRegistration.AIControllerClass = ConfiguredControllerClass;
			}
		}
	}
}

void UAITCSDirectorSubsystem::AddAgentToRuntimeGroupState(const FAITCSTacticalAgentRuntimeState& AgentState)
{
	if (!AgentState.Handle.AgentId.IsValid() || !AgentState.Handle.ObjectId.IsValid())
	{
		return;
	}

	FAITCSTacticalRuntimeGroupState* GroupState = RuntimeGroupStatesByObjectId.Find(AgentState.Handle.ObjectId);
	if (!GroupState)
	{
		return;
	}

	for (const FGuid& ExistingAgentId : GroupState->MemberAgentIds)
	{
		if (ExistingAgentId == AgentState.Handle.AgentId)
		{
			return;
		}
	}

	GroupState->MemberAgentIds.Add(AgentState.Handle.AgentId);
}

void UAITCSDirectorSubsystem::BuildRuntimeGroupStateFromCompiledObject(const FAITCSCompiledTacticalObject& CompiledObject)
{
	const bool bIsSimpleObjectFreePool = CompiledObject.bIsSimpleObject
		&& (!CompiledObject.bHasActiveGroup || !CompiledObject.ActiveGroupId.IsValid());
	if ((!CompiledObject.bHasActiveGroup || !CompiledObject.ActiveGroupId.IsValid()) && !bIsSimpleObjectFreePool)
	{
		return;
	}

	FAITCSTacticalRuntimeGroupState& GroupState = RuntimeGroupStatesByObjectId.Add(CompiledObject.ObjectId);
	GroupState.TacticalObjectId = CompiledObject.ObjectId;
	GroupState.TacticalObjectName = CompiledObject.DisplayName;
	GroupState.GroupId = CompiledObject.ActiveGroupId;
	GroupState.bIsSimpleObjectFreePool = bIsSimpleObjectFreePool;
	if (bIsSimpleObjectFreePool)
	{
		GroupState.GroupDisplayName = FText::FromString(TEXT("Simple Object Free Pool"));
		GroupState.ExpectedSize = 0;
		GroupState.MaxMembers = 0;
	}
	else
	{
		GroupState.GroupDisplayName = CompiledObject.ActiveGroup.DisplayName;
		GroupState.ExpectedSize = CompiledObject.ActiveGroup.ExpectedSize;
		GroupState.MaxMembers = CompiledObject.ActiveGroup.MaxMembers;
	}
}

FAITCSResolvedTacticalObjectData UAITCSDirectorSubsystem::ResolveTacticalObjectDataForUnit(UAITCSTacticalUnitComponent* UnitComponent, float DefaultFollowDistance) const
{
	FAITCSResolvedTacticalObjectData Result;

	if (!UnitComponent || !CompiledGraph.IsCompiled())
	{
		return Result;
	}

	FGuid ObjectId = UnitComponent->GetTacticalObjectId();
	if (const FAITCSTacticalAgentRuntimeState *RuntimeState = RegisteredAgentStates.Find(UnitComponent->GetTacticalAgentId()))
	{
		// The Director's registration state is authoritative. In particular, a
		// Simple Object has a valid ObjectId but intentionally has no GroupId.
		if (RuntimeState->Handle.ObjectId.IsValid())
		{
			ObjectId = RuntimeState->Handle.ObjectId;
		}
	}
	const FAITCSCompiledTacticalObject* CompiledObject = ObjectId.IsValid() ? CompiledGraph.FindObject(ObjectId) : nullptr;
	const bool bHasExplicitCompiledObject = CompiledObject != nullptr;
	if (!CompiledObject)
	{
		// A unit can be queried before the deferred registration pass. Resolve only
		// an unambiguous runtime candidate; never silently expose Simple Object data
		// for an already assigned Tactical Object.
		FAITCSTacticalAgentRegistration RuntimeRegistration = UnitComponent->BuildRegistration();
		PopulateRegistrationRuntimeClasses(RuntimeRegistration, UnitComponent->GetOwner());

		FGuid ResolvedObjectId;
		FGameplayTag ResolvedGroupId;
		if (!TryFindBestRuntimeGroupForRegistration(RuntimeRegistration, ResolvedObjectId, ResolvedGroupId)
			&& !TryFindSimpleObjectFallbackForRegistration(RuntimeRegistration, ResolvedObjectId))
		{
			return Result;
		}

		CompiledObject = CompiledGraph.FindObject(ResolvedObjectId);
	}

	if (!CompiledObject)
	{
		return Result;
	}

	FAITCSTacticalAgentRegistration RuntimeRegistration = UnitComponent->BuildRegistration();
	PopulateRegistrationRuntimeClasses(RuntimeRegistration, UnitComponent->GetOwner());
	// A valid ObjectId is an authoritative assignment made by the Director. Do
	// not discard its authored metadata just because the pawn/controller class is
	// temporarily unavailable during StateTree evaluation. Class matching is
	// required only for the pre-assignment fallback path.
	if (!bHasExplicitCompiledObject && !CanCompiledObjectAcceptRegistration(*CompiledObject, RuntimeRegistration))
	{
		return Result;
	}

	Result.bIsValid = true;
	Result.TacticalObjectId = CompiledObject->ObjectId;
	Result.TacticalObjectName = CompiledObject->DisplayName;
	Result.RoleTag = CompiledObject->RoleTag;
	Result.DesiredPlayerDistance = FMath::Max(0.0f, CompiledObject->DesiredPlayerDistance);
	Result.FollowDistance = ResolveFollowDistanceForObject(*CompiledObject, DefaultFollowDistance);
	Result.AttackTargetDistance = FMath::Max(0.0f, CompiledObject->AttackTargetDistance);
	Result.bUseAttackTargetDistanceAsFormationOverride = CompiledObject->bUseAttackTargetDistanceAsFormationOverride;
	Result.bIsSimpleObject = CompiledObject->bIsSimpleObject;
	Result.bHasActiveGroup = CompiledObject->bHasActiveGroup && CompiledObject->ActiveGroupId.IsValid();
	Result.bIsSimpleObjectFreePool = Result.bIsSimpleObject && !Result.bHasActiveGroup;
	Result.GroupId = CompiledObject->ActiveGroupId;
	return Result;
}

FAITCSResolvedTacticalObjectData UAITCSDirectorSubsystem::ResolveTacticalObjectDataForActor(const AActor* AgentActor, float DefaultFollowDistance) const
{
	if (!AgentActor)
	{
		return FAITCSResolvedTacticalObjectData();
	}

	for (const TPair<FGuid, TWeakObjectPtr<UAITCSTacticalUnitComponent>>& Pair : RegisteredUnitComponents)
	{
		UAITCSTacticalUnitComponent* UnitComponent = Pair.Value.Get();
		const AActor* RegisteredActor = UnitComponent ? UnitComponent->GetOwner() : nullptr;
		if (!RegisteredActor)
		{
			continue;
		}

		bool bRelatedActor = RegisteredActor == AgentActor;
		if (const AController* RegisteredController = Cast<AController>(RegisteredActor))
		{
			bRelatedActor = bRelatedActor || RegisteredController->GetPawn() == AgentActor;
		}
		else if (const APawn* RegisteredPawn = Cast<APawn>(RegisteredActor))
		{
			bRelatedActor = bRelatedActor || RegisteredPawn->GetController() == AgentActor;
		}

		if (bRelatedActor)
		{
			return ResolveTacticalObjectDataForUnit(UnitComponent, DefaultFollowDistance);
		}
	}

	return FAITCSResolvedTacticalObjectData();
}

bool UAITCSDirectorSubsystem::ResolveObjectMetadataForUnit(UAITCSTacticalUnitComponent* UnitComponent, FGameplayTag& OutRoleTag, float& OutAttackTargetDistance, bool& bOutIsSimpleObject) const
{
	const FAITCSResolvedTacticalObjectData ObjectData = ResolveTacticalObjectDataForUnit(UnitComponent);
	OutRoleTag = ObjectData.RoleTag;
	OutAttackTargetDistance = ObjectData.AttackTargetDistance;
	bOutIsSimpleObject = ObjectData.bIsSimpleObject;
	return ObjectData.bIsValid;
}

float UAITCSDirectorSubsystem::ResolveFollowDistanceForObject(const FAITCSCompiledTacticalObject& CompiledObject, float DefaultFollowDistance, const FAITCSCompiledTacticalObject* FallbackObject) const
{
	if (CompiledObject.bUseAttackTargetDistanceAsFormationOverride && CompiledObject.AttackTargetDistance > 0.0f)
	{
		return FMath::Max(0.0f, CompiledObject.AttackTargetDistance);
	}

	if (FallbackObject && FallbackObject->bUseAttackTargetDistanceAsFormationOverride && FallbackObject->AttackTargetDistance > 0.0f)
	{
		return FMath::Max(0.0f, FallbackObject->AttackTargetDistance);
	}

	if (CompiledObject.DesiredPlayerDistance > 0.0f)
	{
		return FMath::Max(0.0f, CompiledObject.DesiredPlayerDistance);
	}

	if (FallbackObject && FallbackObject->DesiredPlayerDistance > 0.0f)
	{
		return FMath::Max(0.0f, FallbackObject->DesiredPlayerDistance);
	}

	return FMath::Max(0.0f, DefaultFollowDistance);
}

float UAITCSDirectorSubsystem::ResolveMemberSpacingForObject(const FAITCSCompiledTacticalObject& CompiledObject, float DefaultMemberSpacing, const FAITCSCompiledTacticalObject* FallbackObject) const
{
	if (CompiledObject.ActiveGroup.FormationMemberSpacing > 0.0f)
	{
		return FMath::Max(0.0f, CompiledObject.ActiveGroup.FormationMemberSpacing);
	}

	if (FallbackObject && FallbackObject->ActiveGroup.FormationMemberSpacing > 0.0f)
	{
		return FMath::Max(0.0f, FallbackObject->ActiveGroup.FormationMemberSpacing);
	}

	return FMath::Max(0.0f, DefaultMemberSpacing);
}

FVector UAITCSDirectorSubsystem::ResolveDirectionForObject(const FAITCSCompiledTacticalObject& CompiledObject, const AActor* TargetActor) const
{
	float DirectionDegrees = CompiledObject.DirectionSector.CompiledCenterDegrees;
	if (CompiledObject.DirectionSector.DirectionSpace == EAITCSTacticalDirectionSpace::RelativeToPlayer && TargetActor)
	{
		DirectionDegrees += TargetActor->GetActorRotation().Yaw;
	}

	const float DirectionRadians = FMath::DegreesToRadians(DirectionDegrees);
	FVector Direction(FMath::Cos(DirectionRadians), FMath::Sin(DirectionRadians), 0.0f);
	if (!Direction.Normalize())
	{
		return FVector::ForwardVector;
	}

	return Direction;
}

FVector UAITCSDirectorSubsystem::ResolveMemberSlotOffset(const FAITCSTacticalRuntimeGroupState& GroupState, const FGuid& AgentId, float MemberSpacing) const
{
	if (!AgentId.IsValid() || MemberSpacing <= 0.0f)
	{
		return FVector::ZeroVector;
	}

	int32 MemberIndex = INDEX_NONE;
	for (int32 Index = 0; Index < GroupState.MemberAgentIds.Num(); ++Index)
	{
		if (GroupState.MemberAgentIds[Index] == AgentId)
		{
			MemberIndex = Index;
			break;
		}
	}

	if (MemberIndex == INDEX_NONE)
	{
		return FVector::ZeroVector;
	}

	const int32 SlotCount = GroupState.MaxMembers > 0 ? GroupState.MaxMembers : FMath::Max(1, GroupState.MemberAgentIds.Num());
	const float SlotAngleDegrees = 360.0f * static_cast<float>(MemberIndex) / static_cast<float>(FMath::Max(1, SlotCount));
	const float SlotAngleRadians = FMath::DegreesToRadians(SlotAngleDegrees);
	return FVector(FMath::Cos(SlotAngleRadians) * MemberSpacing, FMath::Sin(SlotAngleRadians) * MemberSpacing, 0.0f);
}

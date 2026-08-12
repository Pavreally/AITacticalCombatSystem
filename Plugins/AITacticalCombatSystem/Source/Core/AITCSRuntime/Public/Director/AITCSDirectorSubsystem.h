// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/AITCSCoreData.h"
#include "Features/AITCSTacticalFeatureTypes.h"
#include "Graph/AITCSCompiledTacticalGraph.h"

#include "AITCSDirectorSubsystem.generated.h"

class UAITCSTacticalGraphAsset;
class UAITCSTacticalUnitComponent;
class AActor;

/**
 * Runtime director subsystem responsible for tactical graph compilation, agent
 * registration, order dispatch, and evaluation coordination.
 */
UCLASS(BlueprintType)
class AITCSRUNTIME_API UAITCSDirectorSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Initialize the director subsystem and register required world-level services. */
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/** Clean up runtime state and unregister any dependencies. */
	virtual void Deinitialize() override;

	/** Tick the director each frame to update evaluators and runtime group state. */
	virtual void Tick(float DeltaTime) override;

	/** Return the stats ID for this tickable subsystem. */
	virtual TStatId GetStatId() const override;

	/** Whether this subsystem should be ticked. */
	virtual bool IsTickable() const override;

	/** Set the active tactical graph asset used by the director. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Director")
	bool SetActiveTacticalGraph(UAITCSTacticalGraphAsset* InGraphAsset);

	/** Get the currently active tactical graph asset. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Director")
	UAITCSTacticalGraphAsset* GetActiveTacticalGraph() const;

	/** Compile the active graph asset into runtime data structures. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Graph")
	bool CompileActiveTacticalGraph();

	/** Check whether the active tactical graph has been successfully compiled. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Graph")
	bool IsTacticalGraphCompiled() const;

	/** Return a copy of the compiled tactical graph for safe access. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Graph")
	FAITCSCompiledTacticalGraph GetCompiledTacticalGraphCopy() const;

	/** Return the compiled tactical graph by reference. */
	const FAITCSCompiledTacticalGraph& GetCompiledTacticalGraph() const;

	/** Retrieve the last graph compile validation messages. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Graph")
	TArray<FAITCSGraphValidationMessage> GetLastGraphCompileMessages() const;

	/** Assign an external decision provider object used to build tactical orders. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Director")
	bool SetDecisionProvider(UObject* InDecisionProvider);

	/** Get the currently assigned decision provider object. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Director")
	UObject* GetDecisionProvider() const;

	/** Register a unit component with the director and allocate a tactical handle. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	bool RegisterTacticalUnit(UAITCSTacticalUnitComponent* UnitComponent, const FAITCSTacticalAgentRegistration& Registration, FAITCSTacticalAgentHandle& OutHandle);

	/** Unregister a tactical unit by its agent identifier. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	bool UnregisterTacticalUnit(const FGuid& AgentId);

	/** Register a virtual tactical agent without requiring a component instance. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	bool RegisterVirtualTacticalAgent(const FAITCSTacticalAgentRegistration& Registration, FAITCSTacticalAgentHandle& OutHandle);

	/** Update the registration data for an existing virtual tactical agent. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	bool UpdateVirtualTacticalAgent(const FGuid& AgentId, const FAITCSTacticalAgentRegistration& Registration);

	/** Unregister a virtual tactical agent by its identifier. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	bool UnregisterVirtualTacticalAgent(const FGuid& AgentId);

	/** Create a snapshot of the current tactical context for evaluators and debugging. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Context")
	FAITCSTacticalContextSnapshot BuildContextSnapshot() const;

	/** Get the runtime state for all registered tactical agents. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Context")
	TArray<FAITCSTacticalAgentRuntimeState> GetRegisteredAgentStates() const;

	/** Get runtime state information for all tactical groups. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Groups")
	TArray<FAITCSTacticalRuntimeGroupState> GetRuntimeGroupStates() const;

	/** Rebuild runtime group state based on current registrations and compiled graph data. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Groups")
	void RebuildRuntimeGroupStates();

	/** Resolve assignments for all registered agents, optionally overwriting existing assignments. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Groups")
	int32 ResolveRegisteredAgentAssignments(bool bOverwriteExistingAssignments = false);

	/**
	 * Resolve a destination for a unit in formation using the active tactical graph.
	 * @param UnitComponent The unit component requesting formation movement.
	 * @param TargetActor The actor that the unit should move toward.
	 * @param OutDestination Resulting movement destination.
	 * @param DefaultFollowDistance Fallback follow distance when no specific object assignment exists.
	 * @param MemberSpacing Spacing between formation members.
	 * @param bApplyMemberSlotOffset Whether to apply slot-based formation offsets.
	 */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Formation")
	bool ResolveFormationMoveDestination(UAITCSTacticalUnitComponent* UnitComponent, AActor* TargetActor, FVector& OutDestination, float DefaultFollowDistance = 800.0f, float MemberSpacing = 175.0f, bool bApplyMemberSlotOffset = true) const;

	/** Detailed formation resolution that also returns follow distance and simple object metadata. */
	bool ResolveFormationMoveDestinationDetailed(UAITCSTacticalUnitComponent* UnitComponent, AActor* TargetActor, FVector& OutDestination, float& OutFollowDistance, bool& bOutUsesSimpleObjectSettings, float DefaultFollowDistance = 800.0f, float MemberSpacing = 175.0f, bool bApplyMemberSlotOffset = true) const;

	/** Resolve the object data for a given unit component during runtime. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Object")
	FAITCSResolvedTacticalObjectData ResolveTacticalObjectDataForUnit(UAITCSTacticalUnitComponent* UnitComponent, float DefaultFollowDistance = 800.0f) const;

	/** Resolve the object data for a given actor instance. */
	FAITCSResolvedTacticalObjectData ResolveTacticalObjectDataForActor(const AActor* AgentActor, float DefaultFollowDistance = 800.0f) const;

	/** Extract object metadata for a unit component without allocating runtime state. */
	bool ResolveObjectMetadataForUnit(UAITCSTacticalUnitComponent* UnitComponent, FGameplayTag& OutRoleTag, float& OutAttackTargetDistance, bool& bOutIsSimpleObject) const;

	/** Enqueue a tactical order for later dispatch. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Orders")
	void EnqueueTacticalOrder(const FAITCSTacticalOrder& Order);

	/** Dispatch a tactical order immediately to matching agents. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Orders")
	int32 DispatchTacticalOrder(const FAITCSTacticalOrder& Order);

	/** Flush all pending tactical orders from the director queue. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Orders")
	int32 FlushPendingOrders();

	/** Request a single decision provider pass on the next tick. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Decision Provider")
	int32 RequestDecisionProviderPass();

	/** Register an evaluator object with the director. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Evaluators")
	bool RegisterTacticalEvaluator(UObject* InEvaluator, const FAITCSTacticalEvaluatorRegistration& Registration);

	/** Unregister a previously registered evaluator object. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Evaluators")
	bool UnregisterTacticalEvaluator(UObject* InEvaluator);

	/** Enable or disable a registered evaluator. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Evaluators")
	bool SetTacticalEvaluatorEnabled(UObject* InEvaluator, bool bEnabled);

	/** Clear all runtime evaluators from the director. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Evaluators")
	void ClearTacticalEvaluators();

	/** Trigger an evaluator pass immediately. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Evaluators")
	int32 RequestEvaluatorPass();

	/** Get the last generated evaluator reports. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Evaluators")
	TArray<FAITCSTacticalEvaluatorReport> GetLastEvaluatorReports() const;

	/** Get the latest proposals generated by evaluators. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Evaluators")
	TArray<FAITCSTacticalOrderProposal> GetLastEvaluatorProposals() const;

	/** Run the decision provider on every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Decision Provider")
	bool bRunDecisionProviderOnTick = false;

	/** How often the director should request a decision provider pass when ticking. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Decision Provider", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float DecisionProviderTickInterval = 0.5f;

	/** Options for compiled tactical graph building. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Graph")
	FAITCSCompiledTacticalGraphBuildOptions GraphCompileOptions;

	/** Run evaluator passes on every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Evaluators")
	bool bRunEvaluatorsOnTick = false;

	/** Interval between evaluator passes when ticked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Evaluators", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float EvaluatorTickInterval = 0.5f;

	/** Maximum number of evaluator orders processed per pass. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Evaluators", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxEvaluatorOrdersPerPass = 64;

	/** Minimum score required for evaluator proposals to be considered. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Evaluators")
	float MinimumEvaluatorProposalScore = 0.0f;

private:
	FAITCSTacticalAgentHandle MakeHandleFromRegistration(const FAITCSTacticalAgentRegistration& Registration) const;
	void RefreshComponentAgentStates() const;
	int32 DeliverOrderToAgent(const FGuid& AgentId, FAITCSTacticalOrder& Order);
	bool DoesOrderTargetAgent(const FAITCSTacticalOrder& Order, const FAITCSTacticalAgentRuntimeState& AgentState) const;
	int32 RunEvaluatorPass(float DeltaSeconds);
	FAITCSTacticalEvaluationContext BuildEvaluationContext(float DeltaSeconds) const;
	int32 FindTacticalEvaluatorIndex(const UObject* InEvaluator) const;
	bool IsValidEvaluatorObject(const UObject* InEvaluator) const;
	FName ResolveEvaluatorName(UObject* InEvaluator, const FAITCSTacticalEvaluatorRegistration& Registration) const;
	bool ResolveRegistrationAssignment(FAITCSTacticalAgentRegistration& InOutRegistration);
	bool TryFindBestRuntimeGroupForRegistration(const FAITCSTacticalAgentRegistration& Registration, FGuid& OutObjectId, FGameplayTag& OutGroupId) const;
	bool TryFindSimpleObjectFallbackForRegistration(const FAITCSTacticalAgentRegistration& Registration, FGuid& OutObjectId) const;
	bool CanRuntimeGroupAcceptRegistration(const FAITCSTacticalRuntimeGroupState& GroupState, const FAITCSTacticalAgentRegistration& Registration) const;
	bool CanCompiledObjectAcceptRegistration(const FAITCSCompiledTacticalObject& CompiledObject, const FAITCSTacticalAgentRegistration& Registration) const;
	void PopulateRegistrationRuntimeClasses(FAITCSTacticalAgentRegistration& InOutRegistration, const AActor* AgentActor) const;
	void AddAgentToRuntimeGroupState(const FAITCSTacticalAgentRuntimeState& AgentState);
	void BuildRuntimeGroupStateFromCompiledObject(const FAITCSCompiledTacticalObject& CompiledObject);
	float ResolveFollowDistanceForObject(const FAITCSCompiledTacticalObject& CompiledObject, float DefaultFollowDistance, const FAITCSCompiledTacticalObject* FallbackObject = nullptr) const;
	float ResolveMemberSpacingForObject(const FAITCSCompiledTacticalObject& CompiledObject, float DefaultMemberSpacing, const FAITCSCompiledTacticalObject* FallbackObject = nullptr) const;
	FVector ResolveDirectionForObject(const FAITCSCompiledTacticalObject& CompiledObject, const AActor* TargetActor) const;
	FVector ResolveMemberSlotOffset(const FAITCSTacticalRuntimeGroupState& GroupState, const FGuid& AgentId, float MemberSpacing) const;

	UPROPERTY(Transient)
	TObjectPtr<UAITCSTacticalGraphAsset> ActiveGraphAsset = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UObject> DecisionProviderObject = nullptr;

	UPROPERTY(Transient)
	FAITCSCompiledTacticalGraph CompiledGraph;

	UPROPERTY(Transient)
	TArray<FAITCSGraphValidationMessage> LastGraphCompileMessages;

	UPROPERTY(Transient)
	TArray<FAITCSTacticalRegisteredEvaluator> TacticalEvaluators;

	UPROPERTY(Transient)
	TArray<FAITCSTacticalEvaluatorReport> LastEvaluatorReports;

	UPROPERTY(Transient)
	TArray<FAITCSTacticalOrderProposal> LastEvaluatorProposals;

	TMap<FGuid, TWeakObjectPtr<UAITCSTacticalUnitComponent>> RegisteredUnitComponents;
	mutable TMap<FGuid, FAITCSTacticalAgentRuntimeState> RegisteredAgentStates;
	TMap<FGuid, FAITCSTacticalRuntimeGroupState> RuntimeGroupStatesByObjectId;
	TMap<FGuid, FAITCSTacticalOrder> LastOrdersByAgent;
	TArray<FAITCSTacticalOrder> PendingOrders;

	float LastDecisionProviderPassTime = 0.0f;
	float LastEvaluatorPassTime = 0.0f;
	int32 EvaluationFrameCounter = 0;
};

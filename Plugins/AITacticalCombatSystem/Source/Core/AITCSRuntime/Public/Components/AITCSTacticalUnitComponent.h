// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AITCSCoreData.h"
#include "TimerManager.h"

#include "AITCSTacticalUnitComponent.generated.h"

class UAITCSDirectorSubsystem;

DECLARE_MULTICAST_DELEGATE_OneParam(FAITCSOnTacticalOrderReceivedNative, const FAITCSTacticalOrder&);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAITCSOnTacticalOrderReceived, FAITCSTacticalOrder, Order);

/**
 * Component that registers a unit with the AITCS director and receives tactical orders.
 */
UCLASS(ClassGroup = (AI), meta = (BlueprintSpawnableComponent))
class AITCSRUNTIME_API UAITCSTacticalUnitComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	/** Construct a tactical unit component with default registration and retry settings. */
	UAITCSTacticalUnitComponent();

	/** Register this component with the director subsystem. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	bool RegisterWithDirector();

	/** Unregister this component from the director subsystem. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	void UnregisterFromDirector();

	/** Refresh the director registration state for this unit. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	bool RefreshDirectorRegistration();

	/** Return true when the unit has a resolved tactical assignment. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Registration")
	bool HasResolvedTacticalAssignment() const;

	/** Apply a tactical agent handle to this component. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	void ApplyTacticalAgentHandle(const FAITCSTacticalAgentHandle& InHandle);

	/** Receive a tactical order from the director or another system. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Orders")
	void ReceiveTacticalOrder(const FAITCSTacticalOrder& Order);

	/** Return whether this component is currently registered with the director. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Registration")
	bool IsRegisteredWithDirector() const;

	/** Get the tactical agent identifier assigned to this component. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Registration")
	FGuid GetTacticalAgentId() const;

	/** Get the tactical object identifier assigned to this component. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Registration")
	FGuid GetTacticalObjectId() const;

	/** Get the tactical group identifier assigned to this component. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Registration")
	FGameplayTag GetTacticalGroupId() const;

	/** Get the initial location captured when this component began play. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Registration")
	FVector GetInitialLocation() const;

	/** Get the current tactical order being processed by this component. */
	UFUNCTION(BlueprintPure, Category = "AITCS|Orders")
	FAITCSTacticalOrder GetCurrentTacticalOrder() const;

	/** Build the registration payload used by the director. */
	FAITCSTacticalAgentRegistration BuildRegistration() const;

	/** Native delegate for low-level tactical order notifications. */
	FAITCSOnTacticalOrderReceivedNative OnTacticalOrderReceivedNative;

	/** Blueprint event triggered when a tactical order is received. */
	UPROPERTY(BlueprintAssignable, Category = "AITCS|Orders")
	FAITCSOnTacticalOrderReceived OnTacticalOrderReceived;

	/** Automatically register with the director during gameplay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	bool bAutoRegisterWithDirector = true;

	/** Require a resolved tactical assignment before auto-registering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	bool bRequireResolvedAssignmentForAutoRegistration = true;

	/** Time interval between registration retry attempts. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float RegistrationRetryInterval = 0.2f;

	/** Maximum number of registration retry attempts before giving up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration", meta = (ClampMin = "0", UIMin = "0"))
	int32 MaxRegistrationRetryAttempts = 0;

	/** Desired tactical object identifier for this unit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	FGuid TacticalObjectId;

	/** Desired tactical group identifier for this unit. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	FGameplayTag TacticalGroupId;

	/** Optional debug name used for tactical registration tracing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	FName TacticalDebugName = NAME_None;

	/** Tags used to describe the tactical capabilities and state of this agent. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	FGameplayTagContainer AgentTags;

protected:
	/** Capture initial runtime state on begin play. */
	virtual void BeginPlay() override;

	/** Clean up director registration on end play. */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	UAITCSDirectorSubsystem* GetDirectorSubsystem() const;
	bool ShouldKeepRetryingRegistration() const;
	void ScheduleRegistrationRetry();
	void ClearRegistrationRetry();
	void HandleRegistrationRetry();

	/** The runtime agent identifier assigned to this unit. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AITCS|Registration", meta = (AllowPrivateAccess = "true"))
	FGuid TacticalAgentId;

	/** Whether this unit is currently registered with the director. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AITCS|Registration", meta = (AllowPrivateAccess = "true"))
	bool bRegisteredWithDirector = false;

	/** Most recent tactical order received by this unit. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AITCS|Orders", meta = (AllowPrivateAccess = "true"))
	FAITCSTacticalOrder CurrentTacticalOrder;

	/** Initial location captured for fallback or re-evaluation. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AITCS|Registration", meta = (AllowPrivateAccess = "true"))
	FVector InitialLocation = FVector::ZeroVector;

	FTimerHandle RegistrationRetryTimerHandle;
	int32 RegistrationRetryAttempts = 0;
};

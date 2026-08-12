// Pavel Gornostaev <https://github.com/Pavreally>

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

#include "AITCSDirectorActor.generated.h"

class UAITCSDirectorSubsystem;
class UAITCSTacticalGraphAsset;

/**
 * Actor helper that drives AITCS director activation and runtime registration.
 * This actor can be placed in levels to auto-activate graphs at BeginPlay.
 */
UCLASS(Blueprintable, ClassGroup = (AI), meta = (DisplayName = "AITCS Director"))
class AITCSRUNTIME_API AAITCSDirectorActor : public AActor
{
	GENERATED_BODY()

public:
	/** Construct the director actor and initialize default behavior. */
	AAITCSDirectorActor();

	/** Begin play hook used to activate the graph and optionally resolve units. */
	virtual void BeginPlay() override;

	/** Activate the configured tactical graph through the director subsystem. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Director")
	bool ActivateTacticalGraph();

	/** Resolve any units that were registered before graph activation. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Registration")
	int32 ResolveRegisteredUnits();

	/** Log a summary of runtime groups for debugging purposes. */
	UFUNCTION(BlueprintCallable, Category = "AITCS|Debug")
	void LogRuntimeGroupSummary() const;

	/** Tactical graph asset to activate when the actor begins play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Director")
	TObjectPtr<UAITCSTacticalGraphAsset> TacticalGraphAsset = nullptr;

	/** Automatically activate the tactical graph on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Director")
	bool bActivateOnBeginPlay = true;

	/** Resolve registered units after the graph is activated. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	bool bResolveRegisteredUnitsAfterActivation = true;

	/** Overwrite existing assignments when resolving registered units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	bool bOverwriteExistingUnitAssignments = false;

	/** Run deferred unit resolution shortly after BeginPlay if graph activation is delayed. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration")
	bool bRunDeferredResolveAfterBeginPlay = true;

	/** Delay before the deferred resolve pass runs after BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Registration", meta = (ClampMin = "0.0", UIMin = "0.0", EditCondition = "bRunDeferredResolveAfterBeginPlay"))
	float DeferredResolveDelay = 0.1f;

	/** Log a runtime group summary automatically when BeginPlay runs. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Debug")
	bool bLogRuntimeGroupSummaryOnBeginPlay = true;

	/** Log a runtime group summary after deferred unit resolution completes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AITCS|Debug")
	bool bLogRuntimeGroupSummaryAfterDeferredResolve = true;

private:
	void ScheduleDeferredResolvePass();
	void RunDeferredResolvePass();
	UAITCSDirectorSubsystem* GetDirectorSubsystem() const;

	FTimerHandle DeferredResolveTimerHandle;
};

// Pavel Gornostaev <https://github.com/Pavreally>

#include "Components/AITCSTacticalUnitComponent.h"

#include "Director/AITCSDirectorSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

UAITCSTacticalUnitComponent::UAITCSTacticalUnitComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UAITCSTacticalUnitComponent::BeginPlay()
{
	Super::BeginPlay();

	if (const AActor* Owner = GetOwner())
	{
		InitialLocation = Owner->GetActorLocation();
	}

	if (bAutoRegisterWithDirector)
	{
		if (!RegisterWithDirector() && ShouldKeepRetryingRegistration())
		{
			ScheduleRegistrationRetry();
		}
	}
}

void UAITCSTacticalUnitComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearRegistrationRetry();
	UnregisterFromDirector();
	Super::EndPlay(EndPlayReason);
}

bool UAITCSTacticalUnitComponent::RegisterWithDirector()
{
	UAITCSDirectorSubsystem* Director = GetDirectorSubsystem();
	if (!Director)
	{
		bRegisteredWithDirector = false;
		ScheduleRegistrationRetry();
		return false;
	}

	FAITCSTacticalAgentHandle RegisteredHandle;
	if (!Director->RegisterTacticalUnit(this, BuildRegistration(), RegisteredHandle))
	{
		bRegisteredWithDirector = false;
		ScheduleRegistrationRetry();
		return false;
	}

	ApplyTacticalAgentHandle(RegisteredHandle);
	if (HasResolvedTacticalAssignment() || !bRequireResolvedAssignmentForAutoRegistration)
	{
		ClearRegistrationRetry();
		return true;
	}

	ScheduleRegistrationRetry();
	return false;
}

void UAITCSTacticalUnitComponent::UnregisterFromDirector()
{
	ClearRegistrationRetry();

	if (!bRegisteredWithDirector || !TacticalAgentId.IsValid())
	{
		bRegisteredWithDirector = false;
		return;
	}

	if (UAITCSDirectorSubsystem* Director = GetDirectorSubsystem())
	{
		Director->UnregisterTacticalUnit(TacticalAgentId);
	}

	bRegisteredWithDirector = false;
}

bool UAITCSTacticalUnitComponent::RefreshDirectorRegistration()
{
	ClearRegistrationRetry();
	UnregisterFromDirector();
	return RegisterWithDirector();
}

bool UAITCSTacticalUnitComponent::HasResolvedTacticalAssignment() const
{
	return TacticalAgentId.IsValid() && TacticalObjectId.IsValid();
}

void UAITCSTacticalUnitComponent::ApplyTacticalAgentHandle(const FAITCSTacticalAgentHandle& InHandle)
{
	TacticalAgentId = InHandle.AgentId;
	TacticalObjectId = InHandle.ObjectId;
	TacticalGroupId = InHandle.GroupId;
	TacticalDebugName = InHandle.DebugName;
	bRegisteredWithDirector = TacticalAgentId.IsValid();

	if (HasResolvedTacticalAssignment() || (bRegisteredWithDirector && !bRequireResolvedAssignmentForAutoRegistration))
	{
		RegistrationRetryAttempts = 0;
		ClearRegistrationRetry();
	}
}

void UAITCSTacticalUnitComponent::ReceiveTacticalOrder(const FAITCSTacticalOrder& Order)
{
	CurrentTacticalOrder = Order;
	OnTacticalOrderReceivedNative.Broadcast(CurrentTacticalOrder);
	OnTacticalOrderReceived.Broadcast(CurrentTacticalOrder);
}

bool UAITCSTacticalUnitComponent::IsRegisteredWithDirector() const
{
	return bRegisteredWithDirector;
}

FGuid UAITCSTacticalUnitComponent::GetTacticalAgentId() const
{
	return TacticalAgentId;
}

FGuid UAITCSTacticalUnitComponent::GetTacticalObjectId() const
{
	return TacticalObjectId;
}

FGameplayTag UAITCSTacticalUnitComponent::GetTacticalGroupId() const
{
	return TacticalGroupId;
}

FVector UAITCSTacticalUnitComponent::GetInitialLocation() const
{
	return InitialLocation;
}

FAITCSTacticalOrder UAITCSTacticalUnitComponent::GetCurrentTacticalOrder() const
{
	return CurrentTacticalOrder;
}

FAITCSTacticalAgentRegistration UAITCSTacticalUnitComponent::BuildRegistration() const
{
	FAITCSTacticalAgentRegistration Registration;
	Registration.PreferredAgentId = TacticalAgentId;
	Registration.ObjectId = TacticalObjectId;
	Registration.GroupId = TacticalGroupId;
	Registration.DebugName = TacticalDebugName;
	Registration.AgentTags = AgentTags;
	Registration.AgentKind = EAITCSAgentKind::ActorComponent;

	const AActor* Owner = GetOwner();
	Registration.Location = Owner ? Owner->GetActorLocation() : FVector::ZeroVector;
	if (Registration.DebugName.IsNone() && Owner)
	{
		Registration.DebugName = Owner->GetFName();
	}

	return Registration;
}

UAITCSDirectorSubsystem* UAITCSTacticalUnitComponent::GetDirectorSubsystem() const
{
	const UWorld* World = GetWorld();
	return World ? World->GetSubsystem<UAITCSDirectorSubsystem>() : nullptr;
}

bool UAITCSTacticalUnitComponent::ShouldKeepRetryingRegistration() const
{
	if (!bAutoRegisterWithDirector)
	{
		return false;
	}

	if (MaxRegistrationRetryAttempts <= 0)
	{
		return true;
	}

	return RegistrationRetryAttempts < MaxRegistrationRetryAttempts;
}

void UAITCSTacticalUnitComponent::ScheduleRegistrationRetry()
{
	if (!ShouldKeepRetryingRegistration())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || World->GetTimerManager().IsTimerActive(RegistrationRetryTimerHandle))
	{
		return;
	}

	const float RetryInterval = FMath::Max(0.05f, RegistrationRetryInterval);
	World->GetTimerManager().SetTimer(RegistrationRetryTimerHandle, this, &UAITCSTacticalUnitComponent::HandleRegistrationRetry, RetryInterval, false);
}

void UAITCSTacticalUnitComponent::ClearRegistrationRetry()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RegistrationRetryTimerHandle);
	}
}

void UAITCSTacticalUnitComponent::HandleRegistrationRetry()
{
	RegistrationRetryAttempts++;

	if (bRegisteredWithDirector && HasResolvedTacticalAssignment())
	{
		ClearRegistrationRetry();
		return;
	}

	UnregisterFromDirector();
	RegisterWithDirector();
}

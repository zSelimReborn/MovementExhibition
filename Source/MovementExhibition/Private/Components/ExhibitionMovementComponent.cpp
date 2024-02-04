// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ExhibitionMovementComponent.h"

#include "CableComponent.h"
#include "Characters/ExhibitionCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#pragma region Saved Move

UExhibitionMovementComponent::FSavedMove_Exhibition::FSavedMove_Exhibition() { }

bool UExhibitionMovementComponent::FSavedMove_Exhibition::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Exhibition* NewMoveCasted = static_cast<FSavedMove_Exhibition*>(NewMove.Get());

	if (Saved_bWantsToSprint != NewMoveCasted->Saved_bWantsToSprint)
	{
		return false;
	}

	if (Saved_bWantsToDive != NewMoveCasted->Saved_bWantsToDive)
	{
		return false;
	}

	if (Saved_bWantsToHook != NewMoveCasted->Saved_bWantsToHook)
	{
		return false;
	}
	
	return FSavedMove_Character::CanCombineWith(NewMove, InCharacter, MaxDelta);
}

void UExhibitionMovementComponent::FSavedMove_Exhibition::Clear()
{
	FSavedMove_Character::Clear();

	Saved_bWantsToSprint = 0;
	Saved_bPrevWantsToCrouch = 0;
	Saved_bWantsToDive = 0;
	Saved_FlyingDiveCount = 0;
	Saved_bWantsToHook = 0;
}

uint8 UExhibitionMovementComponent::FSavedMove_Exhibition::GetCompressedFlags() const
{
	uint8 CompressedFlags = FSavedMove_Character::GetCompressedFlags();
	if (Saved_bWantsToSprint)
	{
		CompressedFlags |= FLAG_Custom_0;
	}

	if (Saved_bWantsToDive)
	{
		CompressedFlags |= FLAG_Custom_1;
	}

	if (Saved_bWantsToHook)
	{
		CompressedFlags |= FLAG_Custom_2;
	}

	return CompressedFlags;
}

void UExhibitionMovementComponent::FSavedMove_Exhibition::SetMoveFor(ACharacter* C, float InDeltaTime,
	FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData)
{
	FSavedMove_Character::SetMoveFor(C, InDeltaTime, NewAccel, ClientData);

	if (C == nullptr)
	{
		return;
	}
	
	const UExhibitionMovementComponent* MovComponent = Cast<UExhibitionMovementComponent>(C->GetMovementComponent());
	if (MovComponent == nullptr)
	{
		return;
	}

	Saved_bWantsToSprint = MovComponent->Safe_bWantsToSprint;
	Saved_bPrevWantsToCrouch = MovComponent->Safe_bPrevWantsToCrouch;
	Saved_bWantsToDive = MovComponent->Safe_bWantsToDive;
	Saved_FlyingDiveCount = MovComponent->Safe_FlyingDiveCount;
	Saved_bWantsToHook = MovComponent->Safe_bWantsToHook;
}

void UExhibitionMovementComponent::FSavedMove_Exhibition::PrepMoveFor(ACharacter* C)
{
	FSavedMove_Character::PrepMoveFor(C);

	if (C == nullptr)
	{
		return;
	}

	UExhibitionMovementComponent* MovComponent = Cast<UExhibitionMovementComponent>(C->GetMovementComponent());
	if (MovComponent == nullptr)
	{
		return;
	}

	MovComponent->Safe_bWantsToSprint = Saved_bWantsToSprint;
	MovComponent->Safe_bPrevWantsToCrouch = Saved_bPrevWantsToCrouch;
	MovComponent->Safe_bWantsToDive = Saved_bWantsToDive;
	MovComponent->Safe_FlyingDiveCount = Saved_FlyingDiveCount;
	MovComponent->Safe_bWantsToHook = Saved_bWantsToHook;
}

#pragma endregion 

#pragma region Network Prediction Data

UExhibitionMovementComponent::FNetworkPredictionData_Client_Exhibition::FNetworkPredictionData_Client_Exhibition(const UCharacterMovementComponent& ClientMovement)
	: Super(ClientMovement)
{
}

FSavedMovePtr UExhibitionMovementComponent::FNetworkPredictionData_Client_Exhibition::AllocateNewMove()
{
	return MakeShared<FSavedMove_Exhibition>();
}

#pragma endregion

UExhibitionMovementComponent::UExhibitionMovementComponent()
{
	NavAgentProps.bCanCrouch = true;

	MaxWalkSpeed = 500.f;
	MaxWalkSpeedCrouched = 270.f;
	
	MaxAcceleration = 1500.f;
	BrakingFrictionFactor = 1.f;
	// This is used to slowly decrease the max speed rather than an instant drop.
	// e.g: from sprinting to crouching 
	bUseSeparateBrakingFriction = true;

	bCanWalkOffLedgesWhenCrouching = true;
}

void UExhibitionMovementComponent::InitializeComponent()
{
	Super::InitializeComponent();

	if (CharacterOwner == nullptr || CharacterOwner->GetCapsuleComponent() == nullptr)
	{
		return;
	}
	
	InitialCapsuleHalfHeight = CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
	ExhibitionCharacterRef = Cast<AExhibitionCharacter>(CharacterOwner);
}

FNetworkPredictionData_Client* UExhibitionMovementComponent::GetPredictionData_Client() const
{
	if (ClientPredictionData == nullptr)
	{
		UExhibitionMovementComponent* MutableThis = const_cast<UExhibitionMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_Exhibition(*this);
	}
	return ClientPredictionData;
}

float UExhibitionMovementComponent::GetMaxSpeed() const
{
	if (IsSprinting())
	{
		return IsCrouching()? MaxSprintCrouchedSpeed : MaxSprintSpeed;
	}

	if (!IsMovementMode(MOVE_Custom))
	{
		return Super::GetMaxSpeed();
	}

	switch (CustomMovementMode)
	{
	case CMOVE_Hook:
		return MaxHookSpeed;
	case CMOVE_Slide:
	default:
		return MaxCustomMovementSpeed;
	}
}

float UExhibitionMovementComponent::GetMaxBrakingDeceleration() const
{
	if (!IsMovementMode(MOVE_Custom))
	{
		return Super::GetMaxBrakingDeceleration();
	}

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		return SlideBrakingDeceleration;
	case CMOVE_Hook:
		return 0.f;
	default:
		return -1.f;
	}
}

bool UExhibitionMovementComponent::IsMovingOnGround() const
{
	return Super::IsMovingOnGround() || IsCustomMovementMode(CMOVE_Slide);
}

bool UExhibitionMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() || IsSliding();
}

void UExhibitionMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	Super::PhysCustom(deltaTime, Iterations);

	switch (CustomMovementMode)
	{
	case CMOVE_Slide:
		PhysSlide(deltaTime, Iterations);
		break;
	case CMOVE_Hook:
		PhysHook(deltaTime, Iterations);
		break;
	default:
		UE_LOG(LogTemp, Fatal, TEXT("PhysCustom Invalid Custom Movement Mode: %d"), CustomMovementMode);
	}
}

void UExhibitionMovementComponent::OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode)
{
	Super::OnMovementModeChanged(PreviousMovementMode, PreviousCustomMode);

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Slide)
	{
		FinishSlide();
	}

	if (IsCustomMovementMode(CMOVE_Slide))
	{
		EnterSlide();
	}

	if ((PreviousMovementMode == MOVE_Falling) && IsMovingOnGround())
	{
		Safe_FlyingDiveCount = 0;
	}

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Hook)
	{
		FinishHook();
	}

	if (IsCustomMovementMode(CMOVE_Hook))
	{
		EnterHook();
	}
}

void UExhibitionMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	const bool bAuthProxy = IsAuthProxy();

	// Update sprint status
	if (Safe_bWantsToSprint && Velocity.IsNearlyZero())
	{
		Safe_bWantsToSprint = false;
	}
	
	// Slide
	if (bWantsToCrouch && CanSlide() && !IsDiving())
	{
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
	}
	else if (IsSliding() && !bWantsToCrouch)
	{
		SetMovementMode(MOVE_Walking);
	}

	// Roll
	if (Safe_bWantsToDive && CanDive())
	{
		PerformDive();
		Proxy_Dive = !Proxy_Dive;
	}

	// Try hooking
	if (Safe_bWantsToHook && TryHook())
	{
		Proxy_FindHook = !Proxy_FindHook;
		SetMovementMode(MOVE_Custom, CMOVE_Hook);
	}
	else if (Safe_bWantsToHook && IsHooking())
	{
		SetMovementMode(MOVE_Falling);
	}

	// Check if a montage ended
	if (CharacterOwner->GetCurrentMontage() == nullptr)
	{
		OnFinishMontage(CurrentAnimMontage);
	}
	
	Safe_bWantsToDive = false;
	Safe_bWantsToHook = false;
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

bool UExhibitionMovementComponent::DoJump(bool bReplayingMoves)
{
	const bool bJumped = Super::DoJump(bReplayingMoves);
	if (bJumped)
	{
		if (CharacterOwner->JumpCurrentCount > 1)
		{
			PlayMontage(JumpExtraMontage);
			Proxy_JumpExtra = !Proxy_JumpExtra;
		}
	}

	return bJumped;
}

void UExhibitionMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UExhibitionMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToSprint = (Flags & FSavedMove_Character::CompressedFlags::FLAG_Custom_0) != 0;
	Safe_bWantsToDive = (Flags & FSavedMove_Character::CompressedFlags::FLAG_Custom_1) != 0;
	Safe_bWantsToHook = (Flags & FSavedMove_Character::CompressedFlags::FLAG_Custom_2) != 0;
}

bool UExhibitionMovementComponent::IsCustomMovementMode(const ECustomMovementMode& InMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InMovementMode;
}

void UExhibitionMovementComponent::PlayMontage(UAnimMontage* InMontage)
{
	ensure(CharacterOwner != nullptr);

	if (InMontage == nullptr)
	{
		return;
	}
	
	if (CurrentAnimMontage == InMontage)
	{
		return;
	}
	
	if (CurrentAnimMontage != nullptr)
	{
		OnFinishMontage(CurrentAnimMontage);
	}
	
	CharacterOwner->PlayAnimMontage(InMontage);
	CurrentAnimMontage = InMontage;
}

void UExhibitionMovementComponent::OnFinishMontage(const UAnimMontage* Montage)
{
	if (Montage == DiveMontage)
	{
		bWantsToCrouch = false;
	}
	else if (Montage == DodgeBackMontage)
	{
		bOrientRotationToMovement = true;
	}
	else if (Montage == FlyingDiveMontage)
	{
		SetMovementMode(MOVE_Falling);
	}

	CurrentAnimMontage = nullptr;
}

bool UExhibitionMovementComponent::IsAuthProxy() const
{
	ensure(CharacterOwner != nullptr);
	return CharacterOwner->HasAuthority() && !CharacterOwner->IsLocallyControlled();
}

float UExhibitionMovementComponent::GetCapsuleRadius() const
{
	ensure(CharacterOwner != nullptr && CharacterOwner->GetCapsuleComponent() != nullptr);
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleRadius();
}

float UExhibitionMovementComponent::GetCapsuleHalfHeight() const
{
	ensure(CharacterOwner != nullptr && CharacterOwner->GetCapsuleComponent() != nullptr);
	return CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
}

void UExhibitionMovementComponent::ToggleHookCable() const
{
	ensure(CharacterOwner != nullptr);

	UCableComponent* HookCable = CharacterOwner->FindComponentByClass<UCableComponent>();
	if (HookCable)
	{
		HookCable->bAttachEnd = true;
		HookCable->bAttachStart = true;
		HookCable->SetHiddenInGame(!HookCable->bHiddenInGame);
	}
}

void UExhibitionMovementComponent::UpdateHookCable() const
{
	ensure(CharacterOwner != nullptr);

	UCableComponent* HookCable = CharacterOwner->FindComponentByClass<UCableComponent>();
	if (HookCable && CurrentHook != nullptr)
	{
		const float Length = FVector::Dist(HookCable->GetComponentLocation(), CurrentHook->GetActorLocation());
		//HookCable->SetUsingAbsoluteLocation(true);
		//HookCable->SetWorldLocation(CurrentHook->GetActorLocation());
		HookCable->SetAttachEndTo(CurrentHook, NAME_None);
		//HookCable->EndLocation = CurrentHook->GetActorLocation();
		HookCable->CableLength = Length;
	}
}

void UExhibitionMovementComponent::ResetHookCable() const
{
	ensure(CharacterOwner != nullptr);

	UCableComponent* HookCable = CharacterOwner->FindComponentByClass<UCableComponent>();
	if (HookCable)
	{
		ToggleHookCable();
		//HookCable->SetWorldLocation(CharacterOwner->GetActorLocation());
		HookCable->SetAttachEndTo(nullptr, NAME_None);
		//HookCable->EndLocation = UpdatedComponent->GetComponentLocation();
		HookCable->CableLength = 0.f;
	}
}

#pragma region Slide

void UExhibitionMovementComponent::EnterSlide()
{
	bWantsToCrouch = true;
	bOrientRotationToMovement = false;

	Velocity += Velocity.GetSafeNormal2D() * SlideEnterImpulse;

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, nullptr);

	OnEnterSlide.Broadcast();
}

void UExhibitionMovementComponent::FinishSlide()
{
	bWantsToCrouch = false;
	bOrientRotationToMovement = true;

	OnExitSlide.Broadcast();
}

bool UExhibitionMovementComponent::CanSlide() const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.5f * FVector::DownVector;
	const FName ProfileName = TEXT("BlockAll");
	const bool bValidSurface = GetWorld()->LineTraceTestByProfile(Start, End, ProfileName, ExhibitionCharacterRef->GetIgnoreCollisionParams());
	const bool bEnoughSpeed = Velocity.SizeSquared2D() > FMath::Pow(SlideMinSpeed, 2);
	
	return bValidSurface && bEnoughSpeed && IsMovingOnGround();
}

void UExhibitionMovementComponent::PhysSlide(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (!CanSlide())
	{
		SetMovementMode(MOVE_Walking);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	bJustTeleported = false;
	float remainingTime = deltaTime;

	// Sub-stepping
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();

		// Ensure velocity is horizontal.
		MaintainHorizontalGroundVelocity();
		const FVector OldVelocity = Velocity;

		FVector SlopeForce = CurrentFloor.HitResult.Normal;
		SlopeForce.Z = 0.f;
		Velocity += SlopeForce * SlideGravityForce * timeTick;
		
		Acceleration = Acceleration.ProjectOnTo(UpdatedComponent->GetRightVector().GetSafeNormal2D());

		// Apply acceleration
		CalcVelocity(timeTick, GroundFriction * SlideFrictionFactor, false, GetMaxBrakingDeceleration());
		
		// Compute move parameters
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		FStepDownResult StepDownResult;
		bool bFloorWalkable = CurrentFloor.IsWalkableFloor();
		
		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			// try to move forward
			MoveAlongFloor(MoveVelocity, timeTick, &StepDownResult);

			if (IsFalling())
			{
				const float DesiredDist = Delta.Size();
				if (DesiredDist > KINDA_SMALL_NUMBER)
				{
					const float ActualDist = (UpdatedComponent->GetComponentLocation() - OldLocation).Size2D();
					remainingTime += timeTick * (1.f - FMath::Min(1.f,ActualDist/DesiredDist));
				}
				
				StartNewPhysics(remainingTime,Iterations);
				return;
			}
			
			if (IsSwimming())
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		if (StepDownResult.bComputedFloor)
		{
			CurrentFloor = StepDownResult.FloorResult;
		}
		else
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, bZeroDelta, nullptr);
		}
		
		if (IsMovingOnGround() && bFloorWalkable)
		{
			// Make velocity reflect actual move
			if( !bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
				MaintainHorizontalGroundVelocity();
				AdjustFloorHeight();
			}
		}

		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}
}

#pragma endregion

#pragma region Roll

void UExhibitionMovementComponent::PerformDive()
{
	ensure(CharacterOwner != nullptr);
	
	FVector RollDirection = (Acceleration.IsNearlyZero()? CharacterOwner->GetActorForwardVector() : Acceleration).GetSafeNormal2D();
	UAnimMontage* NextMontage;
	float ApplyingImpulse;
	
	if (Velocity.SizeSquared2D() > FMath::Pow(DiveMinSpeed, 2) && !IsFalling())
	{
		NextMontage = DiveMontage;
		bWantsToCrouch = true;

		ApplyingImpulse = DiveImpulse;

		OnDive.Broadcast();
	}
	else if (IsFalling())
	{
		NextMontage = FlyingDiveMontage;
		ApplyingImpulse = FlyingDiveImpulse;
		Safe_FlyingDiveCount++;

		SetMovementMode(MOVE_Flying);
	}
	else
	{
		RollDirection = -RollDirection;
		NextMontage = DodgeBackMontage;
		bOrientRotationToMovement = false;

		ApplyingImpulse = DodgeBackImpulse;
	}

	PlayMontage(NextMontage);
	const float OldZVelocity = Velocity.Z;
	Velocity = RollDirection * ApplyingImpulse;
	Velocity.Z = OldZVelocity;
}

bool UExhibitionMovementComponent::CanDive() const
{
	return (IsWalking() && !IsCrouching()) || (IsFalling() && Safe_FlyingDiveCount == 0);
}

#pragma endregion

#pragma region Hooking

bool UExhibitionMovementComponent::TryHook()
{
	if (!IsFalling() && !IsWalking())
	{
		return false;
	}
	
	TArray<AActor*> AllHooks;
	// Can be improved
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), TagHookName, AllHooks);

	if (AllHooks.IsEmpty())
	{
		return false;
	}

	AActor* SelectedHook = nullptr;
	float SelectedHookDistanceSrd = FMath::Pow(MinHookDistance, 2);
	for (AActor* Hook : AllHooks)
	{
		const float HookDistance = FVector::DistSquared(UpdatedComponent->GetComponentLocation(), Hook->GetActorLocation());
		if (HookDistance <= FMath::Pow(IgnoreHookDistance, 2))
		{
			continue;
		}
		
		if (CanUseHook(Hook) && HookDistance <= SelectedHookDistanceSrd)
		{
			SelectedHook = Hook;
			SelectedHookDistanceSrd = HookDistance;
		}
	}

	CurrentHook = SelectedHook;
	return CurrentHook != nullptr;
}

bool UExhibitionMovementComponent::CanUseHook(const AActor* Hook) const
{
	if (!Hook)
	{
		return false;
	}

	const FVector CharacterLocation = UpdatedComponent->GetComponentLocation();
	const FRotator CharacterViewRotation = UpdatedComponent->GetComponentRotation();
	
	const FVector ControlLook = CharacterViewRotation.Vector().GetSafeNormal2D();
	const FVector HookLocation = Hook->GetActorLocation();
	const FVector ControlLookToHook = (HookLocation - CharacterLocation).GetSafeNormal2D();

	DrawDebugLine(
		GetWorld(),
		CharacterLocation,
		CharacterLocation + ControlLook * 500.f,
		FColor::Red,
		false,
		5.f
	);
	
	DrawDebugLine(
		GetWorld(),
		CharacterLocation,
		CharacterLocation + ControlLookToHook * 500.f,
		FColor::Green,
		false,
		5.f
	);

	const bool bNear = FVector::DistSquared(CharacterLocation, HookLocation) <= FMath::Pow(MinHookDistance, 2);

	if (!bNear)
	{
		return false;
	}

	const bool bInFov = (ControlLook | ControlLookToHook) >= 0.8f;
	if (!bInFov)
	{
		return false;
	}

	FCollisionShape Capsule = FCollisionShape::MakeCapsule(GetCapsuleRadius(), GetCapsuleHalfHeight());
	FHitResult Hit;
	FCollisionQueryParams IgnoreParams = ExhibitionCharacterRef->GetIgnoreCollisionParams();
	IgnoreParams.AddIgnoredActor(Hook);
	const bool bIsBlocked = GetWorld()->SweepSingleByProfile(
		Hit,
		CharacterLocation,
		HookLocation,
		FRotator::ZeroRotator.Quaternion(),
		FName(TEXT("BlockAll")),
		Capsule,
		IgnoreParams
	);

	if (bIsBlocked)
	{
		DrawDebugCapsule(
			GetWorld(),
			Hit.Location,
			Capsule.GetCapsuleHalfHeight(),
			Capsule.GetCapsuleRadius(),
			FRotator::ZeroRotator.Quaternion(),
			FColor::Red,
			false,
			5.f
		);
	}
	
	return !bIsBlocked;
}

void UExhibitionMovementComponent::EnterHook()
{
	bOrientRotationToMovement = false;
	PlayMontage(GrabHookMontage);

	if (bHandleCable)
	{
		ToggleHookCable();
		UpdateHookCable();
	}
}

void UExhibitionMovementComponent::FinishHook()
{
	bOrientRotationToMovement = true;
	CurrentHook = nullptr;
	

	// Force stop
	Velocity -= Velocity / 1.4f;
	
	if (bHandleCable)
	{
		ResetHookCable();
	}
}

void UExhibitionMovementComponent::PhysHook(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}

	if (CurrentHook == nullptr)
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}
	
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (UpdatedComponent->GetComponentLocation().Equals(CurrentHook->GetActorLocation(), ReleaseHookTolerance))
	{
		SetMovementMode(MOVE_Falling);
		StartNewPhysics(deltaTime, Iterations);
		return;
	}

	bJustTeleported = false;
	float remainingTime = deltaTime;

	// Sub-stepping
	while ((remainingTime >= MIN_TICK_TIME) && (Iterations < MaxSimulationIterations) && CharacterOwner && (CharacterOwner->Controller || bRunPhysicsWithNoController || HasAnimRootMotion() || CurrentRootMotion.HasOverrideVelocity() || (CharacterOwner->GetLocalRole() == ROLE_SimulatedProxy)))
	{
		Iterations++;
		bJustTeleported = false;
		const float timeTick = GetSimulationTimeStep(remainingTime, Iterations);
		remainingTime -= timeTick;

		// Save current values
		const FVector OldLocation = UpdatedComponent->GetComponentLocation();
		const FVector OldVelocity = Velocity;

		const FVector Direction = (CurrentHook->GetActorLocation() - UpdatedComponent->GetComponentLocation()).GetSafeNormal();
		const FRotator NewRotation = Direction.Rotation();
		const FQuat NewCharacterRotation = FRotator{0.f, NewRotation.Yaw, 0.f}.Quaternion();

		const float Impulse = (HookPullImpulse <= MaxHookSpeed)? HookPullImpulse : MaxHookSpeed;
		Velocity = Direction * Impulse;

		UE_LOG(LogTemp, Error, TEXT("Velocity: %.2f"), Velocity.Size());
		
		// Move params
		const FVector MoveVelocity = Velocity;
		const FVector Delta = timeTick * MoveVelocity;
		const bool bZeroDelta = Delta.IsNearlyZero();
		
		if (bZeroDelta)
		{
			remainingTime = 0.f;
		}
		else
		{
			FHitResult MoveHit;
			SafeMoveUpdatedComponent(Delta, NewCharacterRotation, true, MoveHit);
			if (bHandleCable)
			{
				UpdateHookCable();
			}
			
			// Make velocity reflect actual move
			if( !bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && timeTick >= MIN_TICK_TIME)
			{
				Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / timeTick;
			}
		}

		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}

		if (UpdatedComponent->GetComponentLocation().Equals(CurrentHook->GetActorLocation(), ReleaseHookTolerance))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(remainingTime, Iterations);
			break;
		}

		// Fallback check to leave hooking
		if (CurrentHook->IsOverlappingActor(CharacterOwner))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(remainingTime, Iterations);
			break;
		}
	}
}

#pragma endregion

void UExhibitionMovementComponent::ToggleCrouch()
{
	bWantsToCrouch = !bWantsToCrouch;
}

void UExhibitionMovementComponent::ToggleSprint()
{
	Safe_bWantsToSprint = !Safe_bWantsToSprint;
}

bool UExhibitionMovementComponent::CanSprint() const
{
	return IsMovementMode(MOVE_Walking) && Velocity.Size2D() > 0.f;
}

bool UExhibitionMovementComponent::IsSprinting() const
{
	return Safe_bWantsToSprint && CanSprint();
}

bool UExhibitionMovementComponent::IsSliding() const
{
	return IsCustomMovementMode(CMOVE_Slide);
}

void UExhibitionMovementComponent::RequestDive()
{
	Safe_bWantsToDive = true;
}

bool UExhibitionMovementComponent::IsDiving() const
{
	ensure(CharacterOwner != nullptr);
	return CharacterOwner->GetCurrentMontage() == DiveMontage;
}

void UExhibitionMovementComponent::ToggleHook()
{
	Safe_bWantsToHook = !Safe_bWantsToHook;
}

bool UExhibitionMovementComponent::IsHooking() const
{
	return IsCustomMovementMode(CMOVE_Hook);
}

void UExhibitionMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UExhibitionMovementComponent, Proxy_Dive, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UExhibitionMovementComponent, Proxy_JumpExtra, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UExhibitionMovementComponent, Proxy_FindHook, COND_SkipOwner);
}

void UExhibitionMovementComponent::OnRep_Dive()
{
	if (bWantsToCrouch)
	{
		// If player wants to crouch he requested a dive
		CharacterOwner->PlayAnimMontage(DiveMontage);
		OnDive.Broadcast();
	}
	else if (IsFalling())
	{
		CharacterOwner->PlayAnimMontage(FlyingDiveMontage);
	}
	else
	{
		// else he requested a dodge
		CharacterOwner->PlayAnimMontage(DodgeBackMontage);
	}
}

void UExhibitionMovementComponent::OnRep_JumpExtra()
{
	CharacterOwner->PlayAnimMontage(JumpExtraMontage);
}

void UExhibitionMovementComponent::OnRep_FindHook()
{
	TryHook();
}

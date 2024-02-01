// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ExhibitionMovementComponent.h"

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
	
	return Super::GetMaxSpeed();
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
		break;
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
	if (Safe_bWantsToHook)
	{
		if (TryHook())
		{
			SetMovementMode(MOVE_Custom, CMOVE_Hook);
			HookTotalTime = 0.f;
			HookCurrentDistance = FVector::Dist(CharacterOwner->GetActorLocation(), CurrentHook->GetActorLocation());
		}
	}
	else if (IsHooking())
	{
		// TODO Add force if jumped
		SetMovementMode(MOVE_Falling);
		CurrentHook = nullptr;
		HookTotalTime = 0.f;
		HookCurrentDistance = 0.f;
	}

	// Check if a montage ended
	if (CharacterOwner->GetCurrentMontage() == nullptr)
	{
		OnFinishMontage(CurrentAnimMontage);
	}
	
	Safe_bWantsToDive = false;
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

float UExhibitionMovementComponent::GetAngleInDegrees(const FVector& First, const FVector& Second) const
{
	const FVector FirstNormalized = First.GetSafeNormal();
	const FVector SecondNormalized = Second.GetSafeNormal();

	const float Dot = FirstNormalized | SecondNormalized;
	return FMath::RadiansToDegrees(FMath::Acos(Dot));
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

	for (AActor* Hook : AllHooks)
	{
		if (CanUseHook(Hook))
		{
			CurrentHook = Hook;
			break;
		}
	}

	// TODO transition to fixed height
	
	return CurrentHook != nullptr;
}

bool UExhibitionMovementComponent::CanUseHook(const AActor* Hook) const
{
	if (!Hook)
	{
		return false;
	}

	const FVector CharacterLocation = CharacterOwner->GetActorLocation();
	const FRotator ControlRotation = CharacterOwner->GetControlRotation();
	const FVector ControlLook = ControlRotation.Vector().GetSafeNormal();
	const FVector HookLocation = Hook->GetActorLocation();
	const FVector ControlLookToHook = (HookLocation - CharacterLocation).GetSafeNormal();

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

	const float DistToHook = FVector::Dist(CharacterLocation, HookLocation);
	const bool bNear = FVector::DistSquared(CharacterLocation, HookLocation) <= FMath::Pow(MinHookDistance, 2);

	UE_LOG(LogTemp, Error, TEXT("Dist to hook: %.2f"), DistToHook);
	if (!bNear)
	{
		return false;
	}

	const bool bInFov = (ControlLook | ControlLookToHook) >= 0.8f;
	UE_LOG(LogTemp, Error, TEXT("Dot to hook: %.2f"), ControlLook | ControlLookToHook);
	if (!bInFov)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams IgnoreParams = ExhibitionCharacterRef->GetIgnoreCollisionParams();
	IgnoreParams.AddIgnoredActor(Hook);
	const bool bIsBlocked = GetWorld()->LineTraceSingleByProfile(
		Hit,
		CharacterLocation,
		HookLocation,
		FName(TEXT("BlockAll")),
		IgnoreParams
	);

	// TODO Add busy check?
	
	return !bIsBlocked;
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

	bJustTeleported = false;
	float remainingTime = deltaTime;

	const float DistanceToHook = HookCurrentDistance - 50.f;
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

		const FVector HookToCharacter = UpdatedComponent->GetComponentLocation() - CurrentHook->GetActorLocation();
		float Angle = GetAngleInDegrees(HookToCharacter, -CurrentHook->GetActorUpVector());
		const float Cross = (HookToCharacter ^ -CurrentHook->GetActorUpVector()).Z;
		
		Angle = FMath::Clamp(Angle, -MaxHookAngle, MaxHookAngle);
		Angle = (Cross > 0)? Angle : -Angle;
		UE_LOG(LogTemp, Error, TEXT("Angle Deg: %.2f | CrossZ: %.2f"), Angle, Cross);
		
		HookTotalTime += timeTick;
		const FVector Delta1 = FVector(
			DistanceToHook * sin(MaxHookAngle * sin(HookTotalTime) / 30),
			0.0f,
			-cos(MaxHookAngle * sin(HookTotalTime) / 30) * DistanceToHook
		);
		const FVector NextPosition = CurrentHook->GetActorLocation() + Delta1;
		const FVector DeltaVelocity = NextPosition - UpdatedComponent->GetComponentLocation();

		Velocity = DeltaVelocity * 10.f * timeTick;
		// Apply acceleration
		CalcVelocity(timeTick, 0.f, false, 0.f);

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
			SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, MoveHit);
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

void UExhibitionMovementComponent::RequestHook()
{
	Safe_bWantsToHook = true;
}

void UExhibitionMovementComponent::ReleaseHook()
{
	Safe_bWantsToHook = false;
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

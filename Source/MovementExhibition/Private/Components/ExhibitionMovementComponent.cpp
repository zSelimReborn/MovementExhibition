// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ExhibitionMovementComponent.h"

#include "Characters/ExhibitionCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

#pragma region Saved Move

UExhibitionMovementComponent::FSavedMove_Exhibition::FSavedMove_Exhibition() { }

bool UExhibitionMovementComponent::FSavedMove_Exhibition::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const
{
	const FSavedMove_Exhibition* NewMoveCasted = static_cast<FSavedMove_Exhibition*>(NewMove.Get());

	if (Saved_bWantsToSprint != NewMoveCasted->Saved_bWantsToSprint)
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
}

uint8 UExhibitionMovementComponent::FSavedMove_Exhibition::GetCompressedFlags() const
{
	uint8 CompressedFlags = FSavedMove_Character::GetCompressedFlags();
	if (Saved_bWantsToSprint)
	{
		CompressedFlags |= FLAG_Custom_0;
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
}

void UExhibitionMovementComponent::UpdateCharacterStateBeforeMovement(float DeltaSeconds)
{
	// Slide
	/* if (MovementMode == MOVE_Walking && !bWantsToCrouch && Safe_bPrevWantsToCrouch)
	{
		if (CanSlide())
		{
			SetMovementMode(MOVE_Custom, CMOVE_Slide);
		}
	}
	else if (IsCustomMovementMode(CMOVE_Slide) && !bWantsToCrouch)
	{
		SetMovementMode(MOVE_Walking);
	} */

	if (bWantsToCrouch && CanSlide())
	{
		SetMovementMode(MOVE_Custom, CMOVE_Slide);
	}
	else if (IsSliding() && !bWantsToCrouch)
	{
		SetMovementMode(MOVE_Walking);
	}
	
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UExhibitionMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);
}

void UExhibitionMovementComponent::UpdateFromCompressedFlags(uint8 Flags)
{
	Super::UpdateFromCompressedFlags(Flags);

	Safe_bWantsToSprint = (Flags & FSavedMove_Character::CompressedFlags::FLAG_Custom_0) != 0;
}

bool UExhibitionMovementComponent::IsCustomMovementMode(const ECustomMovementMode& InMovementMode) const
{
	return MovementMode == MOVE_Custom && CustomMovementMode == InMovementMode;
}

#pragma region Slide

void UExhibitionMovementComponent::EnterSlide()
{
	bWantsToCrouch = true;
	bOrientRotationToMovement = false;

	Velocity += Velocity.GetSafeNormal2D() * SlideEnterImpulse;

	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, true, nullptr);
}

void UExhibitionMovementComponent::FinishSlide()
{
	bWantsToCrouch = false;
	bOrientRotationToMovement = true;
}

bool UExhibitionMovementComponent::CanSlide() const
{
	const FVector Start = UpdatedComponent->GetComponentLocation();
	const FVector End = Start + CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 2.5f * FVector::DownVector;
	const FName ProfileName = TEXT("BlockAll");
	const bool bValidSurface = GetWorld()->LineTraceTestByProfile(Start, End, ProfileName, ExhibitionCharacterRef->GetIgnoreCollisionParams());
	const bool bEnoughSpeed = Velocity.SizeSquared2D() > FMath::Pow(SlideMinSpeed, 2);
	
	return bValidSurface && bEnoughSpeed;
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
			
			if (IsSwimming()) //just entered water
			{
				StartSwimming(OldLocation, OldVelocity, timeTick, remainingTime, Iterations);
				return;
			}
		}

		// Update floor.
		// StepUp might have already done it for us.
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

		// If we didn't move at all this iteration then abort (since future iterations will also be stuck).
		if (UpdatedComponent->GetComponentLocation() == OldLocation)
		{
			remainingTime = 0.f;
			break;
		}
	}

	/* FHitResult Hit;
	FQuat NewRotation = FRotationMatrix::MakeFromXZ(Velocity.GetSafeNormal2D(), FVector::UpVector).ToQuat();
	SafeMoveUpdatedComponent(FVector::ZeroVector, NewRotation, false, Hit); */
}

#pragma endregion

void UExhibitionMovementComponent::ToggleCrouch()
{
	bWantsToCrouch = !bWantsToCrouch;
}

void UExhibitionMovementComponent::RequestSprint()
{
	Safe_bWantsToSprint = true;
}

void UExhibitionMovementComponent::FinishSprint()
{
	Safe_bWantsToSprint = false;
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

// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/ExhibitionMovementComponent.h"

#include "CableComponent.h"
#include "Characters/ExhibitionCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

#define SHAPES_DEBUG_DURATION 5.f
#define LINE(Start, End, Color) DrawDebugLine(GetWorld(), Start, End, Color, false, SHAPES_DEBUG_DURATION)
#define CAPSULE(Center, Hh, Radius, Color) DrawDebugCapsule(GetWorld(), Center, Hh, Radius, FRotator::ZeroRotator.Quaternion(), Color, false, SHAPES_DEBUG_DURATION)
#define POINT(Location, Size, Color) DrawDebugPoint(GetWorld(), Location, Size, Color, false, SHAPES_DEBUG_DURATION);
#define SCREEN_LOG(Text, Color) GEngine->AddOnScreenDebugMessage(INDEX_NONE, SHAPES_DEBUG_DURATION, Color, Text)

const FString UExhibitionMovementComponent::HOOK_TRAVEL_NAME = TEXT("HookTravel");

const FString UExhibitionMovementComponent::ROPE_TRAVEL_NAME = TEXT("RopeTravel");
const FString UExhibitionMovementComponent::ROPE_TRANSITION_NAME = TEXT("RopeTransition");

static TAutoConsoleVariable<bool> CVarDebugMovement(
	TEXT("MovExhibition.Debug.CMC"),
	false,
	TEXT("Debug info about custom character movement component"),
	ECVF_Default
);

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
	Saved_bReachedDestination = 0;
	Saved_bCustomPressedJump = 0;
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

	if (Saved_bCustomPressedJump)
	{
		CompressedFlags |= FLAG_JumpPressed; 
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
	Saved_bReachedDestination = MovComponent->Safe_bReachedDestination;
	Saved_bCustomPressedJump = MovComponent->ExhibitionCharacterRef->bCustomPressedJump;
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
	MovComponent->Safe_bReachedDestination = Saved_bReachedDestination;
	MovComponent->ExhibitionCharacterRef->bCustomPressedJump = Saved_bCustomPressedJump;
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
	if (!TravelData.IsSet())
	{
		TravelData.Emplace();
	}
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
		PhysTravel(deltaTime, Iterations);
		UpdateHookCable(deltaTime);
		break;
	case CMOVE_Rope:
		PhysTravel(deltaTime, Iterations);
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

	if (PreviousMovementMode == MOVE_Custom && PreviousCustomMode == CMOVE_Rope)
	{
		FinishRope();
	}

	if (IsCustomMovementMode(CMOVE_Rope))
	{
		EnterRope();
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
	// Sim.Proxies get replicated hook state
	if (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)
	{
		if (Safe_bWantsToHook && TryHook())
		{
			Proxy_FindHook = !Proxy_FindHook;
			SetMovementMode(MOVE_Custom, CMOVE_Hook);
		}
		else if (!Safe_bWantsToHook && IsHooking())
		{
			SetMovementMode(MOVE_Falling);
		}
	}

	if (ExhibitionCharacterRef->bCustomPressedJump)
	{
		if (!IsOnRope() && TryRope())
		{
			ExhibitionCharacterRef->StopJumping();
		}
		else if (IsOnRope())
		{
			ExhibitionCharacterRef->bCustomPressedJump = false;
			SetMovementMode(MOVE_Falling);
		}
		else
		{
			ExhibitionCharacterRef->bCustomPressedJump = false;
			CharacterOwner->bPressedJump = true;
			CharacterOwner->CheckJumpInput(DeltaSeconds);
			bOrientRotationToMovement = true;
		}
	}

	// Check if a montage ended
	if (CharacterOwner->GetCurrentMontage() == nullptr)
	{
		OnFinishMontage(CurrentAnimMontage);
	}
	
	Safe_bWantsToDive = false;
	Super::UpdateCharacterStateBeforeMovement(DeltaSeconds);
}

void UExhibitionMovementComponent::UpdateCharacterStateAfterMovement(float DeltaSeconds)
{
	Super::UpdateCharacterStateAfterMovement(DeltaSeconds);

	const TSharedPtr<FRootMotionSource> HookMotionSource = GetRootMotionSource(FName(HOOK_TRAVEL_NAME));
	const TSharedPtr<FRootMotionSource> RopeMotionSource = GetRootMotionSource(FName(ROPE_TRAVEL_NAME));
	if (IsRootMotionEnded(HookMotionSource))
	{
		OnCompleteTravel(true, HookBrakingFactor);
	}

	if (IsRootMotionEnded(RopeMotionSource))
	{
		OnCompleteTravel(false, RopeBrakingFactor);
	}
	
	const TSharedPtr<FRootMotionSource> CurrentTransition = GetRootMotionSourceByID(CurrentTransitionId);
	if (IsRootMotionEnded(CurrentTransition))
	{
		if (CVarDebugMovement->GetBool())
		{
			SCREEN_LOG(FString::Printf(TEXT("Transition ended: %s"), *CurrentTransition->InstanceName.ToString()), FColor::Green);
		}
		
		HandleEndTransition(*CurrentTransition.Get());
		RemoveRootMotionSourceByID(CurrentTransitionId);
	}
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
	float SelectedHookDistanceSrd = FMath::Pow(MaxHookDistance, 2);
	float SelectedDotResult = 0.f;
	for (AActor* Hook : AllHooks)
	{
		float HookDistance = 0.f, DotResult = 0.f;
		bool bIsBlocked = false;
		
		if (CanUseHook(Hook, HookDistance, DotResult, bIsBlocked) &&
			HookDistance <= SelectedHookDistanceSrd &&
			DotResult >= SelectedDotResult
		)
		{
			SelectedHook = Hook;
			SelectedHookDistanceSrd = HookDistance;
			SelectedDotResult = DotResult;
		}
	}

	CurrentHook = SelectedHook;
	const FVector Destination = (SelectedHook != nullptr)? SelectedHook->GetActorLocation() : FVector::ZeroVector;
	PrepareTravel(HOOK_TRAVEL_NAME, Destination, ReleaseHookTolerance, FVector::ZeroVector, MaxHookSpeed, HookCurve);
	return SelectedHook != nullptr;
}

bool UExhibitionMovementComponent::CanUseHook(const AActor* Hook, float& DistSqr, float& DotResult, bool& bIsBlocked) const
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

	if (CVarDebugMovement->GetBool())
	{
		LINE(CharacterLocation, CharacterLocation + ControlLook * 500.f, FColor::Red);
		LINE(CharacterLocation, CharacterLocation + ControlLookToHook * 500.f, FColor::Green);
	}

	DistSqr = FVector::DistSquared(CharacterLocation, HookLocation);
	const bool bNear = DistSqr <= FMath::Pow(MaxHookDistance, 2);

	if (!bNear)
	{
		return false;
	}

	DotResult = ControlLook | ControlLookToHook;
	const bool bInFov = DotResult >= 0.8f;
	if (!bInFov)
	{
		return false;
	}

	FCollisionShape Capsule = FCollisionShape::MakeCapsule(GetCapsuleRadius(), GetCapsuleHalfHeight());
	FHitResult Hit;
	FCollisionQueryParams IgnoreParams = ExhibitionCharacterRef->GetIgnoreCollisionParams();
	IgnoreParams.AddIgnoredActor(Hook);
	bIsBlocked = GetWorld()->SweepSingleByProfile(
		Hit,
		CharacterLocation,
		HookLocation,
		FRotator::ZeroRotator.Quaternion(),
		FName(TEXT("BlockAll")),
		Capsule,
		IgnoreParams
	);

	if (bIsBlocked && CVarDebugMovement->GetBool())
	{
		CAPSULE(Hit.Location, Capsule.GetCapsuleHalfHeight(), Capsule.GetCapsuleRadius(), FColor::Red);
	}
	
	return !bIsBlocked;
}

void UExhibitionMovementComponent::EnterHook()
{
	bOrientRotationToMovement = false;

	ApplyTravel();
	
	if (bHandleCable)
	{
		ToggleHookCable();
	}

	OnEnterHook.Broadcast();
}

void UExhibitionMovementComponent::FinishHook()
{
	Safe_bWantsToHook = false;
	bOrientRotationToMovement = true;
	TravelData->Reset();
	CurrentHook = nullptr;
	RemoveRootMotionSource(FName(HOOK_TRAVEL_NAME));
	
	if (bHandleCable)
	{
		ResetHookCable();
	}

	OnExitHook.Broadcast();
}

void UExhibitionMovementComponent::ToggleHookCable()
{
	ensure(CharacterOwner != nullptr);

	UCableComponent* HookCable = CharacterOwner->FindComponentByClass<UCableComponent>();
	if (HookCable)
	{
		HookCable->bAttachEnd = true;
		HookCable->bAttachStart = true;
		HookCable->SetHiddenInGame(!HookCable->bHiddenInGame);
		CurrentCableTime = 0.f;
	}
}

void UExhibitionMovementComponent::UpdateHookCable(const float DeltaTime)
{
	ensure(CharacterOwner != nullptr);

	UCableComponent* HookCable = CharacterOwner->FindComponentByClass<UCableComponent>();
	const FVector TravelDestinationLocation = TravelData->Destination;
	
	if (HookCable && TravelDestinationLocation != FVector::ZeroVector)
	{
		const FVector HandLocation = (!HookSocketName.IsNone())? CharacterOwner->GetMesh()->GetSocketLocation(HookSocketName) : UpdatedComponent->GetComponentLocation();
		const FVector TargetToHand = (TravelDestinationLocation - HandLocation).GetSafeNormal();
		
		const float MaxLength = FVector::Dist(HookCable->GetComponentLocation(), TravelDestinationLocation);
		float CurrentLength = MaxLength;
		
		CurrentCableTime = FMath::Clamp(CurrentCableTime + DeltaTime, 0.f, CableTimeToReachDestination);
		FVector EndLocation = TravelDestinationLocation;
		if (CurrentCableTime < CableTimeToReachDestination)
		{
			const float TimeRatio = FMath::Clamp(CableCurve.GetRichCurveConst()->Eval(CurrentCableTime / CableTimeToReachDestination), 0.f, 1.f);
			CurrentLength = FMath::Lerp(0.f, MaxLength, TimeRatio);
			EndLocation = HandLocation + (TargetToHand * CurrentLength);
		}

		HookCable->SetUsingAbsoluteLocation(true);
		HookCable->SetWorldLocation(EndLocation);
		HookCable->CableLength = CurrentLength;
	}
}

void UExhibitionMovementComponent::ResetHookCable()
{
	ensure(CharacterOwner != nullptr);

	UCableComponent* HookCable = CharacterOwner->FindComponentByClass<UCableComponent>();
	if (HookCable)
	{
		ToggleHookCable();
		HookCable->SetWorldLocation(UpdatedComponent->GetComponentLocation());
		HookCable->SetUsingAbsoluteLocation(false);
		HookCable->CableLength = 0.f;
	}
}

#pragma endregion

#pragma region Rope

bool UExhibitionMovementComponent::TryRope()
{
	if (!CharacterOwner->CanJump())
	{
		return false;
	}
	
	const FVector StartTrace = UpdatedComponent->GetComponentLocation() + FVector::UpVector * GetCapsuleHalfHeight();
	const float JumpHeight = GetMaxJumpHeightWithJumpTime();
	FCollisionShape CollisionCapsule = FCollisionShape::MakeCapsule(GetCapsuleRadius(), GetCapsuleHalfHeight());

	const float Additive = FMath::Clamp(JumpAdditive, 0.f, 100.f);
	const FVector JumpVelocity = {Velocity.X, Velocity.Y, JumpHeight + (GetCapsuleHalfHeight() * 2) + Additive};
	FPredictProjectilePathParams JumpSimulatePath;
	JumpSimulatePath.StartLocation = StartTrace;
	JumpSimulatePath.LaunchVelocity = JumpVelocity;
	JumpSimulatePath.ActorsToIgnore = ExhibitionCharacterRef->GetIgnoredActors();
	JumpSimulatePath.bTraceWithCollision = true;
	JumpSimulatePath.bTraceWithChannel = true;
	JumpSimulatePath.TraceChannel = ECC_WorldStatic;
	JumpSimulatePath.ProjectileRadius = GetCapsuleRadius();

	if (CVarDebugMovement->GetBool())
	{
		JumpSimulatePath.DrawDebugType = EDrawDebugTrace::ForDuration;
		JumpSimulatePath.DrawDebugTime = 5.f;
	}

	FPredictProjectilePathResult JumpSimulateResult;
	const bool bFoundSomething = UGameplayStatics::PredictProjectilePath(
		GetWorld(),
		JumpSimulatePath,
		JumpSimulateResult
	);

	if (!bFoundSomething)
	{
		return false;
	}
	
	const FHitResult Hit = JumpSimulateResult.HitResult;
	if (!Hit.GetActor() || !Hit.GetActor()->ActorHasTag(TagRopeName))
	{
		return false;
	}
	
	const AActor* HitActor = Hit.GetActor();
	UCableComponent* ActualRope = HitActor->FindComponentByClass<UCableComponent>();
	if (ActualRope == nullptr)
	{
		return false;
	}

	FVector StartRope, EndRope;
	GetRopePositions(ActualRope, StartRope, EndRope);
	const FVector RopeNormal = (EndRope - StartRope).GetSafeNormal();
	
	const FVector HitOnRope = FMath::ClosestPointOnSegment(Hit.Location, StartRope, EndRope);
	const float DownFactor = FMath::Clamp(RopeGrabFactor, 0.f, 1.f);
	const FVector TransitionDestination = HitOnRope + FVector::DownVector * (GetCapsuleHalfHeight() * DownFactor);

	const FVector RealDestination = EndRope + (-RopeNormal * GetCapsuleRadius() * 4) + (FVector::DownVector * GetCapsuleHalfHeight());

	if (CVarDebugMovement->GetBool())
	{
		POINT(HitOnRope, 25.f, FColor::Blue);
		CAPSULE(TransitionDestination, GetCapsuleHalfHeight(), GetCapsuleRadius(), FColor::Blue);
		CAPSULE(RealDestination, GetCapsuleHalfHeight(), GetCapsuleRadius(), FColor::Blue);
	}

	const float IgnoreRopeDistSqr = FMath::Square(IgnoreRopeDistance);
	if (FVector::DistSquared(StartTrace, RealDestination) <= IgnoreRopeDistSqr)
	{
		return false;
	}

	FCollisionQueryParams QueryParams = ExhibitionCharacterRef->GetIgnoreCollisionParams();
	QueryParams.TraceTag = FName(TEXT("RopeUnReachable"));
	QueryParams.AddIgnoredActor(HitActor);
	FHitResult UnReachable;
	const bool bUnReachable = GetWorld()->
		SweepSingleByProfile(
			UnReachable,
			TransitionDestination,
			RealDestination,
			UpdatedComponent->GetComponentQuat(),
			FName(TEXT("BlockAll")),
			CollisionCapsule,
			QueryParams
	);

	if (bUnReachable)
	{
		if (CVarDebugMovement->GetBool())
		{
			CAPSULE(UnReachable.Location, GetCapsuleHalfHeight(), GetCapsuleRadius(), FColor::Red);
		}
		
		return false;
	}

	PrepareTravel(ROPE_TRAVEL_NAME, RealDestination, RopeReleaseTolerance, RopeNormal, MaxRopeSpeed, RopeSpeedCurve);

	const float TravelDistance = FVector::Dist(RealDestination, UpdatedComponent->GetComponentLocation());
	const float JumpToRopeDuration = FMath::Clamp(TravelDistance / 500.f, 0.1, JumpToRopeMaxDuration);

	PlayMontage(HangToRopeMontage);
	if (IsServer())
	{
		Proxy_FindRope = !Proxy_FindRope;
	}
	SetMovementMode(MOVE_Flying);
	ApplyTransition(ROPE_TRANSITION_NAME, TransitionDestination, JumpToRopeDuration);
	return true;
}

void UExhibitionMovementComponent::EnterRope()
{
	bOrientRotationToMovement = false;

	if (GetOwnerRole() != ROLE_SimulatedProxy)
	{
		ApplyTravel();
	}

	OnEnterRope.Broadcast();
}

void UExhibitionMovementComponent::FinishRope()
{
	bOrientRotationToMovement = true;

	TravelData->Reset();
	RemoveRootMotionSource(FName(ROPE_TRAVEL_NAME));

	OnExitRope.Broadcast();
}

void UExhibitionMovementComponent::GetRopePositions(const UCableComponent* Rope, FVector& StartPosition, FVector& EndPosition)
{
	if (Rope == nullptr)
	{
		return;
	}

	StartPosition = Rope->GetComponentLocation();
	const USceneComponent* EndComponent = Cast<USceneComponent>(Rope->AttachEndTo.GetComponent(Rope->GetOwner()));
	if(EndComponent == nullptr)
	{
		EndComponent = Rope;
	}

	if (Rope->AttachEndToSocketName != NAME_None)
	{
		EndPosition = EndComponent->GetSocketTransform(Rope->AttachEndToSocketName).TransformPosition(Rope->EndLocation);
	}
	else
	{
		EndPosition = EndComponent->GetComponentTransform().TransformPosition(Rope->EndLocation);
	}
}

#pragma endregion 

bool UExhibitionMovementComponent::IsRootMotionEnded(const TSharedPtr<FRootMotionSource>& Source)
{
	if (!Source.IsValid())
	{
		return false;
	}

	return Source->Status.HasFlag(ERootMotionSourceStatusFlags::Finished) ||
		Source->Status.HasFlag(ERootMotionSourceStatusFlags::MarkedForRemoval);
}

#pragma region Travel

FTravelData::FTravelData()
	: TravelName(NAME_None), Destination(FVector::ZeroVector), bHasTolerance(false), Tolerance(0.f), bHasNormal(false), Normal(FVector::ZeroVector), Speed(0.f), SpeedCurve(nullptr)
{
	
}

void FTravelData::Fill(const FName& InTravelName, const FVector& InDestination, const float InTolerance, const FVector& InNormal, const float InSpeed, UCurveFloat* InSpeedCurve)
{
	TravelName = InTravelName;
	
	Destination = InDestination;

	bHasTolerance = (InTolerance > 0.f);
	Tolerance = InTolerance;
	
	bHasNormal = (!InNormal.IsZero());
	Normal = InNormal;

	Speed = InSpeed;
	SpeedCurve = InSpeedCurve;
}

void FTravelData::Reset()
{
	TravelName = NAME_None;
	
	Destination = FVector::ZeroVector;
	
	bHasTolerance = false;
	Tolerance = 0.f;

	bHasNormal = 0.f;
	Normal = FVector::ZeroVector;

	Speed = 0.f;
	SpeedCurve = nullptr;
}

void UExhibitionMovementComponent::PrepareTravel(const FString& TravelName, const FVector Destination, const float Tolerance, const FVector TravelNormal, const float MaxSpeed, UCurveFloat* Curve)
{
	if (!TravelData.IsSet())
	{
		TravelData.Emplace();
	}
	else
	{
		TravelData->Reset();
	}

	TravelData->Fill(
		FName(TravelName),
		Destination,
		Tolerance,
		TravelNormal,
		MaxSpeed,
		Curve
	);
}

uint16 UExhibitionMovementComponent::ApplyTravel()
{
	if (!TravelData.IsSet())
	{
		return (uint16)ERootMotionSourceID::Invalid;
	}

	const float Radius = FVector::Dist(UpdatedComponent->GetComponentLocation(), TravelData->Destination) + 1.f;
	const TSharedPtr<FRootMotionSource_RadialForce> MoveToTransition = MakeShared<FRootMotionSource_RadialForce>();
	MoveToTransition->AccumulateMode = ERootMotionAccumulateMode::Override;
	MoveToTransition->Priority = 6;
	MoveToTransition->InstanceName = TravelData->TravelName;
	MoveToTransition->Radius = Radius;
	MoveToTransition->bIsPush = false;
	if (TravelData->SpeedCurve.IsValid())
	{
		MoveToTransition->StrengthOverTime = TravelData->SpeedCurve.Get();
	}
	MoveToTransition->Strength = TravelData->Speed;
	MoveToTransition->Location = TravelData->Destination;
	MoveToTransition->bUseFixedWorldDirection = false;

	Safe_bReachedDestination = false;
	Velocity = FVector::ZeroVector;
	return ApplyRootMotionSource(MoveToTransition);
}

void UExhibitionMovementComponent::OnCompleteTravel(const bool bNullifyVelocity, const float Factor)
{
	if (Safe_bReachedDestination && bNullifyVelocity)
	{
		Velocity = FVector::ZeroVector;
	}
	else
	{
		Velocity -= Velocity / (1.f + (1.f - Factor));
	}

	Safe_bReachedDestination = false;
}

uint16 UExhibitionMovementComponent::ApplyTransition(const FString& TransitionName, const FVector& Destination, const float Duration)
{
	const TSharedPtr<FRootMotionSource_MoveToForce> NewTransition = MakeShared<FRootMotionSource_MoveToForce>();
	NewTransition->StartLocation = UpdatedComponent->GetComponentLocation();
	NewTransition->TargetLocation = Destination;
	NewTransition->InstanceName = FName(TransitionName);
	NewTransition->AccumulateMode = ERootMotionAccumulateMode::Override;
	NewTransition->Duration = Duration;

	if (CVarDebugMovement->GetBool())
	{
		CAPSULE(Destination, GetCapsuleHalfHeight(), GetCapsuleRadius(), FColor::Purple);
	}

	Velocity = FVector::ZeroVector;
	CurrentTransitionId = ApplyRootMotionSource(NewTransition);

	return CurrentTransitionId;
}

void UExhibitionMovementComponent::HandleEndTransition(const FRootMotionSource& Source)
{
	if (Source.InstanceName.IsEqual(FName(ROPE_TRANSITION_NAME)))
	{
		SetMovementMode(MOVE_Custom, CMOVE_Rope);
	}
}

void UExhibitionMovementComponent::PhysTravel(float deltaTime, int32 Iterations)
{
	if (deltaTime < MIN_TICK_TIME)
	{
		return;
	}
	
	if (!CharacterOwner || (!CharacterOwner->Controller && !bRunPhysicsWithNoController && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && (CharacterOwner->GetLocalRole() != ROLE_SimulatedProxy)))
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (TravelData->Destination.IsZero())
	{
		Acceleration = FVector::ZeroVector;
		Velocity = FVector::ZeroVector;
		return;
	}

	if (TravelData->bHasTolerance)
	{
		if (UpdatedComponent->GetComponentLocation().Equals(TravelData->Destination, TravelData->Tolerance))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(deltaTime, Iterations);
			Safe_bReachedDestination = true;
			return;
		}
	}

	RestorePreAdditiveRootMotionVelocity();

	if( !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() )
	{
		if( bCheatFlying && Acceleration.IsZero() )
		{
			Velocity = FVector::ZeroVector;
		}
		CalcVelocity(deltaTime, FallingLateralFriction, true, GetMaxBrakingDeceleration());
	}

	ApplyRootMotionToVelocity(deltaTime);

	Iterations++;
	bJustTeleported = false;

	if (TravelData->bHasNormal)
	{
		Velocity = Velocity.ProjectOnToNormal(TravelData->Normal);
	}
	
	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	const FVector Adjusted = Velocity * deltaTime;
	FHitResult Hit(1.f);

	FRotator NewRotation = (TravelData->Destination - UpdatedComponent->GetComponentLocation()).Rotation();
	NewRotation.Pitch = 0.f;
	NewRotation.Roll = 0.f;

	SafeMoveUpdatedComponent(Adjusted, NewRotation, true, Hit);

	// Make velocity reflect actual move
	if( !bJustTeleported && !HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity() && deltaTime >= MIN_TICK_TIME)
	{
		Velocity = (UpdatedComponent->GetComponentLocation() - OldLocation) / deltaTime;
	}

	if (UpdatedComponent->GetComponentLocation() == OldLocation)
	{
		return;
	}

	if (TravelData->bHasTolerance)
	{
		if (UpdatedComponent->GetComponentLocation().Equals(TravelData->Destination, TravelData->Tolerance))
		{
			SetMovementMode(MOVE_Falling);
			StartNewPhysics(deltaTime, Iterations);
			Safe_bReachedDestination = true;
			return;
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

bool UExhibitionMovementComponent::IsOnRope() const
{
	return IsCustomMovementMode(CMOVE_Rope);
}

bool UExhibitionMovementComponent::IsServer() const
{
	return CharacterOwner->HasAuthority();
}

void UExhibitionMovementComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(UExhibitionMovementComponent, Proxy_Dive, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UExhibitionMovementComponent, Proxy_JumpExtra, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UExhibitionMovementComponent, Proxy_FindHook, COND_SkipOwner);
	DOREPLIFETIME_CONDITION(UExhibitionMovementComponent, Proxy_FindRope, COND_SkipOwner);
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

void UExhibitionMovementComponent::OnRep_FindRope()
{
	CharacterOwner->PlayAnimMontage(HangToRopeMontage);
}

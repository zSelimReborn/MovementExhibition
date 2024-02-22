// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ExhibitionMovementComponent.generated.h"

class AExhibitionCharacter;
class UAnimMontage;
class UCableComponent;

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_Hook			UMETA(DisplayName = "Hook"),
	CMOVE_Rope			UMETA(DisplayName = "On Rope"),
	CMOVE_MAX			UMETA(Hidden),
};

USTRUCT()
struct FTravelData
{
	GENERATED_BODY()

	FName TravelName;
	
	FVector Destination;
	
	bool bHasTolerance;
	float Tolerance;

	bool bHasNormal;
	FVector Normal;

	float Speed;
	TWeakObjectPtr<UCurveFloat> SpeedCurve;

	FTravelData();
	void Fill(const FName& InTravelName, const FVector& InDestination, const float InTolerance, const FVector& InNormal, const float InSpeed, UCurveFloat* InSpeedCurve);
	void Reset();
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterSlideDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitSlideDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDiveDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterHookDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitHookDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterRopeDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitRopeDelegate);

/**
 * 
 */
UCLASS()
class MOVEMENTEXHIBITION_API UExhibitionMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	// SavedMove struct

	class FSavedMove_Exhibition : public FSavedMove_Character
	{
	public:
		FSavedMove_Exhibition();

		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;
		virtual void Clear() override;
		virtual uint8 GetCompressedFlags() const override;
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;
		virtual void PrepMoveFor(ACharacter* C) override;
		
		// Flags
		uint8 Saved_bWantsToSprint:1 = false;
		uint8 Saved_bWantsToDive:1 = false;
		uint8 Saved_bWantsToHook:1 = false;
		uint8 Saved_bCustomPressedJump:1 = false;

		// Vars
		uint8 Saved_bPrevWantsToCrouch:1 = false;
		uint8 Saved_bReachedDestination:1 = false;
		int32 Saved_FlyingDiveCount = 0;
	};

	class FNetworkPredictionData_Client_Exhibition : public FNetworkPredictionData_Client_Character
	{
	public: 
		using Super = FNetworkPredictionData_Client_Character;
		
		FNetworkPredictionData_Client_Exhibition(const UCharacterMovementComponent& ClientMovement);
		
		virtual FSavedMovePtr AllocateNewMove() override;
	};

// CMC Specific
public:
	UExhibitionMovementComponent();

	virtual void InitializeComponent() override;

	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	virtual float GetMaxSpeed() const override;

	virtual float GetMaxBrakingDeceleration() const override;

	virtual bool IsMovingOnGround() const override;

	virtual bool CanAttemptJump() const override;

	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

	virtual bool DoJump(bool bReplayingMoves) override;

protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

// Helpers
protected:
	FORCEINLINE bool IsMovementMode(const EMovementMode& Mode) const { return MovementMode ==  Mode;}
	FORCEINLINE bool IsCustomMovementMode(const ECustomMovementMode& InMovementMode) const;

	void PlayMontage(UAnimMontage* InMontage);

	void OnFinishMontage(const UAnimMontage* Montage);
	
	bool IsAuthProxy() const;
	
	float GetCapsuleRadius() const;

	float GetCapsuleHalfHeight() const;

	void ToggleHookCable();

	void UpdateHookCable(const float DeltaTime);

	void ResetHookCable();

	static bool IsRootMotionEnded(const TSharedPtr<FRootMotionSource>&);
	
// Movement modes
protected:
	// Sliding
	void EnterSlide();
	
	void FinishSlide();

	bool CanSlide() const;
	
	void PhysSlide(float deltaTime, int32 Iterations);

	// Diving
	void PerformDive();

	bool CanDive() const;

	// Hooking
	bool TryHook();

	bool CanUseHook(const AActor* Hook, float& DistSqr, float& DotResult, bool& bIsBlocked) const;

	void EnterHook();

	void FinishHook();
	
	// Rope
	bool TryRope();

	void EnterRope();

	void FinishRope();

	static void GetRopePositions(const UCableComponent* Rope, FVector& StartPosition, FVector& EndPosition);
	
	// Travel to destination
	void PhysTravel(float deltaTime, int32 Iterations);

	void PrepareTravel(const FString& TravelName, const FVector Destination, const float Tolerance, const FVector TravelNormal, const float MaxSpeed, UCurveFloat* Curve);

	uint16 ApplyTravel();

	void OnCompleteTravel(const bool bNullifyVelocity, const float Factor);

	// Generic transitions
	uint16 ApplyTransition(const FString& TransitionName, const FVector& Destination, const float Duration);

	void HandleEndTransition(const FRootMotionSource&);
	
// Interface
public:
	UFUNCTION(BlueprintCallable)
	void ToggleCrouch();

	UFUNCTION(BlueprintCallable)
	void ToggleSprint();
	
	UFUNCTION(BlueprintPure)
	bool CanSprint() const;

	UFUNCTION(BlueprintPure)
	bool IsSprinting() const;

	UFUNCTION(BlueprintPure)
	bool IsSliding() const;

	UFUNCTION(BlueprintCallable)
	void RequestDive();

	UFUNCTION(BlueprintCallable)
	bool IsDiving() const;
	
	UFUNCTION(BlueprintCallable)
	void RequestHook();

	UFUNCTION(BlueprintCallable)
	void ReleaseHook();
	
	UFUNCTION(BlueprintPure)
	bool IsHooking() const;

	UFUNCTION(BlueprintPure)
	bool IsOnRope() const;

	UFUNCTION(BlueprintPure)
	bool IsServer() const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetInitialCapsuleHalfHeight() const { return InitialCapsuleHalfHeight; };

// Replication
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Dive();

	UFUNCTION()
	void OnRep_JumpExtra();

	UFUNCTION()
	void OnRep_FindHook();

	UFUNCTION()
	void OnRep_FindRope();

// CMC Safe Properties
protected:
	bool Safe_bWantsToSprint = false;

	bool Safe_bPrevWantsToCrouch = false;

	bool Safe_bWantsToDive = false;

	bool Safe_bWantsToHook = false;

	uint16 CurrentTransitionId;

// FSavedMove properties
protected:
	int32 Safe_FlyingDiveCount = 0;

	bool Safe_bReachedDestination = false;
	
// Replication properties
protected:
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Dive)
	bool Proxy_Dive = false;
	
	UPROPERTY(Transient, ReplicatedUsing=OnRep_JumpExtra)
	bool Proxy_JumpExtra = false;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_FindHook)
	bool Proxy_FindHook = false;

	UPROPERTY(Transient, ReplicatedUsing=OnRep_FindRope)
	bool Proxy_FindRope = false;
	
// Standard Properties
protected:
	UPROPERTY(EditAnywhere, Category="Exhibition|Sprint")
	float MaxSprintSpeed = 750.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Sprint")
	float MaxSprintCrouchedSpeed = 350.f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Slide")
	float SlideMinSpeed = 600.f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Slide")
	float SlideEnterImpulse = 700.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Slide")
	float SlideGravityForce = 4000.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Slide")
	float SlideFrictionFactor = 0.06f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Slide")
	float SlideBrakingDeceleration = 1000.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Dive")
	float DiveImpulse = 1000.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Dive")
	float FlyingDiveImpulse = 500.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Dive")
	float DodgeBackImpulse = 750.f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Dive")
    float DiveMinSpeed = 400.f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Dive")
	TObjectPtr<UAnimMontage> DiveMontage;

	UPROPERTY(EditAnywhere, Category="Exhibition|Dive")
	TObjectPtr<UAnimMontage> FlyingDiveMontage;

	UPROPERTY(EditAnywhere, Category="Exhibition|Dive")
	TObjectPtr<UAnimMontage> DodgeBackMontage;

	UPROPERTY(EditAnywhere, Category="Exhibition|Jump")
	TObjectPtr<UAnimMontage> JumpExtraMontage;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	FName TagHookName = NAME_None;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	float MaxHookDistance = 5000.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	float IgnoreHookDistance = 100.f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	float MaxHookSpeed = 600.f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	float ReleaseHookTolerance = 10.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook", meta=(ClampMin=0.f, ClampMax=1.f))
	float HookBrakingFactor = 0.9f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	TObjectPtr<UCurveFloat> HookCurve;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	bool bHandleCable = true;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook", meta=(EditCondition=bHandleCable))
	float CableTimeToReachDestination = 0.2f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook", meta=(EditCondition=bHandleCable))
	FName HookSocketName = NAME_None;

	UPROPERTY(Transient)
	float CurrentCableTime = 0.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook", meta=(EditCondition=bHandleCable))
	FRuntimeFloatCurve CableCurve;
	
	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentHook;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope")
	FName TagRopeName = NAME_None;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Rope", meta=(ClampMin=0.f, ClampMax=100.f))
	float JumpAdditive = 35.f;
	
	UPROPERTY(EditAnywhere, Category="Exhibition|Rope", meta=(ClampMin=0.1f))
    float JumpToRopeMaxDuration = 0.2f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope", meta=(ClampMin=0.f, ClampMax=1.f))
	float RopeGrabFactor = 0.8f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope")
	float MaxRopeSpeed = 800.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope")
	float RopeReleaseTolerance = 50.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope")
	float IgnoreRopeDistance = 200.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope", meta=(ClampMin=0.f, ClampMax=1.f))
	float RopeBrakingFactor = 0.8f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope")
	TObjectPtr<UAnimMontage> HangToRopeMontage;

	UPROPERTY(EditAnywhere, Category="Exhibition|Rope")
	TObjectPtr<UCurveFloat> RopeSpeedCurve;

	TOptional<FTravelData> TravelData;
	
	// TODO Probably not the ideal solution
	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentAnimMontage;
	
	UPROPERTY(Transient)
	float InitialCapsuleHalfHeight = 88.f;

	UPROPERTY(Transient)
	TObjectPtr<AExhibitionCharacter> ExhibitionCharacterRef;

// Delegates
protected:
	UPROPERTY(BlueprintAssignable, Category="Exhibition Events")
	FOnEnterSlideDelegate OnEnterSlide;

	UPROPERTY(BlueprintAssignable, Category="Exhibition Events")
	FOnExitSlideDelegate OnExitSlide;

	UPROPERTY(BlueprintAssignable, Category="Exhibition Events")
	FOnDiveDelegate OnDive;

	UPROPERTY(BlueprintAssignable, Category="Exhibition Events")
	FOnEnterHookDelegate OnEnterHook;

	UPROPERTY(BlueprintAssignable, Category="Exhibition Events")
	FOnExitHookDelegate OnExitHook;

	UPROPERTY(BlueprintAssignable, Category="Exhibition Events")
	FOnEnterRopeDelegate OnEnterRope;

	UPROPERTY(BlueprintAssignable, Category="Exhibition Events")
	FOnExitRopeDelegate OnExitRope;

// Constants
public:
	static const FString HOOK_TRAVEL_NAME;

	static const FString ROPE_TRAVEL_NAME;
	static const FString ROPE_TRANSITION_NAME;
};

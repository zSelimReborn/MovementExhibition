// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ExhibitionMovementComponent.generated.h"

class AExhibitionCharacter;
class UAnimMontage;

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_Hook			UMETA(DisplayName = "Hook"),
	CMOVE_MAX			UMETA(Hidden),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEnterSlideDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnExitSlideDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDiveDelegate);

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

		// Vars
		uint8 Saved_bPrevWantsToCrouch:1 = false;
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

	float GetAngleInDegrees(const FVector& First, const FVector& Second) const;
	
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

	bool CanUseHook(const AActor* Hook) const;

	void PhysHook(float deltaTime, int32 Iterations);
	
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
	FORCEINLINE float GetInitialCapsuleHalfHeight() const { return InitialCapsuleHalfHeight; };

// Replication
public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_Dive();

	UFUNCTION()
	void OnRep_JumpExtra();

// CMC Safe Properties
protected:
	bool Safe_bWantsToSprint = false;

	bool Safe_bPrevWantsToCrouch = false;

	bool Safe_bWantsToDive = false;

	bool Safe_bWantsToHook = false;

// FSavedMove properties
protected:
	int32 Safe_FlyingDiveCount = 0;
	
// Replication properties
protected:
	UPROPERTY(Transient, ReplicatedUsing=OnRep_Dive)
	bool Proxy_Dive = false;
	
	UPROPERTY(Transient, ReplicatedUsing=OnRep_JumpExtra)
	bool Proxy_JumpExtra = false;
	
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
	float MinHookDistance = 5000.f;

	UPROPERTY(EditAnywhere, Category="Exhibition|Hook")
	float MaxHookAngle = 40.f;

	UPROPERTY(Transient)
	float HookTotalTime = 0.f;
	
	UPROPERTY(Transient)
	float HookCurrentDistance = 0.f;

	UPROPERTY(Transient)
	TObjectPtr<AActor> CurrentHook;

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
};

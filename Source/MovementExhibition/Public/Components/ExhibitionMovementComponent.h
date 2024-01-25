// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ExhibitionMovementComponent.generated.h"

class AExhibitionCharacter;

UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_None			UMETA(Hidden),
	CMOVE_Slide			UMETA(DisplayName = "Slide"),
	CMOVE_MAX			UMETA(Hidden),
};

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

		// Vars
		uint8 Saved_bPrevWantsToCrouch:1 = false;
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

protected:
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

// Helpers
protected:
	FORCEINLINE bool IsMovementMode(const EMovementMode& Mode) const { return MovementMode ==  Mode;}
	FORCEINLINE bool IsCustomMovementMode(const ECustomMovementMode& InMovementMode) const;
	
// Movement modes
protected:
	// Sliding
	void EnterSlide();
	
	void FinishSlide();

	bool CanSlide() const;
	
	void PhysSlide(float deltaTime, int32 Iterations);
	
	
// Interface
public:
	UFUNCTION(BlueprintCallable)
	void ToggleCrouch();

	UFUNCTION(BlueprintCallable)
	void RequestSprint();

	UFUNCTION(BlueprintCallable)
	void FinishSprint();

	UFUNCTION(BlueprintPure)
	bool CanSprint() const;

	UFUNCTION(BlueprintPure)
	bool IsSprinting() const;

	UFUNCTION(BlueprintPure)
	bool IsSliding() const;

	UFUNCTION(BlueprintPure)
	FORCEINLINE float GetInitialCapsuleHalfHeight() const { return InitialCapsuleHalfHeight; };

// CMC Safe Properties
protected:
	bool Safe_bWantsToSprint = false;

	bool Safe_bPrevWantsToCrouch = false;
	
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
	
	UPROPERTY(Transient)
	float InitialCapsuleHalfHeight = 88.f;

	UPROPERTY(Transient)
	TObjectPtr<AExhibitionCharacter> ExhibitionCharacterRef;
};

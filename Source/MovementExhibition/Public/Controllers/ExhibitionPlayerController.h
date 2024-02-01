// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ExhibitionPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;
class AExhibitionCharacter;

/**
 * 
 */
UCLASS()
class MOVEMENTEXHIBITION_API AExhibitionPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void AcknowledgePossession(APawn* P) override;
	
	virtual void OnUnPossess() override;

	virtual void SetupInputComponent() override;

	virtual void InitializeMappingContext();
	
public:
	void RequestMove(const FInputActionValue& InputAction);
	
	void RequestLook(const FInputActionValue& InputAction);
	
	void RequestJump();
	void RequestStopJump();
	
	void RequestCrouch();

	void RequestSprint();

	void RequestRoll();

	void RequestHook();
	void RequestReleaseHook();
	
// Inputs
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputAction> RollAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputAction> HookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input Actions")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

// Properties
protected:
	UPROPERTY(EditAnywhere, Category="Camera")
	float LookUpRate = 90.f;

	UPROPERTY(EditAnywhere, Category="Camera")
	float LookRightRate = 90.f; 
	
	UPROPERTY(Transient)
	TObjectPtr<AExhibitionCharacter> CharacterRef;
};

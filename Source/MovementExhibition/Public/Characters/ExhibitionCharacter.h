// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ExhibitionCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UExhibitionMovementComponent;

UCLASS()
class MOVEMENTEXHIBITION_API AExhibitionCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	AExhibitionCharacter(const FObjectInitializer&);
	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable)
	void ToggleCrouch();

	virtual void Jump() override;

	virtual void StopJumping() override;

	FORCEINLINE UExhibitionMovementComponent* GetExhibitionMovComponent() { return ExhibitionMovementComponent; };

	FCollisionQueryParams GetIgnoreCollisionParams() const;

// Flags
public:
	bool bCustomPressedJump = false;

// Components
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UCameraComponent> CameraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TObjectPtr<UExhibitionMovementComponent> ExhibitionMovementComponent;
};

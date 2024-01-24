// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ExhibitionCameraManager.generated.h"

class AExhibitionCharacter;

/**
 * 
 */
UCLASS()
class MOVEMENTEXHIBITION_API AExhibitionCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

protected:
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	void ComputeCrouch(FTViewTarget& OutVT, float DeltaTime);

// Properties
protected:
	UPROPERTY(EditDefaultsOnly, Category="Crouch")
	float CrouchLocationDuration = 1.f;

	UPROPERTY(Transient)
	float CrouchLocationTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Crouch")
	float CrouchOffsetFOV = 70.f;

	UPROPERTY(EditDefaultsOnly, Category="Crouch")
	float CrouchFOVDuration = .5f;

	UPROPERTY(Transient)
	float CrouchFOVTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category="Crouch")
	FRuntimeFloatCurve CrouchLocationCurve;

	UPROPERTY(EditDefaultsOnly, Category="Crouch")
	FRuntimeFloatCurve CrouchFOVCurve;
};

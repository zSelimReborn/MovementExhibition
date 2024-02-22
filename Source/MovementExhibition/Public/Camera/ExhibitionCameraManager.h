// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "ExhibitionCameraManager.generated.h"

class AExhibitionCharacter;
class UExhibitionMovementComponent;
class UCameraShakeBase;

/**
 * 
 */
UCLASS()
class MOVEMENTEXHIBITION_API AExhibitionCameraManager : public APlayerCameraManager
{
	GENERATED_BODY()

protected:
	void Setup();
	
	virtual void UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime) override;

	void ComputeCrouch(FTViewTarget& OutVT, float DeltaTime);
	
	void ComputeHook(FTViewTarget& OutVT, float DeltaTime);
	
	void ComputeRope(FTViewTarget& OutVT, float DeltaTime);
	
	void HandleCameraShake(float DeltaTime);

	static void ToggleCustomBlur(FTViewTarget& OutVT, const float BlurAmountOffset, const float BlurDistortionOffset, const bool bAdd);
	static void TogglePostProcessMaterial(FTViewTarget& OutVT, UMaterialInstance* Material, const bool bAdd);

public:
	virtual void InitializeFor(APlayerController* PC) override;

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

	UPROPERTY(EditDefaultsOnly, Category="Hooking")
	float HookBlurAmountOffset = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category="Hooking")
	float HookBlurMaxDistortionOffset = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Hooking")
	float HookBlurSpeedThreshold = 600.f;

	UPROPERTY(EditDefaultsOnly, Category="Hooking")
	TSoftObjectPtr<UMaterialInstance> HookSpeedLines;

	UPROPERTY(EditDefaultsOnly, Category="Rope")
	float RopeBlurAmountOffset = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category="Rope")
	float RopeBlurMaxDistortionOffset = 2.f;
	
	UPROPERTY(EditDefaultsOnly, Category="Rope")
	TSoftObjectPtr<UMaterialInstance> RopeSpeedLines;

	UPROPERTY(EditDefaultsOnly, Category="Camera Shake")
	TSubclassOf<UCameraShakeBase> IdleCameraShake;

	UPROPERTY(EditDefaultsOnly, Category="Camera Shake")
	TSubclassOf<UCameraShakeBase> WalkCameraShake;
	
	UPROPERTY(EditDefaultsOnly, Category="Camera Shake")
	TSubclassOf<UCameraShakeBase> SprintCameraShake;

	UPROPERTY(Transient)
	TObjectPtr<AExhibitionCharacter> CharacterRef;

	UPROPERTY(Transient)
	TObjectPtr<UExhibitionMovementComponent> MovementComponentRef;
};

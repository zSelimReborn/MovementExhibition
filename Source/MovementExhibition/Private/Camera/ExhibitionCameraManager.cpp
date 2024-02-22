// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/ExhibitionCameraManager.h"

#include "Characters/ExhibitionCharacter.h"
#include "Components/ExhibitionMovementComponent.h"

void AExhibitionCameraManager::Setup()
{
	if (CharacterRef != nullptr && MovementComponentRef != nullptr)
	{
		return;
	}
	
	const APlayerController* PC = GetOwningPlayerController();
	if (PC == nullptr)
	{
		return;
	}

	CharacterRef = Cast<AExhibitionCharacter>(PC->GetCharacter());
	if (CharacterRef == nullptr)
	{
		return;
	}

	MovementComponentRef = CharacterRef->GetExhibitionMovComponent();
}

void AExhibitionCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);

	Setup();
	ComputeCrouch(OutVT, DeltaTime);

	if (MovementComponentRef != nullptr)
	{
		if (MovementComponentRef->IsHooking())
		{
			ComputeHook(OutVT, DeltaTime);
		}
		if (MovementComponentRef->IsOnRope())
		{
			ComputeRope(OutVT, DeltaTime);
		}
	}

	HandleCameraShake(DeltaTime);
}

void AExhibitionCameraManager::ComputeCrouch(FTViewTarget& OutVT, float DeltaTime)
{
	if (MovementComponentRef == nullptr)
	{
		return;
	}

	float TimeOffset = DeltaTime;
	if (!MovementComponentRef->IsCrouching())
	{
		TimeOffset = -DeltaTime;
	}

	CrouchLocationTime = FMath::Clamp(CrouchLocationTime + TimeOffset, 0.f, CrouchLocationDuration);
	CrouchFOVTime = FMath::Clamp(CrouchFOVTime + TimeOffset, 0.f, CrouchFOVDuration);

	const float LocationRatio = CrouchLocationCurve.GetRichCurveConst()->Eval(CrouchLocationTime / CrouchLocationDuration);
	const float FOVRatio = CrouchFOVCurve.GetRichCurveConst()->Eval(CrouchFOVTime / CrouchFOVDuration);

	const FVector TargetLocationOffset = {0.f, 0.f, MovementComponentRef->GetCrouchedHalfHeight() - MovementComponentRef->GetInitialCapsuleHalfHeight()};
	
	FVector LocationOffset = FMath::Lerp(FVector::ZeroVector, TargetLocationOffset, LocationRatio);
	const float FOVOffset = FMath::Lerp(0.f, -CrouchOffsetFOV, FOVRatio);
	
	if (MovementComponentRef->IsCrouching())
	{
		LocationOffset -= TargetLocationOffset;
	}
	
	OutVT.POV.Location += LocationOffset;
	OutVT.POV.FOV += FOVOffset;
}

void AExhibitionCameraManager::ComputeHook(FTViewTarget& OutVT, float DeltaTime)
{
	if (MovementComponentRef == nullptr)
	{
		return;
	}
	
	const float CurrentSpeedSqr = MovementComponentRef->Velocity.SizeSquared();
	if (MovementComponentRef->IsHooking() && CurrentSpeedSqr >= FMath::Square(HookBlurSpeedThreshold))
	{
		ToggleCustomBlur(OutVT, HookBlurAmountOffset, HookBlurMaxDistortionOffset, true);
		TogglePostProcessMaterial(OutVT, HookSpeedLines.LoadSynchronous(), true);
	}
	else if (OutVT.POV.PostProcessSettings.bOverride_MotionBlurAmount)
	{
		ToggleCustomBlur(OutVT, HookBlurAmountOffset, HookBlurMaxDistortionOffset, false);
		TogglePostProcessMaterial(OutVT, HookSpeedLines.LoadSynchronous(), false);
	}
}

void AExhibitionCameraManager::ComputeRope(FTViewTarget& OutVT, float DeltaTime)
{
	if (MovementComponentRef == nullptr)
	{
		return;
	}
	
	if (MovementComponentRef->IsOnRope())
	{
		ToggleCustomBlur(OutVT, RopeBlurAmountOffset, RopeBlurMaxDistortionOffset, true);
		TogglePostProcessMaterial(OutVT, RopeSpeedLines.LoadSynchronous(), true);
	}
	else if (OutVT.POV.PostProcessSettings.bOverride_MotionBlurAmount)
	{
		ToggleCustomBlur(OutVT, RopeBlurAmountOffset, RopeBlurMaxDistortionOffset, false);
		TogglePostProcessMaterial(OutVT, RopeSpeedLines.LoadSynchronous(), false);
	}
}

void AExhibitionCameraManager::HandleCameraShake(float DeltaTime)
{
	Setup();

	if (MovementComponentRef == nullptr)
	{
		return;
	}

	if (!MovementComponentRef->IsWalking())
	{
		StopAllCameraShakes();
	}

	const float SpeedSqr = MovementComponentRef->Velocity.SizeSquared();
	if (MovementComponentRef->IsSprinting())
	{
		StartCameraShake(SprintCameraShake);
	}
	else if (SpeedSqr > 0)
	{
		StartCameraShake(WalkCameraShake);
	}
	else
	{
		StartCameraShake(IdleCameraShake);
	}
}

void AExhibitionCameraManager::ToggleCustomBlur(FTViewTarget& OutVT, const float BlurAmountOffset, const float BlurDistortionOffset, const bool bAdd)
{
	float BlurAmount = BlurAmountOffset;
	float BlurDistortion = BlurDistortionOffset;
	
	if (!bAdd)
	{
		BlurAmount = - BlurAmountOffset;
		BlurDistortion = -BlurDistortionOffset;
	}
	
	OutVT.POV.PostProcessSettings.bOverride_MotionBlurAmount = bAdd;
	OutVT.POV.PostProcessSettings.bOverride_MotionBlurMax = bAdd;
	OutVT.POV.PostProcessSettings.MotionBlurAmount += BlurAmount;
	OutVT.POV.PostProcessSettings.MotionBlurMax += BlurDistortion;
}

void AExhibitionCameraManager::TogglePostProcessMaterial(FTViewTarget& OutVT, UMaterialInstance* Material, const bool bAdd)
{
	if (!OutVT.POV.PostProcessSettings.WeightedBlendables.Array.IsValidIndex(0))
	{
		OutVT.POV.PostProcessSettings.WeightedBlendables.Array.EmplaceAt(0, FWeightedBlendable(0.f, Material));
	}
	else
	{
		OutVT.POV.PostProcessSettings.WeightedBlendables.Array[0] = FWeightedBlendable(0.f, Material);
	}

	float MaterialWeight = 0.f;
	if (bAdd)
	{
		MaterialWeight = 1.f;	
	}

	OutVT.POV.PostProcessSettings.WeightedBlendables.Array[0].Weight = MaterialWeight;
}

void AExhibitionCameraManager::InitializeFor(APlayerController* PC)
{
	Super::InitializeFor(PC);

	Setup();
}

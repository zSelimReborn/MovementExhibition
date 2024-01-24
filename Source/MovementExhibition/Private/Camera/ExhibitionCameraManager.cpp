// Fill out your copyright notice in the Description page of Project Settings.


#include "Camera/ExhibitionCameraManager.h"

#include "Characters/ExhibitionCharacter.h"
#include "Components/ExhibitionMovementComponent.h"

void AExhibitionCameraManager::UpdateViewTarget(FTViewTarget& OutVT, float DeltaTime)
{
	Super::UpdateViewTarget(OutVT, DeltaTime);

	ComputeCrouch(OutVT, DeltaTime);
}

void AExhibitionCameraManager::ComputeCrouch(FTViewTarget& OutVT, float DeltaTime)
{
	AExhibitionCharacter* CharacterRef = Cast<AExhibitionCharacter>(GetOwningPlayerController()->GetCharacter());
	if (CharacterRef == nullptr)
	{
		return;
	}

	UExhibitionMovementComponent* ExhibitionMovementComponent = CharacterRef->GetExhibitionMovComponent();
	if (ExhibitionMovementComponent == nullptr)
	{
		return;
	}

	float TimeOffset = DeltaTime;
	if (!ExhibitionMovementComponent->IsCrouching())
	{
		TimeOffset = -DeltaTime;
	}

	CrouchLocationTime = FMath::Clamp(CrouchLocationTime + TimeOffset, 0.f, CrouchLocationDuration);
	CrouchFOVTime = FMath::Clamp(CrouchFOVTime + TimeOffset, 0.f, CrouchFOVDuration);

	const float LocationRatio = CrouchLocationCurve.GetRichCurveConst()->Eval(CrouchLocationTime / CrouchLocationDuration);
	const float FOVRatio = CrouchFOVCurve.GetRichCurveConst()->Eval(CrouchFOVTime / CrouchFOVDuration);

	const FVector TargetLocationOffset = {0.f, 0.f, ExhibitionMovementComponent->GetCrouchedHalfHeight() - ExhibitionMovementComponent->GetInitialCapsuleHalfHeight()};
	
	FVector LocationOffset = FMath::Lerp(FVector::ZeroVector, TargetLocationOffset, LocationRatio);
	const float FOVOffset = FMath::Lerp(0.f, -CrouchOffsetFOV, FOVRatio);
	
	if (ExhibitionMovementComponent->IsCrouching())
	{
		LocationOffset -= TargetLocationOffset;
	}
	
	OutVT.POV.Location += LocationOffset;
	OutVT.POV.FOV += FOVOffset;
}

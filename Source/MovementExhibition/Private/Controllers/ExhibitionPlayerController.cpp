// Fill out your copyright notice in the Description page of Project Settings.


#include "Controllers/ExhibitionPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Characters/ExhibitionCharacter.h"
#include "Components/ExhibitionMovementComponent.h"

void AExhibitionPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);
	CharacterRef = Cast<AExhibitionCharacter>(P);
}

void AExhibitionPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
	CharacterRef = nullptr;
}

void AExhibitionPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AExhibitionPlayerController::RequestMove);
		
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AExhibitionPlayerController::RequestLook);
		
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &AExhibitionPlayerController::RequestJump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &AExhibitionPlayerController::RequestStopJump);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &AExhibitionPlayerController::RequestCrouch);
		
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Triggered, this, &AExhibitionPlayerController::RequestSprint);

		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Triggered, this, &AExhibitionPlayerController::RequestRoll);

		EnhancedInputComponent->BindAction(HookAction, ETriggerEvent::Triggered, this, &AExhibitionPlayerController::RequestHook);

		InitializeMappingContext();
	}
}

void AExhibitionPlayerController::InitializeMappingContext()
{
	if (UEnhancedInputLocalPlayerSubsystem* EnhancedInputLocalPlayerSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		EnhancedInputLocalPlayerSubsystem->AddMappingContext(DefaultMappingContext, 0);
	}
}

void AExhibitionPlayerController::RequestMove(const FInputActionValue& InputAction)
{
	ensure(CharacterRef != nullptr);
	
	const FVector2D AxisValue = InputAction.Get<FVector2D>();

	const FRotator LookDirection = {0.f, GetControlRotation().Yaw, 0.f};
	const FRotationMatrix LookMatrix{LookDirection};
	
	const FVector ForwardDirection = LookMatrix.GetScaledAxis(EAxis::X);
	const FVector RightDirection = LookMatrix.GetScaledAxis(EAxis::Y);

	CharacterRef->AddMovementInput(ForwardDirection, AxisValue.X);
	CharacterRef->AddMovementInput(RightDirection, AxisValue.Y);
}

void AExhibitionPlayerController::RequestLook(const FInputActionValue& InputAction)
{
	const FVector2D AxisValue = InputAction.Get<FVector2D>();

	const float DeltaTime = GetWorld()->GetDeltaSeconds();
	AddYawInput(AxisValue.X * LookRightRate * DeltaTime);
	AddPitchInput(AxisValue.Y * LookUpRate * DeltaTime);
}

void AExhibitionPlayerController::RequestJump()
{
	ensure(CharacterRef != nullptr);

	CharacterRef->Jump();
}

void AExhibitionPlayerController::RequestStopJump()
{
	ensure(CharacterRef != nullptr);

	CharacterRef->StopJumping();
}

void AExhibitionPlayerController::RequestCrouch()
{
	ensure(CharacterRef != nullptr);

	CharacterRef->ToggleCrouch();
}

void AExhibitionPlayerController::RequestSprint()
{
	ensure(CharacterRef != nullptr);
	ensure(CharacterRef->GetExhibitionMovComponent());

	CharacterRef->GetExhibitionMovComponent()->ToggleSprint();
}

void AExhibitionPlayerController::RequestRoll()
{
	ensure(CharacterRef != nullptr);
	ensure(CharacterRef->GetExhibitionMovComponent());

	CharacterRef->GetExhibitionMovComponent()->RequestDive();
}

void AExhibitionPlayerController::RequestHook()
{
	ensure(CharacterRef != nullptr);
	ensure(CharacterRef->GetExhibitionMovComponent());

	CharacterRef->GetExhibitionMovComponent()->ToggleHook();
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/ExhibitionCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
AExhibitionCharacter::AExhibitionCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("Camera Boom"));
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SetupAttachment(GetRootComponent());

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera Component"));
	CameraComponent->SetupAttachment(CameraBoom);
	
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;

	JumpMaxHoldTime = 0.15f;
	GetCharacterMovement()->AirControl = 0.5f;
}

// Called when the game starts or when spawned
void AExhibitionCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void AExhibitionCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AExhibitionCharacter::ToggleCrouch()
{
}

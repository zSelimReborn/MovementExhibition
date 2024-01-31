// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/ExhibitionCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/ExhibitionMovementComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"

// Sets default values
AExhibitionCharacter::AExhibitionCharacter(const FObjectInitializer& Initializer) :
	Super(Initializer.SetDefaultSubobjectClass<UExhibitionMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	ExhibitionMovementComponent = Cast<UExhibitionMovementComponent>(GetCharacterMovement());
	ExhibitionMovementComponent->SetIsReplicated(true);

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
	ensure(ExhibitionMovementComponent);

	ExhibitionMovementComponent->ToggleCrouch();
}

void AExhibitionCharacter::Jump()
{
	if (ExhibitionMovementComponent->IsCrouching())
	{
		ExhibitionMovementComponent->ToggleCrouch();
	}
	else
	{
		Super::Jump();
	}
}

FCollisionQueryParams AExhibitionCharacter::GetIgnoreCollisionParams() const
{
	TArray<AActor*> ChildActors;
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	GetAllChildActors(ChildActors);
	Params.AddIgnoredActors(Children);
	
	return Params;
}

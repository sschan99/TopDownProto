// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"

ATopDownCharacter::ATopDownCharacter()
{
	// Set this character to call Tick() every frame
	PrimaryActorTick.bCanEverTick = true;

	// Enable replication
	bReplicates = true;
	SetReplicateMovement(true);

	// Configure capsule collision
	GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // Character moves in the direction of input...
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 640.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.0f;
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Set default turn rates
	BaseTurnRate = 45.0f;
	BaseLookUpRate = 45.0f;

	// Initialize character
	InitializeCharacter();
}

void ATopDownCharacter::InitializeCharacter()
{
	// Additional initialization can be done here
	// This is called from constructor to set up default values
}

void ATopDownCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate additional properties here as needed
	// Example:
	// DOREPLIFETIME(ATopDownCharacter, Health);
	// DOREPLIFETIME(ATopDownCharacter, MaxHealth);
}

void ATopDownCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Log character initialization
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("TopDownCharacter spawned on server: %s"), *GetName());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("TopDownCharacter spawned on client: %s"), *GetName());
	}
}

void ATopDownCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Per-frame updates can be done here
}

void ATopDownCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Input bindings will be set up in Task 4 (Enhanced Input System)
	// For now, this is just a placeholder
	check(PlayerInputComponent);
}

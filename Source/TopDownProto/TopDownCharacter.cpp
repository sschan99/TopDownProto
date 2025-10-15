// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownCharacter.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "WeaponComponent.h"
#include "TimerManager.h"
#include "Particles/ParticleSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

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

	// Create weapon component
	WeaponComponent = CreateDefaultSubobject<UWeaponComponent>(TEXT("WeaponComponent"));

	// Initialize Input Actions to nullptr (will be set in Blueprint or C++)
	DefaultMappingContext = nullptr;
	MoveAction = nullptr;
	LookAction = nullptr;
	FireAction = nullptr;
	ReloadAction = nullptr;

	// Initialize firing state
	bIsFirePressed = false;

	// Initialize effects (set in Blueprint)
	MuzzleFlash = nullptr;
	FireSound = nullptr;

	// Initialize health
	MaxHealth = 100.0f;
	Health = MaxHealth;
	bIsDead = false;

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

	// Replicate health properties
	DOREPLIFETIME(ATopDownCharacter, Health);
	DOREPLIFETIME(ATopDownCharacter, bIsDead);
}

void ATopDownCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

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

	// Update character rotation toward mouse cursor (client-side only)
	if (IsLocallyControlled() && TopDownCameraComponent)
	{
		UpdateRotationToMouseCursor(DeltaTime);
	}
}

void ATopDownCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Moving
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ATopDownCharacter::Move);
		}

		// Looking
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ATopDownCharacter::Look);
		}

		// Firing (automatic fire while held)
		if (FireAction)
		{
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ATopDownCharacter::OnFirePressed);
			EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ATopDownCharacter::OnFireReleased);
		}

		// Reloading
		if (ReloadAction)
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ATopDownCharacter::Reload);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("'%s' Failed to find an Enhanced Input Component!"), *GetNameSafe(this));
	}
}

void ATopDownCharacter::Move(const FInputActionValue& Value)
{
	// Input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr && TopDownCameraComponent != nullptr)
	{
		// For top-down, use camera's rotation for movement direction (screen-space movement)
		// This makes WASD move relative to the screen, not the character facing direction
		const FRotator CameraRotation = TopDownCameraComponent->GetComponentRotation();
		const FRotator YawRotation(0, CameraRotation.Yaw, 0);

		// Get forward and right vectors based on camera rotation
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		// Add movement (W=forward, S=backward, A=left, D=right relative to camera)
		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void ATopDownCharacter::Look(const FInputActionValue& Value)
{
	// Look input is handled in Tick() via UpdateRotationToMouseCursor()
	// This function is kept for Enhanced Input compatibility but not actively used
}

void ATopDownCharacter::OnFirePressed()
{
	bIsFirePressed = true;

	// Request fire immediately (server will check CanFire)
	if (WeaponComponent)
	{
		// Calculate fire direction (where character is facing) and normalize
		FVector FireDirection = GetActorRotation().Vector();
		FireDirection.Normalize(); // Ensure normalized for network transmission
		ServerRequestFire(FireDirection);
	}

	// Start automatic firing timer
	if (GetWorld())
	{
		// Calculate fire interval from WeaponComponent's fire rate
		float FireInterval = WeaponComponent ? (60.0f / FMath::Max(1.0f, WeaponComponent->FireRate)) : 0.1f;
		
		GetWorld()->GetTimerManager().SetTimer(
			AutoFireTimerHandle,
			this,
			&ATopDownCharacter::HandleAutoFire,
			FireInterval,
			true  // Loop
		);
	}
}

void ATopDownCharacter::OnFireReleased()
{
	bIsFirePressed = false;

	// Stop automatic firing
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AutoFireTimerHandle);
	}
}

void ATopDownCharacter::HandleAutoFire()
{
	// Only continue firing if button is still pressed
	if (!bIsFirePressed)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().ClearTimer(AutoFireTimerHandle);
		}
		return;
	}

	// Request fire (server will check CanFire and handle auto-reload)
	if (WeaponComponent)
	{
		// Calculate fire direction (where character is facing) and normalize
		FVector FireDirection = GetActorRotation().Vector();
		FireDirection.Normalize(); // Ensure normalized for network transmission
		ServerRequestFire(FireDirection);
	}
}

void ATopDownCharacter::Reload()
{
	// Client-side input - request reload from server
	if (WeaponComponent && WeaponComponent->CanReload())
	{
		ServerRequestReload();
	}
}

void ATopDownCharacter::ServerRequestFire_Implementation(FVector_NetQuantize10 FireDirection)
{
	// Server-side fire logic
	if (WeaponComponent && WeaponComponent->TryFire(FireDirection))
	{
		UE_LOG(LogTemp, Log, TEXT("Server: %s fired weapon in direction %s"), *GetName(), *FireDirection.ToString());
		
		// Calculate muzzle location for effects
		FRotator FireRotation = FireDirection.Rotation();
		FVector MuzzleLocation = GetActorLocation() + FireRotation.RotateVector(WeaponComponent->MuzzleOffset);
		
		// Play fire effects on all clients (including server)
		MulticastPlayFireEffects(MuzzleLocation, FireDirection);
	}
}

bool ATopDownCharacter::ServerRequestFire_Validate(FVector_NetQuantize10 FireDirection)
{
	// Basic validation - just check it's not zero
	// (Network quantization may affect precision, so we're lenient)
	return !FireDirection.IsNearlyZero(0.01f);
}

void ATopDownCharacter::ServerRequestReload_Implementation()
{
	// Server-side reload logic
	if (WeaponComponent && WeaponComponent->StartReload())
	{
		UE_LOG(LogTemp, Log, TEXT("Server: %s started reload"), *GetName());
		// TODO: Play reload animation/effects via multicast RPC
	}
}

bool ATopDownCharacter::ServerRequestReload_Validate()
{
	// Basic validation
	return true;
}

void ATopDownCharacter::MulticastPlayFireEffects_Implementation(FVector_NetQuantize MuzzleLocation, FVector_NetQuantize FireDirection)
{
	// Play muzzle flash particle effect
	if (MuzzleFlash)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			MuzzleFlash,
			MuzzleLocation,
			FireDirection.Rotation(),
			FVector(1.0f),
			true
		);
	}

	// Play fire sound
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSound,
			MuzzleLocation
		);
	}

	UE_LOG(LogTemp, Log, TEXT("Playing fire effects at %s"), *MuzzleLocation.ToString());
}

void ATopDownCharacter::UpdateRotationToMouseCursor(float DeltaTime)
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// Get viewport size
		int32 ViewportSizeX, ViewportSizeY;
		PC->GetViewportSize(ViewportSizeX, ViewportSizeY);

		// Get mouse position (viewport coordinates, 0-1 normalized)
		float MouseX, MouseY;
		if (PC->GetMousePosition(MouseX, MouseY))
		{
			// Convert to viewport coordinates (0 to ViewportSize)
			FVector2D ScreenPosition(MouseX, MouseY);

			// Deproject screen position to world
			FVector WorldLocation, WorldDirection;
			if (PC->DeprojectScreenPositionToWorld(ScreenPosition.X, ScreenPosition.Y, WorldLocation, WorldDirection))
			{
				// For top-down view, find intersection with character's horizontal plane
				FVector CharacterLocation = GetActorLocation();
				float PlaneZ = CharacterLocation.Z;

				// Calculate intersection point on the horizontal plane
				if (!FMath::IsNearlyZero(WorldDirection.Z, KINDA_SMALL_NUMBER))
				{
					float T = (PlaneZ - WorldLocation.Z) / WorldDirection.Z;
					FVector TargetLocation = WorldLocation + (WorldDirection * T);

					// Calculate direction from character to target (XY plane only)
					FVector Direction = TargetLocation - CharacterLocation;
					Direction.Z = 0.0f;

					if (!Direction.IsNearlyZero())
					{
						Direction.Normalize();

						// Calculate target rotation
						FRotator TargetRotation = Direction.Rotation();

						// Smoothly interpolate rotation
						FRotator CurrentRotation = GetActorRotation();
						FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, 10.0f);

						// Apply rotation (Yaw only for top-down)
						SetActorRotation(FRotator(0.0f, NewRotation.Yaw, 0.0f));
					}
				}
			}
		}
	}
}

// ========================================================================================
// Health System
// ========================================================================================

float ATopDownCharacter::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, 
                                     AController* EventInstigator, AActor* DamageCauser)
{
	// Only process damage on server
	if (!HasAuthority())
	{
		return 0.0f;
	}

	// Don't take damage if already dead
	if (bIsDead)
	{
		return 0.0f;
	}

	// Calculate actual damage taken
	const float ActualDamage = Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (ActualDamage > 0.0f)
	{
		// Reduce health
		Health = FMath::Max(0.0f, Health - ActualDamage);

		UE_LOG(LogTemp, Log, TEXT("%s took %.2f damage, health now: %.2f/%.2f"), 
		       *GetName(), ActualDamage, Health, MaxHealth);

		// Check if character died
		if (Health <= 0.0f && !bIsDead)
		{
			Die(EventInstigator);
		}
	}

	return ActualDamage;
}

void ATopDownCharacter::Die(AController* Killer)
{
	// Only execute on server
	if (!HasAuthority())
	{
		return;
	}

	// Mark as dead
	bIsDead = true;

	UE_LOG(LogTemp, Log, TEXT("%s has died"), *GetName());

	// Call multicast to handle death on all clients (including server)
	MulticastHandleDeath();

	// TODO: Respawn logic in future tasks
}

void ATopDownCharacter::MulticastHandleDeath_Implementation()
{
	UE_LOG(LogTemp, Log, TEXT("MulticastHandleDeath: %s"), *GetName());

	// Disable input (on owning client)
	if (IsLocallyControlled())
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			DisableInput(PC);
		}
	}

	// Disable collision
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Ragdoll physics on all clients
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(TEXT("Ragdoll"));

	// TODO: Play death animation/effects in future tasks
}

void ATopDownCharacter::OnRep_Health(float OldHealth)
{
	// Called on clients when health changes
	UE_LOG(LogTemp, Log, TEXT("Client: Health changed from %.2f to %.2f"), OldHealth, Health);

	// TODO: Update health UI in future tasks
	// TODO: Play damage effects in future tasks

	// Check if just died
	if (Health <= 0.0f && !bIsDead)
	{
		// Client-side death visuals
		UE_LOG(LogTemp, Log, TEXT("Client: Character died"));
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "WeaponComponent.h"
#include "Projectile.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

UWeaponComponent::UWeaponComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame
	PrimaryComponentTick.bCanEverTick = true;

	// Enable replication
	SetIsReplicatedByDefault(true);

	// Default weapon configuration
	FireRate = 600.0f;                    // 600 RPM (10 shots per second)
	MagazineSize = 30;                    // 30 rounds per magazine
	StartingReserveAmmo = 90;             // 90 reserve rounds (3 magazines)
	MaxReserveAmmo = 150;                 // Maximum 150 reserve rounds
	ReloadTime = 2.0f;                    // 2 seconds to reload
	bAutoReloadWhenEmpty = true;          // Auto-reload when trying to fire empty gun

	// Projectile configuration
	ProjectileClass = nullptr;            // Set in Blueprint
	MuzzleOffset = FVector(100.0f, 0.0f, 0.0f);  // Forward offset for spawn location

	// Initialize ammo state
	CurrentAmmo = MagazineSize;           // Start with full magazine
	ReserveAmmo = StartingReserveAmmo;    // Start with configured reserve ammo
	
	// Initialize weapon state
	WeaponState = EWeaponState::Idle;
	NextFireTime = 0.0f;
	ReloadCompleteTime = 0.0f;
}

void UWeaponComponent::BeginPlay()
{
	Super::BeginPlay();

	// Initialize ammo on server
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentAmmo = MagazineSize;
		ReserveAmmo = StartingReserveAmmo;
		WeaponState = EWeaponState::Idle;

		UE_LOG(LogTemp, Log, TEXT("WeaponComponent initialized on server: %d/%d ammo"), CurrentAmmo, ReserveAmmo);
	}
}

void UWeaponComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Handle reload completion on server
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (WeaponState == EWeaponState::Reloading && GetWorldTime() >= ReloadCompleteTime)
		{
			CompleteReload();
		}
	}
}

void UWeaponComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate ammo and state
	DOREPLIFETIME(UWeaponComponent, CurrentAmmo);
	DOREPLIFETIME(UWeaponComponent, ReserveAmmo);
	DOREPLIFETIME(UWeaponComponent, WeaponState);
	DOREPLIFETIME(UWeaponComponent, NextFireTime);
	DOREPLIFETIME(UWeaponComponent, ReloadCompleteTime);
}

// ========================================================================================
// Fire Rate System
// ========================================================================================

bool UWeaponComponent::CanFire() const
{
	// Can fire if:
	// 1. Has ammo
	// 2. Not currently reloading
	// 3. Fire cooldown has elapsed
	return HasAmmo() && 
	       WeaponState != EWeaponState::Reloading && 
	       GetWorldTime() >= NextFireTime;
}

bool UWeaponComponent::TryFire(const FVector& FireDirection)
{
	// This should only be called on server
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("TryFire called on client - should only be called on server"));
		return false;
	}

	// Check if we can fire
	if (!CanFire())
	{
		return false;
	}

	// Update weapon state to firing
	WeaponState = EWeaponState::Firing;

	// Spawn projectile if class is set
	if (ProjectileClass && GetWorld())
	{
		AActor* Owner = GetOwner();
		if (Owner)
		{
			// Calculate spawn location (muzzle position) using fire direction
			FRotator FireRotation = FireDirection.Rotation();
			FVector SpawnLocation = Owner->GetActorLocation() + FireRotation.RotateVector(MuzzleOffset);

			// Set spawn parameters
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = Owner;
			SpawnParams.Instigator = Cast<APawn>(Owner);
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

			// Spawn projectile
			AProjectile* Projectile = GetWorld()->SpawnActor<AProjectile>(
				ProjectileClass,
				SpawnLocation,
				FireRotation,
				SpawnParams
			);

			if (Projectile)
			{
				// Fire projectile in the specified direction
				Projectile->FireInDirection(FireDirection);
				
				UE_LOG(LogTemp, Log, TEXT("Spawned projectile at %s facing %s"), 
				       *SpawnLocation.ToString(), *FireDirection.ToString());
			}
		}
	}

	// Consume ammo (this may trigger auto-reload if magazine becomes empty)
	ConsumeAmmo();

	// Set next fire time (fire rate limiting)
	NextFireTime = GetWorldTime() + GetFireCooldown();

	// Return to idle state only if not reloading
	// (ConsumeAmmo may have started reload if magazine is now empty)
	if (WeaponState != EWeaponState::Reloading)
	{
		WeaponState = EWeaponState::Idle;
	}

	UE_LOG(LogTemp, Log, TEXT("Weapon fired! Ammo: %d/%d"), CurrentAmmo, ReserveAmmo);

	return true;
}

float UWeaponComponent::GetFireCooldownRemaining() const
{
	float TimeRemaining = NextFireTime - GetWorldTime();
	return FMath::Max(0.0f, TimeRemaining);
}

// ========================================================================================
// Ammo System
// ========================================================================================

bool UWeaponComponent::HasAmmo() const
{
	return CurrentAmmo > 0;
}

bool UWeaponComponent::HasReserveAmmo() const
{
	return ReserveAmmo > 0;
}

void UWeaponComponent::ConsumeAmmo()
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		CurrentAmmo = FMath::Max(0, CurrentAmmo - 1);
		
		// If we just emptied the magazine, trigger auto-reload immediately
		if (CurrentAmmo == 0 && bAutoReloadWhenEmpty && HasReserveAmmo())
		{
			UE_LOG(LogTemp, Log, TEXT("Magazine empty - triggering auto-reload"));
			StartReload();
		}
	}
}

// ========================================================================================
// Reload System
// ========================================================================================

bool UWeaponComponent::CanReload() const
{
	// Can reload if:
	// 1. Not already at full ammo
	// 2. Has reserve ammo
	// 3. Not already reloading
	return CurrentAmmo < MagazineSize && 
	       HasReserveAmmo() && 
	       WeaponState != EWeaponState::Reloading;
}

bool UWeaponComponent::StartReload()
{
	// Should only be called on server
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartReload called on client - should only be called on server"));
		return false;
	}

	// Check if we can reload
	if (!CanReload())
	{
		return false;
	}

	// Set weapon state to reloading
	WeaponState = EWeaponState::Reloading;
	
	// Set reload completion time
	ReloadCompleteTime = GetWorldTime() + ReloadTime;

	UE_LOG(LogTemp, Log, TEXT("Reload started. Will complete in %.2f seconds"), ReloadTime);

	return true;
}

void UWeaponComponent::CompleteReload()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("CompleteReload called on client - should only be called on server"));
		return;
	}

	// Calculate how much ammo we need to fill the magazine
	int32 AmmoNeeded = MagazineSize - CurrentAmmo;
	
	// Take from reserve (limited by what we have)
	int32 AmmoToReload = FMath::Min(AmmoNeeded, ReserveAmmo);
	
	// Transfer ammo
	CurrentAmmo += AmmoToReload;
	ReserveAmmo -= AmmoToReload;

	// Return to idle state
	WeaponState = EWeaponState::Idle;
	ReloadCompleteTime = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("Reload completed! Ammo: %d/%d"), CurrentAmmo, ReserveAmmo);
}

void UWeaponComponent::CancelReload()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	if (WeaponState == EWeaponState::Reloading)
	{
		WeaponState = EWeaponState::Idle;
		ReloadCompleteTime = 0.0f;
		UE_LOG(LogTemp, Log, TEXT("Reload cancelled"));
	}
}

// ========================================================================================
// Replication Callbacks
// ========================================================================================

void UWeaponComponent::OnRep_CurrentAmmo()
{
	// Called on clients when CurrentAmmo changes
	UE_LOG(LogTemp, Log, TEXT("Client: Current ammo updated to %d"), CurrentAmmo);
	
	// Here you could trigger UI updates, sound effects, etc.
}

void UWeaponComponent::OnRep_ReserveAmmo()
{
	// Called on clients when ReserveAmmo changes
	UE_LOG(LogTemp, Log, TEXT("Client: Reserve ammo updated to %d"), ReserveAmmo);
}

void UWeaponComponent::OnRep_WeaponState()
{
	// Called on clients when WeaponState changes
	UE_LOG(LogTemp, Log, TEXT("Client: Weapon state changed to %d"), (int32)WeaponState);
	
	// Here you could trigger animations, sound effects, etc. based on state
	switch (WeaponState)
	{
	case EWeaponState::Idle:
		// Return to idle animation
		break;
	case EWeaponState::Firing:
		// Play fire animation/effects (handled elsewhere usually)
		break;
	case EWeaponState::Reloading:
		// Play reload animation
		break;
	}
}

// ========================================================================================
// Internal Helper Functions
// ========================================================================================

float UWeaponComponent::GetFireCooldown() const
{
	// Convert RPM to seconds between shots
	// FireRate is in rounds per minute, we need seconds per round
	// 60 seconds / rounds per minute = seconds per round
	return 60.0f / FireRate;
}

float UWeaponComponent::GetWorldTime() const
{
	if (GetWorld())
	{
		// Use server time if we're the server, or approximate client time
		return GetWorld()->GetTimeSeconds();
	}
	return 0.0f;
}

void UWeaponComponent::HandleAutoReload()
{
	// Auto-reload when trying to fire with empty magazine
	if (HasReserveAmmo() && WeaponState != EWeaponState::Reloading)
	{
		StartReload();
	}
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownHUD.h"
#include "TopDownCharacter.h"
#include "WeaponComponent.h"

void UTopDownHUD::InitializeHUD(ATopDownCharacter* InOwnerCharacter)
{
	OwnerCharacter = InOwnerCharacter;

	if (OwnerCharacter)
	{
		UE_LOG(LogTemp, Log, TEXT("HUD initialized for character: %s"), *OwnerCharacter->GetName());
		
		// Initial update
		UpdateHUD();
	}
}

void UTopDownHUD::UpdateHUD()
{
	if (!OwnerCharacter)
	{
		return;
	}

	// Update health display
	float CurrentHealth = GetCurrentHealth();
	float MaxHealth = GetMaxHealth();
	float HealthPercent = GetHealthPercent();
	OnHealthChanged(CurrentHealth, MaxHealth, HealthPercent);

	// Update ammo display
	int32 CurrentAmmo = GetCurrentAmmo();
	int32 ReserveAmmo = GetReserveAmmo();
	int32 MagazineSize = GetMagazineSize();
	OnAmmoChanged(CurrentAmmo, ReserveAmmo, MagazineSize);
}

// ========================================================================================
// Helper Functions
// ========================================================================================

float UTopDownHUD::GetHealthPercent() const
{
	if (!OwnerCharacter)
	{
		return 0.0f;
	}

	float MaxHealth = OwnerCharacter->GetMaxHealth();
	if (MaxHealth <= 0.0f)
	{
		return 0.0f;
	}

	return OwnerCharacter->GetHealth() / MaxHealth;
}

float UTopDownHUD::GetCurrentHealth() const
{
	return OwnerCharacter ? OwnerCharacter->GetHealth() : 0.0f;
}

float UTopDownHUD::GetMaxHealth() const
{
	return OwnerCharacter ? OwnerCharacter->GetMaxHealth() : 0.0f;
}

int32 UTopDownHUD::GetCurrentAmmo() const
{
	if (OwnerCharacter && OwnerCharacter->GetWeaponComponent())
	{
		return OwnerCharacter->GetWeaponComponent()->GetCurrentAmmo();
	}
	return 0;
}

int32 UTopDownHUD::GetReserveAmmo() const
{
	if (OwnerCharacter && OwnerCharacter->GetWeaponComponent())
	{
		return OwnerCharacter->GetWeaponComponent()->GetReserveAmmo();
	}
	return 0;
}

int32 UTopDownHUD::GetMagazineSize() const
{
	if (OwnerCharacter && OwnerCharacter->GetWeaponComponent())
	{
		return OwnerCharacter->GetWeaponComponent()->GetMagazineSize();
	}
	return 0;
}

bool UTopDownHUD::IsCharacterDead() const
{
	return OwnerCharacter ? OwnerCharacter->IsDead() : false;
}


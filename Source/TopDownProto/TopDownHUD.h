// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TopDownHUD.generated.h"

class ATopDownCharacter;

/**
 * UTopDownHUD
 * 
 * Main HUD widget for displaying player information:
 * - Health bar/text
 * - Ammo counter (current/reserve)
 * - Crosshair
 * 
 * Designed to be subclassed in Blueprint for visual design.
 */
UCLASS()
class TOPDOWNPROTO_API UTopDownHUD : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Initialize HUD with owner character
	 * @param InOwnerCharacter - Character that owns this HUD
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeHUD(ATopDownCharacter* InOwnerCharacter);

	/**
	 * Update HUD display (called from C++ or Blueprint)
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void UpdateHUD();

protected:
	/** Cached reference to owner character */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	ATopDownCharacter* OwnerCharacter;

	// ========================================================================================
	// Blueprint-Implementable Events
	// ========================================================================================

	/**
	 * Called when health changes
	 * Implement in Blueprint to update health bar/text
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnHealthChanged(float CurrentHealth, float MaxHealth, float HealthPercent);

	/**
	 * Called when ammo changes
	 * Implement in Blueprint to update ammo display
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnAmmoChanged(int32 CurrentAmmo, int32 ReserveAmmo, int32 MagazineSize);

	// ========================================================================================
	// Helper Functions (Blueprint Callable)
	// ========================================================================================

	/**
	 * Get current health as percentage (0.0 to 1.0)
	 */
	UFUNCTION(BlueprintPure, Category = "HUD")
	float GetHealthPercent() const;

	/**
	 * Get current health value
	 */
	UFUNCTION(BlueprintPure, Category = "HUD")
	float GetCurrentHealth() const;

	/**
	 * Get max health value
	 */
	UFUNCTION(BlueprintPure, Category = "HUD")
	float GetMaxHealth() const;

	/**
	 * Get current ammo in magazine
	 */
	UFUNCTION(BlueprintPure, Category = "HUD")
	int32 GetCurrentAmmo() const;

	/**
	 * Get reserve ammo
	 */
	UFUNCTION(BlueprintPure, Category = "HUD")
	int32 GetReserveAmmo() const;

	/**
	 * Get magazine size
	 */
	UFUNCTION(BlueprintPure, Category = "HUD")
	int32 GetMagazineSize() const;

	/**
	 * Check if character is dead
	 */
	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsCharacterDead() const;
};


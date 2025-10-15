// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WeaponComponent.generated.h"

/**
 * EWeaponState
 * 
 * Defines the current state of the weapon for reload state machine
 */
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	Idle         UMETA(DisplayName = "Idle"),
	Firing       UMETA(DisplayName = "Firing"),
	Reloading    UMETA(DisplayName = "Reloading")
};

/**
 * UWeaponComponent
 * 
 * Actor component that handles weapon functionality including:
 * - Ammunition management (current, reserve, magazine size)
 * - Fire rate limiting with cooldown system
 * - Reload mechanics (manual and automatic)
 * - Network replication of ammo state
 * 
 * This component should be attached to the player character.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNPROTO_API UWeaponComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UWeaponComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	//~ Begin UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent Interface

	// ========================================================================================
	// Fire Rate System
	// ========================================================================================

	/**
	 * Check if weapon can currently fire
	 * @return True if weapon is ready to fire (has ammo, not on cooldown, not reloading)
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool CanFire() const;

	/**
	 * Called when player attempts to fire weapon
	 * Handles fire rate limiting and ammo consumption
	 * @return True if weapon was successfully fired
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool TryFire();

	/**
	 * Get current fire cooldown remaining time
	 * @return Seconds until weapon can fire again
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	float GetFireCooldownRemaining() const;

	// ========================================================================================
	// Ammo System
	// ========================================================================================

	/**
	 * Check if weapon has ammo in current magazine
	 * @return True if current ammo > 0
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	bool HasAmmo() const;

	/**
	 * Check if weapon has reserve ammo for reload
	 * @return True if reserve ammo > 0
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	bool HasReserveAmmo() const;

	/**
	 * Get current ammo in magazine
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	int32 GetCurrentAmmo() const { return CurrentAmmo; }

	/**
	 * Get reserve ammo available
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	int32 GetReserveAmmo() const { return ReserveAmmo; }

	/**
	 * Get magazine capacity
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Ammo")
	int32 GetMagazineSize() const { return MagazineSize; }

	// ========================================================================================
	// Reload System
	// ========================================================================================

	/**
	 * Check if weapon can be reloaded
	 * @return True if weapon needs reload and has reserve ammo
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
	bool CanReload() const;

	/**
	 * Start reload process
	 * @return True if reload was started successfully
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
	bool StartReload();

	/**
	 * Complete the reload process (called after reload time expires)
	 * Transfers ammo from reserve to current magazine
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
	void CompleteReload();

	/**
	 * Cancel ongoing reload
	 */
	UFUNCTION(BlueprintCallable, Category = "Weapon|Reload")
	void CancelReload();

	/**
	 * Check if weapon is currently reloading
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Reload")
	bool IsReloading() const { return WeaponState == EWeaponState::Reloading; }

	/**
	 * Get current weapon state
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon")
	EWeaponState GetWeaponState() const { return WeaponState; }

	// ========================================================================================
	// Configuration Properties
	// ========================================================================================

	/** Fire rate in rounds per minute (RPM) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Fire Rate", meta = (ClampMin = "1.0", ClampMax = "1200.0"))
	float FireRate;

	/** Magazine capacity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo", meta = (ClampMin = "1"))
	int32 MagazineSize;

	/** Starting reserve ammo */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo", meta = (ClampMin = "0"))
	int32 StartingReserveAmmo;

	/** Maximum reserve ammo that can be carried */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Ammo", meta = (ClampMin = "0"))
	int32 MaxReserveAmmo;

	/** Time in seconds to complete reload */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Reload", meta = (ClampMin = "0.1"))
	float ReloadTime;

	/** If true, weapon will automatically reload when empty and fire is attempted */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon|Reload")
	bool bAutoReloadWhenEmpty;

protected:
	// ========================================================================================
	// Replicated State
	// ========================================================================================

	/** Current ammo in magazine (replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentAmmo, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 CurrentAmmo;

	/** Reserve ammo available (replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_ReserveAmmo, BlueprintReadOnly, Category = "Weapon|Ammo")
	int32 ReserveAmmo;

	/** Current weapon state (replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, BlueprintReadOnly, Category = "Weapon")
	EWeaponState WeaponState;

	/** Server time when weapon can fire again */
	UPROPERTY(Replicated)
	float NextFireTime;

	/** Server time when reload will complete */
	UPROPERTY(Replicated)
	float ReloadCompleteTime;

	// ========================================================================================
	// Replication Callbacks
	// ========================================================================================

	UFUNCTION()
	void OnRep_CurrentAmmo();

	UFUNCTION()
	void OnRep_ReserveAmmo();

	UFUNCTION()
	void OnRep_WeaponState();

	// ========================================================================================
	// Internal Helper Functions
	// ========================================================================================

	/**
	 * Consume one ammo from current magazine
	 * Called on server only
	 */
	void ConsumeAmmo();

	/**
	 * Calculate time between shots based on fire rate
	 * @return Seconds between shots
	 */
	float GetFireCooldown() const;

	/**
	 * Get current world time (server time or approximated client time)
	 */
	float GetWorldTime() const;

	/**
	 * Handle automatic reload when attempting to fire with empty magazine
	 */
	void HandleAutoReload();
};

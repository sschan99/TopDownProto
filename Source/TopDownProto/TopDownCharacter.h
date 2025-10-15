// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "TopDownCharacter.generated.h"

class UWeaponComponent;

/**
 * ATopDownCharacter
 * 
 * Player character for top-down shooter with full network replication support.
 * Features top-down camera view and network-replicated properties.
 */
UCLASS(config=Game)
class TOPDOWNPROTO_API ATopDownCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ATopDownCharacter();

	//~ Begin AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	//~ End AActor Interface

	//~ Begin APawn Interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	//~ End APawn Interface

	//~ Begin AActor Interface
	virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent, 
	                         class AController* EventInstigator, AActor* DamageCauser) override;
	//~ End AActor Interface

	/** Returns TopDownCameraComponent subobject */
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	
	/** Returns CameraBoom subobject */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns WeaponComponent subobject */
	FORCEINLINE UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

	/** Get current health */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetHealth() const { return Health; }

	/** Get max health */
	UFUNCTION(BlueprintPure, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	/** Check if character is dead */
	UFUNCTION(BlueprintPure, Category = "Health")
	bool IsDead() const { return bIsDead; }

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);

	/** Called when fire input is pressed */
	void OnFirePressed();

	/** Called when fire input is released */
	void OnFireReleased();

	/** Called for reload input */
	void Reload();

	/** Server RPC - Request to fire weapon with direction */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestFire(FVector_NetQuantize10 FireDirection);
	void ServerRequestFire_Implementation(FVector_NetQuantize10 FireDirection);
	bool ServerRequestFire_Validate(FVector_NetQuantize10 FireDirection);

	/** Server RPC - Request to reload weapon */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestReload();
	void ServerRequestReload_Implementation();
	bool ServerRequestReload_Validate();

	/** Multicast RPC - Play fire effects on all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayFireEffects(FVector_NetQuantize MuzzleLocation, FVector_NetQuantize FireDirection);
	void MulticastPlayFireEffects_Implementation(FVector_NetQuantize MuzzleLocation, FVector_NetQuantize FireDirection);

	// ========================================================================================
	// Health System
	// ========================================================================================

	/** Called when character dies (server only) */
	virtual void Die(AController* Killer);

	/** Multicast RPC - Play death effects on all clients */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHandleDeath();
	void MulticastHandleDeath_Implementation();

	/** Called on clients when health changes */
	UFUNCTION()
	void OnRep_Health(float OldHealth);

protected:
	/** Top down camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* TopDownCameraComponent;

	/** Camera boom positioning the camera above the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Base turn rate, in deg/sec */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
	float BaseLookUpRate;

	/** Enhanced Input Mapping Context */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputMappingContext* DefaultMappingContext;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* FireAction;

	/** Reload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction;

	/** Weapon Component for handling ammo and firing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Weapon, meta = (AllowPrivateAccess = "true"))
	UWeaponComponent* WeaponComponent;

	/** Muzzle flash particle system */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class UParticleSystem* MuzzleFlash;

	/** Fire sound effect */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Effects", meta = (AllowPrivateAccess = "true"))
	class USoundBase* FireSound;

	// ========================================================================================
	// Health Properties
	// ========================================================================================

	/** Current health (replicated to clients) */
	UPROPERTY(ReplicatedUsing = OnRep_Health, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	float Health;

	/** Maximum health */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float MaxHealth;

	/** Is character dead? */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Health", meta = (AllowPrivateAccess = "true"))
	bool bIsDead;

private:
	/** Initialize character components and settings */
	void InitializeCharacter();

	/** Update character rotation to face mouse cursor position */
	void UpdateRotationToMouseCursor(float DeltaTime);

	/** Handle automatic firing while fire button is held */
	void HandleAutoFire();

	/** Timer handle for automatic firing */
	FTimerHandle AutoFireTimerHandle;

	/** Is fire button currently pressed? */
	bool bIsFirePressed;
};

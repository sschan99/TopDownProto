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

	/** Returns TopDownCameraComponent subobject */
	FORCEINLINE class UCameraComponent* GetTopDownCameraComponent() const { return TopDownCameraComponent; }
	
	/** Returns CameraBoom subobject */
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/** Returns WeaponComponent subobject */
	FORCEINLINE UWeaponComponent* GetWeaponComponent() const { return WeaponComponent; }

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

	/** Server RPC - Request to fire weapon */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestFire();
	void ServerRequestFire_Implementation();
	bool ServerRequestFire_Validate();

	/** Server RPC - Request to reload weapon */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestReload();
	void ServerRequestReload_Implementation();
	bool ServerRequestReload_Validate();

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

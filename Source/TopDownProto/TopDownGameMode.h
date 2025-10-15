// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "TopDownGameMode.generated.h"

/**
 * ATopDownGameMode
 * 
 * Server-authoritative game mode for the top-down shooter.
 * Handles player spawning, respawning, and game state management.
 */
UCLASS(minimalapi)
class ATopDownGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	ATopDownGameMode();

	//~ Begin AGameMode Interface
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
	//~ End AGameMode Interface

	/** Request a player respawn after death */
	UFUNCTION(BlueprintCallable, Category = "GameMode")
	void RequestRespawn(AController* Controller);

protected:
	/** Default respawn delay in seconds */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameMode|Respawn")
	float RespawnDelay;

	/** Timer handle for respawn */
	FTimerHandle RespawnTimerHandle;

	/** Handle actual respawn logic */
	void HandleRespawn(AController* Controller);

	/** Find a suitable spawn point for a player */
	AActor* FindPlayerStart(AController* Player);
};

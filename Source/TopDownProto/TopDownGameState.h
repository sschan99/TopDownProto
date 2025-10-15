// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "TopDownGameState.generated.h"

/**
 * ATopDownGameState
 * 
 * Replicated game state that tracks match information and player counts.
 * This class is replicated to all clients and contains authoritative game data.
 */
UCLASS()
class TOPDOWNPROTO_API ATopDownGameState : public AGameState
{
	GENERATED_BODY()

public:
	ATopDownGameState();

	//~ Begin AActor Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor Interface

	/** Get the current number of connected players */
	UFUNCTION(BlueprintPure, Category = "GameState")
	int32 GetPlayerCount() const { return PlayerCount; }

	/** Get match elapsed time in seconds */
	UFUNCTION(BlueprintPure, Category = "GameState")
	float GetMatchTime() const;

	/** Increment player count (server only) */
	void AddPlayer();

	/** Decrement player count (server only) */
	void RemovePlayer();

protected:
	/** Number of currently connected players (replicated) */
	UPROPERTY(ReplicatedUsing = OnRep_PlayerCount, BlueprintReadOnly, Category = "GameState")
	int32 PlayerCount;

	/** Called when PlayerCount is replicated to clients */
	UFUNCTION()
	virtual void OnRep_PlayerCount();

	/** Server timestamp when match started */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "GameState")
	float MatchStartTime;

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	//~ End AActor Interface
};

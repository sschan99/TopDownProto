// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownGameMode.h"
#include "TopDownGameState.h"
#include "TopDownCharacter.h"
#include "TopDownPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ATopDownGameMode::ATopDownGameMode()
{
	// Set default game state class
	GameStateClass = ATopDownGameState::StaticClass();

	// Set default pawn class to TopDownCharacter
	DefaultPawnClass = ATopDownCharacter::StaticClass();

	// Set default player controller class
	PlayerControllerClass = ATopDownPlayerController::StaticClass();

	// Set default respawn delay (3 seconds)
	RespawnDelay = 3.0f;

	// Enable replication
	bReplicates = true;
}

void ATopDownGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (NewPlayer)
	{
		UE_LOG(LogTemp, Log, TEXT("Player logged in: %s"), *NewPlayer->GetName());
		
		// Update player count in game state
		if (ATopDownGameState* GS = GetGameState<ATopDownGameState>())
		{
			GS->AddPlayer();
		}
	}
}

void ATopDownGameMode::Logout(AController* Exiting)
{
	if (Exiting)
	{
		UE_LOG(LogTemp, Log, TEXT("Player logged out: %s"), *Exiting->GetName());
		
		// Update player count in game state
		if (ATopDownGameState* GS = GetGameState<ATopDownGameState>())
		{
			GS->RemovePlayer();
		}
	}

	Super::Logout(Exiting);
}

AActor* ATopDownGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// Use custom player start finding logic
	AActor* PlayerStart = FindPlayerStart(Player);
	
	if (PlayerStart)
	{
		return PlayerStart;
	}

	// Fall back to default behavior
	return Super::ChoosePlayerStart_Implementation(Player);
}

void ATopDownGameMode::RequestRespawn(AController* Controller)
{
	if (!Controller)
	{
		UE_LOG(LogTemp, Warning, TEXT("RequestRespawn called with null controller"));
		return;
	}

	// Destroy current pawn if it exists
	if (APawn* OldPawn = Controller->GetPawn())
	{
		OldPawn->Destroy();
	}

	// Schedule respawn after delay
	if (RespawnDelay > 0.0f)
	{
		FTimerDelegate RespawnDelegate;
		RespawnDelegate.BindUObject(this, &ATopDownGameMode::HandleRespawn, Controller);
		
		GetWorldTimerManager().SetTimer(
			RespawnTimerHandle,
			RespawnDelegate,
			RespawnDelay,
			false
		);

		UE_LOG(LogTemp, Log, TEXT("Respawn scheduled for %s in %.1f seconds"), 
			*Controller->GetName(), RespawnDelay);
	}
	else
	{
		// Respawn immediately if delay is 0
		HandleRespawn(Controller);
	}
}

void ATopDownGameMode::HandleRespawn(AController* Controller)
{
	if (!Controller)
	{
		return;
	}

	// Find a spawn point
	AActor* SpawnPoint = FindPlayerStart(Controller);
	if (!SpawnPoint)
	{
		UE_LOG(LogTemp, Error, TEXT("No valid spawn point found for respawn"));
		return;
	}

	// Spawn new pawn
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	APawn* NewPawn = GetWorld()->SpawnActor<APawn>(
		DefaultPawnClass,
		SpawnPoint->GetActorLocation(),
		SpawnPoint->GetActorRotation(),
		SpawnParams
	);

	if (NewPawn)
	{
		// Possess the new pawn
		Controller->Possess(NewPawn);

		// Reset character state (health, ammo, etc.)
		if (ATopDownCharacter* Character = Cast<ATopDownCharacter>(NewPawn))
		{
			Character->ResetForRespawn();
		}

		UE_LOG(LogTemp, Log, TEXT("Player respawned: %s"), *Controller->GetName());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to spawn pawn for respawn"));
	}
}

AActor* ATopDownGameMode::FindPlayerStart(AController* Player)
{
	// Collect all player starts in the level
	TArray<AActor*> PlayerStarts;
	for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
	{
		PlayerStarts.Add(*It);
	}

	if (PlayerStarts.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("No PlayerStart actors found in level"));
		return nullptr;
	}

	// Simple strategy: find the player start farthest from other players
	AActor* BestStart = nullptr;
	float BestDistance = 0.0f;

	for (AActor* StartActor : PlayerStarts)
	{
		if (!StartActor)
		{
			continue;
		}

		// Calculate minimum distance to any other player
		float MinDistanceToPlayer = MAX_FLT;
		
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			APlayerController* PC = Iterator->Get();
			if (PC && PC->GetPawn() && PC != Player)
			{
				float Distance = FVector::Dist(
					StartActor->GetActorLocation(),
					PC->GetPawn()->GetActorLocation()
				);
				MinDistanceToPlayer = FMath::Min(MinDistanceToPlayer, Distance);
			}
		}

		// Choose the start with maximum minimum distance (farthest from nearest player)
		if (MinDistanceToPlayer > BestDistance)
		{
			BestDistance = MinDistanceToPlayer;
			BestStart = StartActor;
		}
	}

	// If no good start was found (e.g., no other players), just use the first one
	if (!BestStart && PlayerStarts.Num() > 0)
	{
		BestStart = PlayerStarts[0];
	}

	return BestStart;
}

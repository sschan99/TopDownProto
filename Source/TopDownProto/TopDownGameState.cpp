// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownGameState.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

ATopDownGameState::ATopDownGameState()
{
	// Enable replication
	bReplicates = true;
	bAlwaysRelevant = true;

	// Initialize values
	PlayerCount = 0;
	MatchStartTime = 0.0f;
}

void ATopDownGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate player count to all clients
	DOREPLIFETIME(ATopDownGameState, PlayerCount);
	
	// Replicate match start time to all clients
	DOREPLIFETIME(ATopDownGameState, MatchStartTime);
}

void ATopDownGameState::BeginPlay()
{
	Super::BeginPlay();

	// Record match start time on server
	if (HasAuthority())
	{
		MatchStartTime = GetWorld()->GetTimeSeconds();
		UE_LOG(LogTemp, Log, TEXT("Match started at time: %.2f"), MatchStartTime);
	}
}

float ATopDownGameState::GetMatchTime() const
{
	if (MatchStartTime <= 0.0f)
	{
		return 0.0f;
	}

	return GetWorld()->GetTimeSeconds() - MatchStartTime;
}

void ATopDownGameState::AddPlayer()
{
	// Only allow server to modify player count
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("AddPlayer called on client - ignoring"));
		return;
	}

	PlayerCount++;
	UE_LOG(LogTemp, Log, TEXT("Player added. Total players: %d"), PlayerCount);
}

void ATopDownGameState::RemovePlayer()
{
	// Only allow server to modify player count
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("RemovePlayer called on client - ignoring"));
		return;
	}

	PlayerCount = FMath::Max(0, PlayerCount - 1);
	UE_LOG(LogTemp, Log, TEXT("Player removed. Total players: %d"), PlayerCount);
}

void ATopDownGameState::OnRep_PlayerCount()
{
	// This is called on clients when PlayerCount is replicated
	UE_LOG(LogTemp, Log, TEXT("PlayerCount replicated to client: %d"), PlayerCount);
	
	// You can trigger UI updates or other client-side logic here
	// For example: UpdatePlayerCountUI();
}

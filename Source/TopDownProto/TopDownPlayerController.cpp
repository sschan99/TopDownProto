// Copyright Epic Games, Inc. All Rights Reserved.

#include "TopDownPlayerController.h"
#include "TopDownHUD.h"
#include "TopDownCharacter.h"
#include "Blueprint/UserWidget.h"

ATopDownPlayerController::ATopDownPlayerController()
{
	// Initialize HUD
	HUDWidgetClass = nullptr;
	HUDWidget = nullptr;
}

void ATopDownPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Create HUD once (persists across respawns)
	if (IsLocalController())
	{
		CreateHUD();
	}
}

void ATopDownPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Update HUD to point to new pawn (server only)
	UpdateHUDOwner();

	UE_LOG(LogTemp, Log, TEXT("Server: PlayerController possessed new pawn: %s"), 
	       InPawn ? *InPawn->GetName() : TEXT("None"));
}

void ATopDownPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	// Update HUD to point to new pawn (client only)
	UpdateHUDOwner();

	UE_LOG(LogTemp, Log, TEXT("Client: Pawn replicated: %s"), 
	       GetPawn() ? *GetPawn()->GetName() : TEXT("None"));
}

void ATopDownPlayerController::CreateHUD()
{
	if (HUDWidgetClass && !HUDWidget)
	{
		HUDWidget = CreateWidget<UTopDownHUD>(this, HUDWidgetClass);
		if (HUDWidget)
		{
			HUDWidget->AddToViewport();
			UpdateHUDOwner();
			UE_LOG(LogTemp, Log, TEXT("HUD created by PlayerController"));
		}
	}
}

void ATopDownPlayerController::UpdateHUDOwner()
{
	if (HUDWidget)
	{
		ATopDownCharacter* TopDownChar = Cast<ATopDownCharacter>(GetPawn());
		if (TopDownChar)
		{
			HUDWidget->InitializeHUD(TopDownChar);
			UE_LOG(LogTemp, Log, TEXT("HUD owner updated to: %s"), *TopDownChar->GetName());
		}
	}
}


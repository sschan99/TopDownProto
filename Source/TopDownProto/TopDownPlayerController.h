// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TopDownPlayerController.generated.h"

class UTopDownHUD;

/**
 * ATopDownPlayerController
 * 
 * Custom player controller for managing HUD and player input.
 * Persists across pawn respawns.
 */
UCLASS()
class TOPDOWNPROTO_API ATopDownPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATopDownPlayerController();

protected:
	//~ Begin APlayerController Interface
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	//~ End APlayerController Interface

public:
	// ========================================================================================
	// HUD Management
	// ========================================================================================

	/** HUD Widget class to create */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UTopDownHUD> HUDWidgetClass;

	/** Reference to HUD widget (persists across respawns) */
	UPROPERTY(BlueprintReadOnly, Category = "UI")
	UTopDownHUD* HUDWidget;

protected:

	/** Create HUD widget */
	void CreateHUD();

	/** Update HUD to point to current pawn */
	void UpdateHUDOwner();
};


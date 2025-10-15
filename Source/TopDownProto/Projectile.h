// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UStaticMeshComponent;

/**
 * AProjectile
 * 
 * Basic projectile actor with collision and movement.
 * Designed for networked multiplayer gameplay.
 * 
 * Features:
 * - Sphere collision component
 * - Projectile movement (straight line with gravity option)
 * - Network replication
 * - Automatic destruction after lifetime expires
 * - Hit event handling
 */
UCLASS()
class TOPDOWNPROTO_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AProjectile();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// ========================================================================================
	// Components
	// ========================================================================================

	/** Sphere collision component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USphereComponent* CollisionComponent;

	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	/** Visual mesh component (optional) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UStaticMeshComponent* MeshComponent;

	// ========================================================================================
	// Projectile Configuration
	// ========================================================================================

	/** Initial speed of the projectile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement", meta = (ClampMin = "100.0"))
	float InitialSpeed;

	/** Maximum speed of the projectile */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement", meta = (ClampMin = "100.0"))
	float MaxSpeed;

	/** Time in seconds before projectile is automatically destroyed */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Lifetime", meta = (ClampMin = "0.1"))
	float Lifetime;

	/** Whether projectile is affected by gravity */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Movement")
	bool bAffectedByGravity;

	/** Damage amount dealt on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Combat", meta = (ClampMin = "0.0"))
	float Damage;

	// ========================================================================================
	// Initialization
	// ========================================================================================

	/**
	 * Initialize projectile velocity
	 * Call this after spawning the projectile
	 * @param Direction - Direction to fire the projectile
	 */
	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void FireInDirection(const FVector& Direction);

protected:
	// ========================================================================================
	// Collision Handling
	// ========================================================================================

	/**
	 * Called when projectile hits something
	 * @param HitComp - Component that was hit
	 * @param OtherActor - Actor that was hit
	 * @param OtherComp - Other component that was hit
	 * @param NormalImpulse - Force of impact
	 * @param Hit - Hit result information
	 */
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, 
	           FVector NormalImpulse, const FHitResult& Hit);

	/**
	 * Handle projectile destruction
	 */
	virtual void OnProjectileDestroy();

	/**
	 * Multicast RPC - Play hit effects on all clients
	 * @param HitLocation - Location where projectile hit
	 * @param HitNormal - Normal vector of the hit surface
	 */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastPlayHitEffects(FVector_NetQuantize HitLocation, FVector_NetQuantize HitNormal);
	void MulticastPlayHitEffects_Implementation(FVector_NetQuantize HitLocation, FVector_NetQuantize HitNormal);

public:
	// ========================================================================================
	// Hit Effects Configuration
	// ========================================================================================

	/** Particle effect to play on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	class UParticleSystem* HitEffect;

	/** Sound to play on hit */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile|Effects")
	class USoundBase* HitSound;
};


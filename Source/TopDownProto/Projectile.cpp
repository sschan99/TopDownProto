// Copyright Epic Games, Inc. All Rights Reserved.

#include "Projectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AProjectile::AProjectile()
{
	// Set this actor to call Tick() every frame (we don't need tick for basic projectile)
	PrimaryActorTick.bCanEverTick = false;

	// Enable replication
	bReplicates = true;
	SetReplicateMovement(true);

	// Create sphere collision component
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComponent"));
	CollisionComponent->InitSphereRadius(5.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	RootComponent = CollisionComponent;

	// Create mesh component (visual representation)
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetRelativeScale3D(FVector(0.1f, 0.1f, 0.1f));

	// Create projectile movement component
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->UpdatedComponent = CollisionComponent;
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.0f; // No gravity by default

	// Set default values
	InitialSpeed = 3000.0f;
	MaxSpeed = 3000.0f;
	Lifetime = 3.0f;
	bAffectedByGravity = false;
	Damage = 10.0f;

	// Initialize effects (set in Blueprint)
	HitEffect = nullptr;
	HitSound = nullptr;

	// Set initial lifespan
	InitialLifeSpan = Lifetime;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();

	// Bind collision event
	if (CollisionComponent)
	{
		CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectile::OnHit);
	}

	// Apply configured settings to movement component
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = InitialSpeed;
		ProjectileMovement->MaxSpeed = MaxSpeed;
		ProjectileMovement->ProjectileGravityScale = bAffectedByGravity ? 1.0f : 0.0f;
	}

	// Set lifespan
	SetLifeSpan(Lifetime);

	UE_LOG(LogTemp, Log, TEXT("Projectile spawned: %s"), *GetName());
}

void AProjectile::FireInDirection(const FVector& Direction)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction * InitialSpeed;
		UE_LOG(LogTemp, Log, TEXT("Projectile fired in direction: %s at speed: %.2f"), 
		       *Direction.ToString(), InitialSpeed);
	}
}

void AProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                        FVector NormalImpulse, const FHitResult& Hit)
{
	// Only execute on server
	if (!HasAuthority())
	{
		return;
	}

	// Don't hit ourselves or our instigator
	if (OtherActor && OtherActor != this && OtherActor != GetInstigator())
	{
		UE_LOG(LogTemp, Log, TEXT("Projectile hit: %s at location %s"), 
		       *OtherActor->GetName(), *Hit.ImpactPoint.ToString());

		// Play hit effects on all clients
		MulticastPlayHitEffects(Hit.ImpactPoint, Hit.ImpactNormal);

		// Apply damage to hit actor
		if (Damage > 0.0f)
		{
			UGameplayStatics::ApplyDamage(
				OtherActor,
				Damage,
				GetInstigatorController(),
				this,
				UDamageType::StaticClass()
			);

			UE_LOG(LogTemp, Log, TEXT("Applied %.2f damage to %s"), Damage, *OtherActor->GetName());
		}

		// Handle destruction
		OnProjectileDestroy();
	}
}

void AProjectile::OnProjectileDestroy()
{
	// Server handles destruction
	if (HasAuthority())
	{
		UE_LOG(LogTemp, Log, TEXT("Destroying projectile: %s"), *GetName());
		Destroy();
	}
}

void AProjectile::MulticastPlayHitEffects_Implementation(FVector_NetQuantize HitLocation, FVector_NetQuantize HitNormal)
{
	// Play hit particle effect
	if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HitEffect,
			HitLocation,
			HitNormal.Rotation(),
			FVector(1.0f),
			true
		);
	}

	// Play hit sound
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			HitLocation
		);
	}

	UE_LOG(LogTemp, Log, TEXT("Playing hit effects at %s"), *HitLocation.ToString());
}


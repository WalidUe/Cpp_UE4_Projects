// Fill out your copyright notice in the Description page of Project Settings.

//we need to include it since we'll be using its methods (as oppose to forward declarations)
#include "TankAimingComponent.h"//#include "xxxxx.h" Must be first (xxxxx is the name of the header file for this cpp file)
#include "TankBarrel.h"
#include "TankTurret.h"
#include "Projectile.h"
// Sets default values for this component's properties
UTankAimingComponent::UTankAimingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}
void UTankAimingComponent::BeginPlay()
{
	// so that first fire is after initial reload
	LastFireTime = GetWorld()->GetTimeSeconds();
}

void UTankAimingComponent::Initialise(UTankBarrel* BarrelToSet, UTankTurret* TurretToSet)
{
	Barrel = BarrelToSet;
	Turret = TurretToSet;
}

void UTankAimingComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	if (RoundsLeft <= 0)
	{
		FiringState = EFiringState::OutOfAmmo;
	}

	else if ((GetWorld()->GetTimeSeconds() - LastFireTime) < ReloadTimeInSeconds)
	{
		FiringState = EFiringState::Reloading;
	}
	else if (IsBarrelMoving())
	{
		FiringState = EFiringState::Aiming;
	}
	else
	{
		FiringState = EFiringState::Locked;
	}
}

bool UTankAimingComponent::IsBarrelMoving()
{
	if (!ensure(Barrel)) { return false; }
	auto BarrelForward = Barrel->GetForwardVector();
	return !BarrelForward.Equals(AimDirection, 0.01);
}


void UTankAimingComponent::AimAt(FVector HitLocation)
{
	if (!ensure(Barrel)) { return; }

	FVector OutLaunchVelocity;
	FVector StartLocation = Barrel->GetSocketLocation(FName("Projectile"));

	//Calculate the launch velocity
	bool bHaveAimSolution = UGameplayStatics::SuggestProjectileVelocity(
		this,
		OutLaunchVelocity,
		StartLocation,
		HitLocation,
		LaunchSpeed,
		false,
		0,
		0,
		ESuggestProjVelocityTraceOption::DoNotTrace
	);
		//UGameplayStatics:: because it's a static method!
	if(bHaveAimSolution)
	{
		auto TankName = GetOwner()->GetName();
		AimDirection = OutLaunchVelocity.GetSafeNormal();//NB:when we normalize this vector we make the magnitude of vector to be 1 and keeps the direction of vector.
		//"Normalize" in vectors means to change the vector so that it points in the same direction(think of that line from the origin) but its length is one.
		
		MoveBarrelTowards(AimDirection);

	}
}

EFiringState UTankAimingComponent::GetFiringState() const
{
	return FiringState;
}

int32 UTankAimingComponent::GetRoundsLeft() const
{
	return RoundsLeft;
}

void UTankAimingComponent::MoveBarrelTowards(FVector NewAimDirection)
{
	if (!ensure (Barrel) || !ensure(Turret)) { return; }

	//Work-out difference between current barrel rotation, and AimDirection
	auto BarrelRotator = Barrel->GetForwardVector().Rotation();
	auto AimAsRotator = NewAimDirection.Rotation();
	
	auto DeltaRotator = AimAsRotator - BarrelRotator;
	
	Barrel->Elevate(DeltaRotator.Pitch);
	//shortest rotation to aim towards
	if (FMath::Abs(DeltaRotator.Yaw) < 180)
	{
		Turret->Rotate(DeltaRotator.Yaw);
	}
	else//avoid going the long way
	{
		Turret->Rotate(-DeltaRotator.Yaw);
	}
	

}

void UTankAimingComponent::Fire()
{
	

	if (FiringState == EFiringState::Locked || FiringState == EFiringState::Aiming)
	{
		

		//Spawn a projectile at the socket location on the barrel
		if (!ensure(Barrel)) { return; }
		if (!ensure(ProjectileBlueprint)) { return; }

		auto Projectile = GetWorld()->SpawnActor<AProjectile>(
			ProjectileBlueprint,
			Barrel->GetSocketLocation(FName("Projectile")),
			Barrel->GetSocketRotation(FName("Projectile"))
			);

	

		Projectile->LaunchProjectile(LaunchSpeed);
		LastFireTime = GetWorld()->GetTimeSeconds();
		RoundsLeft--;
	}
}

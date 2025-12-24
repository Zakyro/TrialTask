#include "CustomMovementComponent.h"

#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"

UCustomMovementComponent::UCustomMovementComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UCustomMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	DefaultGroundFriction = GroundFriction;
	DefaultBrakingDecel = BrakingDecelerationWalking;

	Stamina = StaminaMax;
	UpdateMaxSpeed();
}

void UCustomMovementComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Keeps track of how recently sprint ended (used for slide grace window).
	if (!bIsSprinting)
	{
		TimeSinceSprintEnded += DeltaTime;
	}

	UpdateStamina(DeltaTime);
	UpdateMaxSpeed();
}

float UCustomMovementComponent::GetHorizontalSpeed() const
{
	const FVector V = Velocity;
	return FVector(V.X, V.Y, 0.f).Size();
}

// --------------------
// STAMINA + SPRINT
// --------------------

void UCustomMovementComponent::SetSprintRequested(bool bRequested)
{
	bSprintRequested = bRequested;
}

void UCustomMovementComponent::UpdateStamina(float DeltaTime)
{
	if (DeltaTime <= 0.f) return;

	const bool bDrainSprint = bIsSprinting && IsMovingOnGround() && !bIsSliding;
	const bool bDrainSlide = bIsSliding;

	if (bDrainSprint)
	{
		Stamina = FMath::Max(0.f, Stamina - SprintDrainPerSec * DeltaTime);
	}
	else if (bDrainSlide)
	{
		Stamina = FMath::Max(0.f, Stamina - SlideDrainPerSec * DeltaTime);
	}
	else
	{
		Stamina = FMath::Min(StaminaMax, Stamina + StaminaRegenPerSec * DeltaTime);
	}

	// Stops sprint if stamina hits zero.
	if (bIsSprinting && Stamina < KINDA_SMALL_NUMBER)
	{
		bIsSprinting = false;
		TimeSinceSprintEnded = 0.f;
	}
}

void UCustomMovementComponent::UpdateMaxSpeed()
{
	const bool bShouldSprint =
		bSprintRequested &&
		!bIsSliding &&
		IsMovingOnGround() &&
		(Stamina >= MinStaminaToSprint);

	if (bShouldSprint)
	{
		bIsSprinting = true;
	}
	else
	{
		if (bIsSprinting)
		{
			bIsSprinting = false;
			TimeSinceSprintEnded = 0.f; // opens grace window for slide
		}
	}

	MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

// --------------------
// SLIDE
// --------------------

bool UCustomMovementComponent::CanStartSlide() const
{
	if (!CharacterOwner) return false;
	if (bIsSliding) return false;
	if (!IsMovingOnGround()) return false;

	if (Stamina < MinStaminaToSlide) return false;

	const float Speed = GetHorizontalSpeed();

	// Either enough speed OR within sprint grace window.
	const bool bHasSpeed = Speed >= SlideMinStartSpeed;
	const bool bInGrace = (TimeSinceSprintEnded <= PostSprintSlideGraceTime) && (Speed >= (SlideMinStartSpeed * 0.85f));

	return bHasSpeed || bInGrace;
}

void UCustomMovementComponent::StartSlide()
{
	if (!CanStartSlide()) return;
	EnterSlide();
}

void UCustomMovementComponent::StopSlide()
{
	if (!bIsSliding) return;
	ExitSlide();
}

void UCustomMovementComponent::EnterSlide()
{
	bIsSliding = true;

	// Saves current walking params, then makes slide feel slippery.
	DefaultGroundFriction = GroundFriction;
	DefaultBrakingDecel = BrakingDecelerationWalking;

	GroundFriction = SlideGroundFriction;
	BrakingDecelerationWalking = 0.f;

	SetMovementMode(MOVE_Custom, (uint8)ECustomMovementMode::CMOVE_Slide);
}

void UCustomMovementComponent::ExitSlide()
{
	bIsSliding = false;

	// Restores walking params.
	GroundFriction = DefaultGroundFriction;
	BrakingDecelerationWalking = DefaultBrakingDecel;

	SetMovementMode(MOVE_Walking);
}

// --------------------
// PHYS
// --------------------

void UCustomMovementComponent::PhysWalking(float DeltaTime, int32 Iterations)
{
	Super::PhysWalking(DeltaTime, Iterations);
}

void UCustomMovementComponent::PhysCustom(float DeltaTime, int32 Iterations)
{
	if (CustomMovementMode == (uint8)ECustomMovementMode::CMOVE_Slide)
	{
		PhysSlide(DeltaTime, Iterations);
		return;
	}

	Super::PhysCustom(DeltaTime, Iterations);
}

void UCustomMovementComponent::PhysSlide(float DeltaTime, int32 Iterations)
{
	if (!CharacterOwner || DeltaTime < KINDA_SMALL_NUMBER)
	{
		return;
	}

	// Updates floor info.
	FindFloor(UpdatedComponent->GetComponentLocation(), CurrentFloor, false);

	if (!CurrentFloor.IsWalkableFloor())
	{
		// If ground is lost, ends slide and falls.
		ExitSlide();
		SetMovementMode(MOVE_Falling);
		return;
	}

	const FVector FloorNormal = CurrentFloor.HitResult.ImpactNormal.GetSafeNormal();
	const FVector Up = FVector::UpVector;

	// Computes slope angle (0 = flat, 90 = vertical wall).
	const float CosSlope = FVector::DotProduct(FloorNormal, Up);
	const float SlopeRad = FMath::Acos(FMath::Clamp(CosSlope, -1.f, 1.f));
	const float SlopeAngleDeg = FMath::RadiansToDegrees(SlopeRad);

	const bool bIsOnSlope = (SlopeAngleDeg >= SlideSlopeAngleMinDeg);

	// Downhill direction = gravity projected onto the floor plane.
	const FVector GravityDir = FVector(0.f, 0.f, -1.f);
	FVector Downhill = GravityDir - FVector::DotProduct(GravityDir, FloorNormal) * FloorNormal;
	Downhill = Downhill.GetSafeNormal();

	// Keeps velocity on the floor plane to avoid tiny Z jitter.
	Velocity = FVector::VectorPlaneProject(Velocity, FloorNormal);

	const float Speed = Velocity.Size();
	if (Speed < SlideMinSpeedToKeep)
	{
		ExitSlide();
		return;
	}

	const FVector VelDir = Velocity.GetSafeNormal();
	const float AlongDownhill = FVector::DotProduct(VelDir, Downhill); // >0 = going downhill, <0 = going uphill

	// Steering input: uses the movement input vector, projected on the floor.
	FVector Input = ConsumeInputVector();
	Input = FVector::VectorPlaneProject(Input, FloorNormal);
	const FVector InputDir = Input.GetSafeNormal();

	FVector Accel = FVector::ZeroVector;

	// Steering always works (arcade feel).
	if (!InputDir.IsNearlyZero())
	{
		Accel += InputDir * SlideSteerAccel;
	}

	if (bIsOnSlope && !Downhill.IsNearlyZero())
	{
		// Uses slope strength (0..1) so small slopes accelerate less.
		const float SlopeStrength = FMath::Clamp(FMath::Sin(SlopeRad), 0.f, 1.f);

		// Always pulls toward downhill on a slope (fixes "always flat" issue).
		Accel += Downhill * (SlideDownhillAccel * SlopeStrength);

		// If moving against downhill, adds extra braking (uphill feels heavy).
		if (AlongDownhill < -0.05f)
		{
			Accel += (-VelDir) * SlideUphillDecel;
		}
	}
	else
	{
		// Flat: loses momentum over time.
		Accel += (-VelDir) * SlideFlatDecel;
	}

	// Integrates velocity.
	Velocity += Accel * DeltaTime;
	Velocity = FVector::VectorPlaneProject(Velocity, FloorNormal);

	// Caps speed depending on slope/flat.
	const float MaxSpeed = bIsOnSlope ? SlideMaxSpeedDownhill : SlideMaxSpeedFlat;
	const float NewSpeed = Velocity.Size();
	if (NewSpeed > MaxSpeed)
	{
		Velocity = Velocity.GetSafeNormal() * MaxSpeed;
	}

	if (Velocity.Size() < SlideMinSpeedToKeep)
	{
		ExitSlide();
		return;
	}

	// Moves capsule.
	const FVector Delta = Velocity * DeltaTime;

	FHitResult Hit;
	SafeMoveUpdatedComponent(Delta, UpdatedComponent->GetComponentQuat(), true, Hit);

	if (Hit.IsValidBlockingHit())
	{
		// Slides along blocking surface.
		SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);

		// Keeps velocity on the new surface plane.
		Velocity = FVector::VectorPlaneProject(Velocity, Hit.Normal);
	}
}

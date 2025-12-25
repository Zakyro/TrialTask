#include "TrialAnimInstance.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.h"
#include "CustomCharacter.h"

#include "KismetAnimationLibrary.h"

void UTrialAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
	CachedMoveComp = CachedCharacter ? Cast<UCustomMovementComponent>(CachedCharacter->GetCharacterMovement()) : nullptr;
}

void UTrialAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (!CachedCharacter)
	{
		CachedCharacter = Cast<ACharacter>(TryGetPawnOwner());
	}
	if (CachedCharacter && !CachedMoveComp)
	{
		CachedMoveComp = Cast<UCustomMovementComponent>(CachedCharacter->GetCharacterMovement());
	}

	if (!CachedCharacter || !CachedMoveComp)
	{
		Speed = 0.0f;
		VerticalVelocity = 0.0f;
		bIsInAir = false;
		bIsSliding = false;
		bIsCrouched = false;
		Direction = 0.0f;
		bJumpRequested = false;
		bIsParkouring = false;
		bIsVaulting = false;
		bIsMantling = false;
		return;
	}

	const FVector Vel = CachedCharacter->GetVelocity();
	VerticalVelocity = Vel.Z;

	bIsInAir = !CachedMoveComp->IsMovingOnGround();
	bIsCrouched = CachedCharacter->bIsCrouched;
	bIsSliding = CachedMoveComp->IsSliding();

	const float HorizontalSpeed = CachedMoveComp->GetHorizontalSpeed();
	const float Walk = FMath::Max(CachedMoveComp->GetWalkSpeed(), 1.0f);
	const float Sprint = FMath::Max(CachedMoveComp->GetSprintSpeed(), Walk + 1.0f);

	if (HorizontalSpeed <= Walk)
	{
		Speed = HorizontalSpeed / Walk;
	}
	else
	{
		const float SprintRange = Sprint - Walk;
		const float Alpha = FMath::Clamp((HorizontalSpeed - Walk) / SprintRange, 0.0f, 1.0f);
		Speed = 1.0f + Alpha;
	}

	Direction = UKismetAnimationLibrary::CalculateDirection(Vel, CachedCharacter->GetActorRotation());
	Direction = FMath::Clamp(Direction, -DirectionClampAbs, DirectionClampAbs);

	if (const ACustomCharacter* CC = Cast<ACustomCharacter>(CachedCharacter))
	{
		bJumpRequested = CC->bJumpRequested;
		bIsParkouring = CC->IsParkouring();
		bIsVaulting = CC->IsVaulting();
		bIsMantling = CC->IsMantling();
	}
	else
	{
		bJumpRequested = false;
		bIsParkouring = false;
		bIsVaulting = false;
		bIsMantling = false;
	}
}


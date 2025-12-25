#include "CustomCharacter.h"

#include "CustomMovementComponent.h"
#include "StaminaWidget.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"

static const FName TAG_PARKOURABLE(TEXT("Parkourable"));

ACustomCharacter::ACustomCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UCustomMovementComponent>(ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	CustomMoveComp = Cast<UCustomMovementComponent>(GetCharacterMovement());

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, 64.f));
	FirstPersonCamera->bUsePawnControlRotation = true;

	bUseControllerRotationYaw = true;
	bUseControllerRotationPitch = true;
	bUseControllerRotationRoll = false;

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
}

void ACustomCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enhanced Input mapping context
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					Subsystem->ClearAllMappings();
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}

	// Stamina UI
	if (StaminaWidgetClass && !StaminaWidget)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			StaminaWidget = CreateWidget<UStaminaWidget>(PC, StaminaWidgetClass);
			if (StaminaWidget)
			{
				StaminaWidget->AddToViewport();
				if (!CustomMoveComp)
				{
					CustomMoveComp = Cast<UCustomMovementComponent>(GetCharacterMovement());
				}
				StaminaWidget->SetMovementComponent(CustomMoveComp);
			}
		}
	}
}

void ACustomCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!bIsParkouring || ParkourPhase == EParkourPhase::None)
	{
		return;
	}

	// Phase movement
	if (ParkourPhase == EParkourPhase::ToApex)
	{
		if (ParkourMoveStep(DeltaSeconds, ParkourStart, ParkourApex, PhaseDuration))
		{
			AdvanceParkourPhase();
		}
	}
	else if (ParkourPhase == EParkourPhase::ToTarget)
	{
		if (ParkourMoveStep(DeltaSeconds, ParkourApex, ParkourTarget, PhaseDuration))
		{
			EndParkour(false, true);
		}
	}
}

void ACustomCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC) return;

	if (IA_MoveForward)
	{
		EIC->BindAction(IA_MoveForward, ETriggerEvent::Triggered, this, &ACustomCharacter::MoveForward);
		EIC->BindAction(IA_MoveForward, ETriggerEvent::Completed, this, &ACustomCharacter::MoveForward);
	}

	if (IA_MoveRight)
	{
		EIC->BindAction(IA_MoveRight, ETriggerEvent::Triggered, this, &ACustomCharacter::MoveRight);
		EIC->BindAction(IA_MoveRight, ETriggerEvent::Completed, this, &ACustomCharacter::MoveRight);
	}

	if (IA_Look)
	{
		EIC->BindAction(IA_Look, ETriggerEvent::Triggered, this, &ACustomCharacter::Look);
	}

	if (IA_Jump)
	{
		EIC->BindAction(IA_Jump, ETriggerEvent::Started, this, &ACharacter::Jump);
		EIC->BindAction(IA_Jump, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	}

	if (IA_Sprint)
	{
		EIC->BindAction(IA_Sprint, ETriggerEvent::Started, this, &ACustomCharacter::SprintPressed);
		EIC->BindAction(IA_Sprint, ETriggerEvent::Completed, this, &ACustomCharacter::SprintReleased);
	}

	if (IA_Crouch)
	{
		EIC->BindAction(IA_Crouch, ETriggerEvent::Started, this, &ACustomCharacter::CrouchPressed);
		EIC->BindAction(IA_Crouch, ETriggerEvent::Completed, this, &ACustomCharacter::CrouchReleased);
	}

	if (IA_Parkour)
	{
		EIC->BindAction(IA_Parkour, ETriggerEvent::Started, this, &ACustomCharacter::ParkourPressed);
	}
}

// --------------------
// INPUT
// --------------------

void ACustomCharacter::SetInputLocked(bool bLocked)
{
	if (bLocked)
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			SavedGravityScale = Move->GravityScale;
			Move->GravityScale = 0.0f;

			Move->SetMovementMode(MOVE_Flying);
		}
	}
	else
	{
		if (UCharacterMovementComponent* Move = GetCharacterMovement())
		{
			Move->GravityScale = SavedGravityScale;
			Move->SetMovementMode(MOVE_Walking);
		}
	}

}

void ACustomCharacter::MoveForward(const FInputActionValue& Value)
{
	if (bInputLocked || bIsParkouring) return;

	const float Axis = Value.Get<float>();
	if (!Controller || FMath::IsNearlyZero(Axis)) return;

	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector ForwardDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDir, Axis);
}

void ACustomCharacter::MoveRight(const FInputActionValue& Value)
{
	if (bInputLocked || bIsParkouring) return;

	const float Axis = Value.Get<float>();
	if (!Controller || FMath::IsNearlyZero(Axis)) return;

	const FRotator YawRot(0.f, Controller->GetControlRotation().Yaw, 0.f);
	const FVector RightDir = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDir, Axis);
}

void ACustomCharacter::Look(const FInputActionValue& Value)
{
	if (!Controller) return;

	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y * -1.0f);
}

void ACustomCharacter::SprintPressed(const FInputActionValue& Value)
{
	if (bInputLocked || bIsParkouring) return;
	if (CustomMoveComp) CustomMoveComp->SetSprintRequested(true);
}

void ACustomCharacter::SprintReleased(const FInputActionValue& Value)
{
	if (CustomMoveComp) CustomMoveComp->SetSprintRequested(false);
}

void ACustomCharacter::CrouchPressed(const FInputActionValue& Value)
{
	if (bInputLocked || bIsParkouring) return;

	if (CustomMoveComp && CustomMoveComp->CanStartSlide())
	{
		CustomMoveComp->StartSlide();
		Crouch();
		return;
	}

	Crouch();
	if (CustomMoveComp) CustomMoveComp->SetCrouchRequested(true);
}

void ACustomCharacter::CrouchReleased(const FInputActionValue& Value)
{
	if (CustomMoveComp && CustomMoveComp->IsSliding())
	{
		CustomMoveComp->StopSlide();
	}

	UnCrouch();
	if (CustomMoveComp) CustomMoveComp->SetCrouchRequested(false);
}

// --------------------
// PARKOUR (detection + geometric move)
// --------------------

void ACustomCharacter::ParkourPressed(const FInputActionValue& Value)
{
	if (bInputLocked || bIsParkouring) return;

	FHitResult FrontHit, TopHit;
	float ObstacleHeight = 0.f;
	FVector TopPoint = FVector::ZeroVector;

	if (!FindParkourObstacle(FrontHit, TopHit, ObstacleHeight, TopPoint))
	{
		UE_LOG(LogTemp, Warning, TEXT("PARKOUR: no valid obstacle"));
		return;
	}

	const EParkourType Type = DecideParkourType(ObstacleHeight);
	if (Type == EParkourType::None)
	{
		UE_LOG(LogTemp, Warning, TEXT("PARKOUR: obstacle out of range (height=%.1f)"), ObstacleHeight);
		return;
	}

	FVector SafeTarget = FVector::ZeroVector;
	if (!ComputeSafeParkourLanding(FrontHit, TopPoint, SafeTarget))
	{
		UE_LOG(LogTemp, Warning, TEXT("PARKOUR: landing blocked"));
		return;
	}

	StartParkour(Type, SafeTarget, TopPoint);
}

EParkourType ACustomCharacter::DecideParkourType(float ObstacleHeight) const
{
	if (ObstacleHeight <= VaultMaxObstacleHeight) return EParkourType::Vault;
	if (ObstacleHeight <= MantleMaxObstacleHeight) return EParkourType::Mantle;
	return EParkourType::None;
}

bool ACustomCharacter::FindParkourObstacle(FHitResult& OutFrontHit, FHitResult& OutTopHit, float& OutObstacleHeight, FVector& OutTopPoint) const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	const FVector Start = GetActorLocation() + FVector(0, 0, 50);
	const FVector Forward = GetActorForwardVector();
	const FVector End = Start + Forward * ParkourFrontCheckDistance;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ParkourFront), false, this);

	const bool bFrontHit = World->SweepSingleByChannel(
		OutFrontHit,
		Start,
		End,
		FQuat::Identity,
		ECC_Visibility,
		FCollisionShape::MakeSphere(ParkourFrontCheckRadius),
		Params
	);

	if (!bFrontHit || !OutFrontHit.GetActor())
	{
		return false;
	}

	if (!OutFrontHit.GetActor()->ActorHasTag(TAG_PARKOURABLE))
	{
		return false;
	}

	const FVector TopStart = OutFrontHit.ImpactPoint + FVector(0, 0, ParkourTopTraceHeight);
	const FVector TopEnd = OutFrontHit.ImpactPoint - FVector(0, 0, ParkourTopTraceHeight);

	const bool bTopHit = World->LineTraceSingleByChannel(
		OutTopHit,
		TopStart,
		TopEnd,
		ECC_Visibility,
		Params
	);

	if (!bTopHit)
	{
		return false;
	}

	const float FeetZ = GetActorLocation().Z;
	OutTopPoint = OutTopHit.ImpactPoint;
	OutObstacleHeight = OutTopPoint.Z - FeetZ;

	return true;
}

bool ACustomCharacter::ComputeSafeParkourLanding(const FHitResult& FrontHit, const FVector& TopPoint, FVector& OutSafeLocation) const
{
	UWorld* World = GetWorld();
	if (!World) return false;

	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule) return false;

	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();

	const FVector Forward = GetActorForwardVector();

	FVector Desired = TopPoint;
	Desired += Forward * (CapsuleRadius + ParkourLandForwardOffset + ParkourLandingForwardExtra);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ParkourLandTrace), false, this);

	FHitResult GroundHit;
	const FVector GroundStart = Desired + FVector(0, 0, 250.f);
	const FVector GroundEnd = Desired - FVector(0, 0, 600.f);

	if (!World->LineTraceSingleByChannel(GroundHit, GroundStart, GroundEnd, ECC_Visibility, Params))
	{
		return false;
	}

	FVector Candidate = GroundHit.ImpactPoint;
	Candidate.Z += CapsuleHalfHeight + ParkourLandUpOffset;

	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		CapsuleRadius + ParkourLandingCapsuleInflate,
		CapsuleHalfHeight
	);

	FHitResult CapsuleHit;
	const bool bBlocked = World->SweepSingleByChannel(
		CapsuleHit,
		Candidate,
		Candidate,
		FQuat::Identity,
		ECC_WorldStatic,
		CapsuleShape,
		Params
	);

	if (!bBlocked)
	{
		OutSafeLocation = Candidate;
		return true;
	}

	// small forward fallback
	const FVector Candidate2 = Candidate + Forward * (CapsuleRadius * 0.75f);

	const bool bBlocked2 = World->SweepSingleByChannel(
		CapsuleHit,
		Candidate2,
		Candidate2,
		FQuat::Identity,
		ECC_WorldStatic,
		CapsuleShape,
		Params
	);

	if (!bBlocked2)
	{
		OutSafeLocation = Candidate2;
		return true;
	}

	return false;
}

bool ACustomCharacter::ComputeParkourApex_Vault(const FVector& TopPoint, FVector& OutApex) const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule) return false;

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const FVector Forward = GetActorForwardVector();

	// Apex as transtion, on top of the block
	FVector Apex = TopPoint + Forward * (CapsuleRadius + ApexForwardExtra);
	Apex.Z = TopPoint.Z + CapsuleHalfHeight + ApexUpExtra;

	OutApex = Apex;
	return true;
}

bool ACustomCharacter::ComputeParkourApex_Mantle(const FVector& TopPoint, FVector& OutApex) const
{
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!Capsule) return false;

	const float CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
	const FVector Forward = GetActorForwardVector();

	FVector Apex = TopPoint + Forward * (CapsuleRadius + ApexForwardExtra);
	Apex.Z = TopPoint.Z + CapsuleHalfHeight + ApexUpExtra;

	UWorld* World = GetWorld();
	if (!World) return false;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ParkourApexFit), false, this);

	FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(
		CapsuleRadius + ParkourLandingCapsuleInflate,
		CapsuleHalfHeight
	);

	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(
		Hit, Apex, Apex, FQuat::Identity, ECC_WorldStatic, CapsuleShape, Params
	);

	if (!bBlocked)
	{
		OutApex = Apex;
		return true;
	}

	Apex.Z += 20.f;

	const bool bBlocked2 = World->SweepSingleByChannel(
		Hit, Apex, Apex, FQuat::Identity, ECC_WorldStatic, CapsuleShape, Params
	);

	if (!bBlocked2)
	{
		OutApex = Apex;
		return true;
	}

	return false;
}


bool ACustomCharacter::StartParkour(EParkourType Type, const FVector& TargetLocation, const FVector& TopPoint)
{
	if (bInputLocked || bIsParkouring) return false;

	UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr;

	UAnimMontage* MontageToPlay = nullptr;
	if (Type == EParkourType::Vault)
	{
		MontageToPlay = VaultMontage;
		bIsVaulting = true;
		bIsMantling = false;
	}
	else if (Type == EParkourType::Mantle)
	{
		MontageToPlay = MantleMontage;
		bIsVaulting = false;
		bIsMantling = true;
	}

	CurrentParkourMontage = MontageToPlay;

	// Points
	FVector Apex;
	bool bApexOk = false;

	if (Type == EParkourType::Vault)
	{
		bApexOk = ComputeParkourApex_Vault(TopPoint, Apex); // sempre ok
	}
	else
	{
		bApexOk = ComputeParkourApex_Mantle(TopPoint, Apex); // fit test
	}

	if (!bApexOk)
	{
		UE_LOG(LogTemp, Warning, TEXT("PARKOUR: Apex blocked (mantle)"));
		return false;
	}


	bIsParkouring = true;
	CurrentParkour = Type;

	ParkourStart = GetActorLocation();
	ParkourApex = Apex;
	ParkourTarget = TargetLocation;

	ParkourPhase = EParkourPhase::ToApex;
	PhaseElapsed = 0.f;

	PhaseDuration = (Type == EParkourType::Vault) ? VaultToApexDuration : MantleToApexDuration;

	SetInputLocked(true);

	// Play montage
	if (AnimInstance && MontageToPlay)
	{
		FOnMontageBlendingOutStarted BlendOutDel;
		BlendOutDel.BindUObject(this, &ACustomCharacter::OnParkourMontageBlendOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDel, MontageToPlay);

		FOnMontageEnded EndDel;
		EndDel.BindUObject(this, &ACustomCharacter::OnParkourMontageEnded);
		AnimInstance->Montage_SetEndDelegate(EndDel, MontageToPlay);

		AnimInstance->Montage_Play(MontageToPlay, 1.0f);
	}

	// failsafe: always unlock
	const float Total = ((Type == EParkourType::Vault) ? (VaultToApexDuration + VaultToTargetDuration) : (MantleToApexDuration + MantleToTargetDuration));
	const float FailSafeDelay = FMath::Max(0.15f, Total + ParkourFailSafeExtraTime);

	GetWorldTimerManager().ClearTimer(ParkourFailsafeTimer);
	GetWorldTimerManager().SetTimer(
		ParkourFailsafeTimer,
		FTimerDelegate::CreateUObject(this, &ACustomCharacter::EndParkour, false, true),
		FailSafeDelay,
		false
	);

	return true;
}

bool ACustomCharacter::TrySafeMoveDelta(const FVector& Delta)
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move) return false;

	FHitResult Hit;
	Move->SafeMoveUpdatedComponent(Delta, GetActorQuat(), true, Hit);

	if (!Hit.IsValidBlockingHit())
	{
		return true;
	}

	const float RemainingTime = 1.f - Hit.Time;
	if (RemainingTime > KINDA_SMALL_NUMBER)
	{
		const FVector SlideDelta = FVector::VectorPlaneProject(Delta, Hit.Normal) * RemainingTime;

		FHitResult Hit2;
		Move->SafeMoveUpdatedComponent(SlideDelta, GetActorQuat(), true, Hit2);

		if (Hit2.IsValidBlockingHit() && Hit2.Time < MoveStepBlockAbortTime)
		{
			return false;
		}
	}

	const float MovedDist = (Move->UpdatedComponent->GetComponentLocation() - (Move->UpdatedComponent->GetComponentLocation() - Delta)).Size();

	const bool bHardBlocked = (Hit.Time < MoveStepBlockAbortTime);

	return !bHardBlocked;
}

bool ACustomCharacter::ParkourMoveStep(float DeltaSeconds, const FVector& From, const FVector& To, float Duration)
{
	Duration = FMath::Max(0.01f, Duration);
	PhaseElapsed += DeltaSeconds;

	const float Alpha = FMath::Clamp(PhaseElapsed / Duration, 0.f, 1.f);
	const FVector Desired = FMath::Lerp(From, To, Alpha);

	const FVector Current = GetActorLocation();
	FVector Delta = Desired - Current;

	// 1) try normal move
	if (TrySafeMoveDelta(Delta))
	{
		return (Alpha >= 1.0f);
	}

	// 2) fallback: try moving slightly UP
	{
		const FVector UpDelta = Delta + FVector(0, 0, MoveStepFallbackUp);
		if (TrySafeMoveDelta(UpDelta))
		{
			return (Alpha >= 1.0f);
		}
	}

	// 3) fallback: try a bit more FORWARD
	{
		const FVector Fwd = GetActorForwardVector();
		const FVector FwdDelta = Delta + Fwd * MoveStepFallbackForward;
		if (TrySafeMoveDelta(FwdDelta))
		{
			return (Alpha >= 1.0f);
		}
	}

	// 4) still blocked -> stop parkour
	// 4) still blocked -> recovery
	UE_LOG(LogTemp, Warning, TEXT("PARKOUR: blocked during move-step -> recovery"));

	if (CurrentParkour == EParkourType::Mantle && !ParkourTarget.IsZero())
	{
		// tries to palce it on target if it fits
		if (TryTeleportIfFits(ParkourTarget))
		{
			SetActorLocation(ParkourTarget, false, nullptr, ETeleportType::TeleportPhysics);
			EndParkour(false, true);
			return true;
		}

		// fallback: tries higher
		FVector UpTarget = ParkourTarget + FVector(0, 0, 20.f);
		if (TryTeleportIfFits(UpTarget))
		{
			SetActorLocation(UpTarget, false, nullptr, ETeleportType::TeleportPhysics);
			EndParkour(false, true);
			return true;
		}
	}

	EndParkour(true, true);
	return true;
}

void ACustomCharacter::AdvanceParkourPhase()
{
	// apex -> target
	ParkourPhase = EParkourPhase::ToTarget;
	PhaseElapsed = 0.f;

	PhaseDuration = (CurrentParkour == EParkourType::Vault) ? VaultToTargetDuration : MantleToTargetDuration;
}

void ACustomCharacter::OnParkourMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
	// LEGACY CONTENT
}

void ACustomCharacter::OnParkourMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	// LEGACY CONTENT
}

void ACustomCharacter::EndParkour(bool bInterrupted, bool bForce)
{
	if (!bIsParkouring && !bForce) return;

	GetWorldTimerManager().ClearTimer(ParkourFailsafeTimer);

	// reset states
	bIsParkouring = false;
	bIsVaulting = false;
	bIsMantling = false;

	CurrentParkour = EParkourType::None;
	CurrentParkourMontage = nullptr;

	ParkourPhase = EParkourPhase::None;
	ParkourStart = FVector::ZeroVector;
	ParkourApex = FVector::ZeroVector;
	ParkourTarget = FVector::ZeroVector;

	PhaseElapsed = 0.f;
	PhaseDuration = 0.f;

	SetInputLocked(false);
}

bool ACustomCharacter::TryTeleportIfFits(const FVector& Location) const
{
	UWorld* World = GetWorld();
	const UCapsuleComponent* Capsule = GetCapsuleComponent();
	if (!World || !Capsule) return false;

	const float R = Capsule->GetScaledCapsuleRadius();
	const float H = Capsule->GetScaledCapsuleHalfHeight();

	FCollisionShape Shape = FCollisionShape::MakeCapsule(R + ParkourLandingCapsuleInflate, H);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ParkourTeleportFit), false, this);

	FHitResult Hit;
	const bool bBlocked = World->SweepSingleByChannel(
		Hit, Location, Location, FQuat::Identity, ECC_WorldStatic, Shape, Params
	);

	return !bBlocked;
}

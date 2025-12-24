#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "CustomCharacter.generated.h"

class UCameraComponent;
class UCustomMovementComponent;
class UInputMappingContext;
class UInputAction;
class UAnimMontage;
class UStaminaWidget;

UENUM(BlueprintType)
enum class EParkourType : uint8
{
	None,
	Vault,
	Mantle
};

UENUM()
enum class EParkourPhase : uint8
{
	None,
	ToApex,
	ToTarget
};

UCLASS()
class TRIALTASK_API ACustomCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ACustomCharacter(const FObjectInitializer& ObjectInitializer);

	// --------------------
	// Input assets
	// --------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_MoveForward;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_MoveRight;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Look;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Jump;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Sprint;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Crouch;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> IA_Parkour; // V

	// --------------------
	// UI (Stamina)
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<UStaminaWidget> StaminaWidgetClass;

	UPROPERTY(Transient)
	TObjectPtr<UStaminaWidget> StaminaWidget;

	// --------------------
	// Montages  ( No Logic )
	// --------------------
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour")
	TObjectPtr<UAnimMontage> VaultMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Parkour")
	TObjectPtr<UAnimMontage> MantleMontage;

	// --------------------
	// Parkour tuning ( Trace Detections )
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Tuning")
	float ParkourFrontCheckDistance = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Tuning")
	float ParkourFrontCheckRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Tuning")
	float ParkourTopTraceHeight = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Tuning")
	float VaultMaxObstacleHeight = 80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Tuning")
	float MantleMaxObstacleHeight = 140.0f;

	// landing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Tuning")
	float ParkourLandForwardOffset = 55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Tuning")
	float ParkourLandUpOffset = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Safety")
	float ParkourLandingForwardExtra = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Safety")
	float ParkourLandingCapsuleInflate = 4.f;

	// --------------------
	// Parkour movement ( Logic )
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Move")
	float VaultToApexDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Move")
	float VaultToTargetDuration = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Move")
	float MantleToApexDuration = 0.30f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Move")
	float MantleToTargetDuration = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Move")
	float ApexForwardExtra = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Move")
	float ApexUpExtra = 6.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Parkour|Safety")
	float ParkourFailSafeExtraTime = 0.25f;

	// --------------------
	// Anim support
	// --------------------
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|State")
	bool bJumpRequested = false;

	UFUNCTION(BlueprintCallable, Category = "Parkour")
	bool IsParkouring() const { return bIsParkouring; }

	UFUNCTION(BlueprintCallable, Category = "Parkour")
	bool IsVaulting() const { return bIsVaulting; }

	UFUNCTION(BlueprintCallable, Category = "Parkour")
	bool IsMantling() const { return bIsMantling; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	// Movement input
	void MoveForward(const FInputActionValue& Value);
	void MoveRight(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);

	// Sprint/Crouch input
	void SprintPressed(const FInputActionValue& Value);
	void SprintReleased(const FInputActionValue& Value);
	void CrouchPressed(const FInputActionValue& Value);
	void CrouchReleased(const FInputActionValue& Value);

	// Parkour input
	void ParkourPressed(const FInputActionValue& Value);

	// Detection
	bool FindParkourObstacle(FHitResult& OutFrontHit, FHitResult& OutTopHit, float& OutObstacleHeight, FVector& OutTopPoint) const;
	EParkourType DecideParkourType(float ObstacleHeight) const;
	bool ComputeSafeParkourLanding(const FHitResult& FrontHit, const FVector& TopPoint, FVector& OutSafeLocation) const;

	// Run
	bool StartParkour(EParkourType Type, const FVector& TargetLocation, const FVector& TopPoint);
	void EndParkour(bool bInterrupted, bool bForce = false);

	UFUNCTION()
	void OnParkourMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION()
	void OnParkourMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);

private:
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	UPROPERTY(Transient)
	TObjectPtr<UCustomMovementComponent> CustomMoveComp;

	// states
	UPROPERTY(Transient)
	bool bIsParkouring = false;

	UPROPERTY(Transient)
	bool bIsVaulting = false;

	UPROPERTY(Transient)
	bool bIsMantling = false;

	UPROPERTY(Transient)
	bool bInputLocked = false;

	UPROPERTY(Transient)
	EParkourType CurrentParkour = EParkourType::None;

	UPROPERTY(Transient)
	TObjectPtr<UAnimMontage> CurrentParkourMontage = nullptr;

	// geometric move
	UPROPERTY(Transient)
	EParkourPhase ParkourPhase = EParkourPhase::None;

	UPROPERTY(Transient)
	FVector ParkourStart = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector ParkourApex = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector ParkourTarget = FVector::ZeroVector;

	UPROPERTY(Transient)
	float PhaseElapsed = 0.f;

	UPROPERTY(Transient)
	float PhaseDuration = 0.f;

	// save/restore gravity
	float SavedGravityScale = 1.0f;

	// apex transition
	bool bVaultApexNoFit = true;

	// Fallbacks tuning ( MANTLE )
	UPROPERTY(EditAnywhere, Category = "Parkour|Move")
	float MoveStepBlockAbortTime = 0.03f; 

	UPROPERTY(EditAnywhere, Category = "Parkour|Move")
	float MoveStepFallbackUp = 18.f;

	UPROPERTY(EditAnywhere, Category = "Parkour|Move")
	float MoveStepFallbackForward = 18.f;


	// Failsafe
	FTimerHandle ParkourFailsafeTimer;

	void SetInputLocked(bool bLocked);

	// Step movement
	bool ParkourMoveStep(float DeltaSeconds, const FVector& From, const FVector& To, float Duration);
	void AdvanceParkourPhase();

	bool ComputeParkourApex_Vault(const FVector& TopPoint, FVector& OutApex) const;
	bool ComputeParkourApex_Mantle(const FVector& TopPoint, FVector& OutApex) const;

	bool TrySafeMoveDelta(const FVector& Delta);

	bool TryTeleportIfFits(const FVector& Location) const;

};

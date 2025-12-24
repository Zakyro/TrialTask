#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "CustomMovementComponent.generated.h"

UENUM()
enum class ECustomMovementMode : uint8
{
	CMOVE_None UMETA(DisplayName = "None"),
	CMOVE_Slide UMETA(DisplayName = "Slide"),
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TRIALTASK_API UCustomMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

public:
	UCustomMovementComponent();

	// --------------------
	// Speed
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed")
	float WalkSpeed = 450.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Speed")
	float SprintSpeed = 750.0f;

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	float GetWalkSpeed() const { return WalkSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	float GetSprintSpeed() const { return SprintSpeed; }

	UFUNCTION(BlueprintCallable, Category = "Movement|Speed")
	float GetHorizontalSpeed() const;

	// --------------------
	// Stamina
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Stamina")
	float StaminaMax = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Stamina")
	float StaminaRegenPerSec = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Stamina")
	float SprintDrainPerSec = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Stamina")
	float SlideDrainPerSec = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Stamina")
	float MinStaminaToSprint = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Stamina")
	float MinStaminaToSlide = 8.0f;

	UFUNCTION(BlueprintCallable, Category = "Movement|Stamina")
	float GetStamina() const { return Stamina; }

	UFUNCTION(BlueprintCallable, Category = "Movement|Stamina")
	float GetStaminaNormalized() const { return (StaminaMax > 0.f) ? (Stamina / StaminaMax) : 0.f; }

	// --------------------
	// Sprint
	// --------------------
	UFUNCTION(BlueprintCallable, Category = "Movement|Sprint")
	bool IsSprinting() const { return bIsSprinting; }

	UFUNCTION(BlueprintCallable, Category = "Movement|Sprint")
	void SetSprintRequested(bool bRequested);

	// --------------------
	// Slide
	// --------------------
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideMinStartSpeed = 520.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideMinSpeedToKeep = 120.0f;

	// Decel on flat (uu/s^2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideFlatDecel = 650.0f;

	// Movement "feel" while sliding (lower = more slippery)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float SlideGroundFriction = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide|Tuning")
	float SlideMaxSpeedFlat = 1200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide|Tuning")
	float SlideMaxSpeedDownhill = 2000.0f;

	// Extra brake when moving against the downhill direction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide|Tuning")
	float SlideUphillDecel = 1100.0f;

	// Base downhill acceleration (uu/s^2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide|Tuning")
	float SlideDownhillAccel = 900.0f;

	// Steering acceleration (uu/s^2)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide|Tuning")
	float SlideSteerAccel = 2600.0f;

	// If floor angle is below this, it's treated as "flat"
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slide|Tuning")
	float SlideSlopeAngleMinDeg = 4.0f;

	// Grace window after sprint: can press only CTRL to slide
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Slide")
	float PostSprintSlideGraceTime = 0.25f;

	UFUNCTION(BlueprintCallable, Category = "Movement|Slide")
	bool IsSliding() const { return bIsSliding; }

	UFUNCTION(BlueprintCallable, Category = "Movement|Slide")
	bool CanStartSlide() const;

	UFUNCTION(BlueprintCallable, Category = "Movement|Slide")
	void StartSlide();

	UFUNCTION(BlueprintCallable, Category = "Movement|Slide")
	void StopSlide();

	// --------------------
	// Crouch helper (state only)
	// --------------------
	UFUNCTION(BlueprintCallable, Category = "Movement|Crouch")
	void SetCrouchRequested(bool bRequested) { bCrouchRequested = bRequested; }

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void PhysWalking(float DeltaTime, int32 Iterations) override;
	virtual void PhysCustom(float DeltaTime, int32 Iterations) override;

private:
	UPROPERTY(Transient)
	float Stamina = 100.0f;

	UPROPERTY(Transient)
	bool bIsSprinting = false;

	UPROPERTY(Transient)
	bool bSprintRequested = false;

	UPROPERTY(Transient)
	bool bIsSliding = false;

	UPROPERTY(Transient)
	bool bCrouchRequested = false;

	UPROPERTY(Transient)
	float TimeSinceSprintEnded = 999.0f;

	float DefaultGroundFriction = 8.0f;
	float DefaultBrakingDecel = 2048.0f;

	void UpdateStamina(float DeltaTime);
	void UpdateMaxSpeed();

	void EnterSlide();
	void ExitSlide();

	void PhysSlide(float DeltaTime, int32 Iterations);
};

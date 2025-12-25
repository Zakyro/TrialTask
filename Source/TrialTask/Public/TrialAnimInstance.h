#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "TrialAnimInstance.generated.h"

class ACharacter;
class UCustomMovementComponent;
class ACustomCharacter;

UCLASS()
class TRIALTASK_API UTrialAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|State")
	float Speed = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|State")
	float VerticalVelocity = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|State")
	bool bIsInAir = false;

	// legacy vars
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Legacy")
	bool bIsSliding = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Legacy")
	bool bIsCrouched = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Legacy")
	float Direction = 0.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|State")
	bool bJumpRequested = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Parkour")
	bool bIsParkouring = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Parkour")
	bool bIsVaulting = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Anim|Parkour")
	bool bIsMantling = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Anim|Tuning")
	float DirectionClampAbs = 180.0f;

protected:
	UPROPERTY(Transient)
	TObjectPtr<ACharacter> CachedCharacter;

	UPROPERTY(Transient)
	TObjectPtr<UCustomMovementComponent> CachedMoveComp;

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
};

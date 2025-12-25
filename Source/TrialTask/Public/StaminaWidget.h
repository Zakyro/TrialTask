// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StaminaWidget.generated.h"

class UCustomMovementComponent;
class UProgressBar;

UCLASS()
class TRIALTASK_API UStaminaWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void SetMovementComponent(UCustomMovementComponent* InMoveComp);

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
    // Bound from the widget designer
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> PB_Stamina;

    // Runtime reference, not saved
    UPROPERTY(Transient)
    TObjectPtr<UCustomMovementComponent> MoveComp;
};
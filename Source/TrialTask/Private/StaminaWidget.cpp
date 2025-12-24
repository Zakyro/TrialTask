// Fill out your copyright notice in the Description page of Project Settings.

#include "StaminaWidget.h"

#include "CustomMovementComponent.h"
#include "Components/ProgressBar.h"

void UStaminaWidget::SetMovementComponent(UCustomMovementComponent* InMoveComp)
{
    MoveComp = InMoveComp;
}

void UStaminaWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    // Keeps the bar in a valid state even before a movement component is assigned
    if (PB_Stamina)
    {
        PB_Stamina->SetPercent(1.0f);
    }
}

void UStaminaWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (!MoveComp || !PB_Stamina)
    {
        return;
    }

    PB_Stamina->SetPercent(MoveComp->GetStaminaNormalized());
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "NiosCore_GE_ExecutCalc.generated.h"

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class NIOS_CORE_API UNiosCore_GE_ExecutCalc : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
	
	virtual void Execute_Implementation(
		const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput
	) const override;
};

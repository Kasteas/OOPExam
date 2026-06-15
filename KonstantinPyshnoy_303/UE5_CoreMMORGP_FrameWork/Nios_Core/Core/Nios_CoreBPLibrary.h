// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Nios_CoreBPLibrary.generated.h"

class AActor;
class UAbilitySystemComponent;
class UNiosCore_StatusEffectComponent;

UCLASS()
class NIOS_CORE_API UNios_CoreBPLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_UCLASS_BODY()

    UFUNCTION(BlueprintCallable, meta = (DisplayName = "Execute Sample function", Keywords = "Nios_Core sample test testing"), Category = "Nios_CoreTesting")
    static float Nios_CoreSampleFunction(float Param);

    /** Finds StatusEffectComponent on Actor, or through INiosGASActorInterface when implemented. */
    UFUNCTION(BlueprintPure, Category = "Nios|StatusEffects", meta = (DefaultToSelf = "Actor"))
    static UNiosCore_StatusEffectComponent* GetNiosStatusEffectComponentFromActor(AActor* Actor);

    /** Finds StatusEffectComponent through ASC owner/avatar, including PlayerState-owned player ASCs. */
    UFUNCTION(BlueprintPure, Category = "Nios|StatusEffects")
    static UNiosCore_StatusEffectComponent* GetNiosStatusEffectComponentFromASC(UAbilitySystemComponent* ASC);
};

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillEffectTypes.h"

class UAbilitySystemComponent;
class UNiosCore_SkillDataAsset;

struct NIOS_CORE_API FNiosCoreSkillEffectProcessParams
{
    UWorld* World = nullptr;
    UAbilitySystemComponent* SourceASC = nullptr;
    AActor* SourceActor = nullptr;
    AActor* SelectedActor = nullptr;
    const UNiosCore_SkillDataAsset* Skill = nullptr;
    const FGameplayAbilityTargetDataHandle* TargetData = nullptr;
    TArray<UAbilitySystemComponent*> OverrideTargetASCs;

    /** Optional hook used by ability/visual layer for impact cues. */
    TFunction<void(UAbilitySystemComponent*, const FNiosSkillEffectDefinition&)> OnEffectApplied;
};

/**
 * Single place for changing gameplay state from SkillEffects.
 * Target resolving is delegated to TargetResolver; visuals are callback-only.
 */
class NIOS_CORE_API FNiosCoreSkillEffectProcessor
{
public:
    static void ApplyEffectsByIndices(
        const FNiosCoreSkillEffectProcessParams& Params,
        const TArray<int32>& EffectIndices
    );

    static float CalculateEffectValue(
        UAbilitySystemComponent* SourceASC,
        const FNiosSkillEffectDefinition& Effect
    );

    static float NormalizeDelta(
        ENiosSkillEffectType EffectType,
        float RawValue
    );

    /** Clamp Health/Mana/Energy/Rage after direct additive changes. Safe no-op for non-resource attrs. */
    static void ClampResourceAttribute(
        UAbilitySystemComponent* ASC,
        FGameplayAttribute Attribute
    );
};

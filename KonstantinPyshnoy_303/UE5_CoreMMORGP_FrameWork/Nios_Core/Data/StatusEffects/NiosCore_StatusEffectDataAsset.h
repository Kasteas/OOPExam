#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/StatusEffects/NiosCore_StatusEffectTypes.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "NiosCore_StatusEffectDataAsset.generated.h"

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_StatusEffectDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** Pure gameplay config. ID/name/description/icon live in StatusEffectCatalog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect")
    ENiosStatusEffectVisibility Visibility = ENiosStatusEffectVisibility::Personal;

    /**
     * Default target policy when this asset is used by ApplyStatusEffect/RemoveStatusEffect
     * and the SkillEffect TargetPolicy is AutoFromEffectType.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|Targeting")
    ENiosCoreTargetApplicationPolicy TargetPolicy = ENiosCoreTargetApplicationPolicy::AutoFromEffectType;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect")
    ENiosStatusEffectDurationPolicy DurationPolicy = ENiosStatusEffectDurationPolicy::Duration;

    /** Only used by Duration effects. Hidden for Instant and Infinite effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (ClampMin = "0", EditCondition = "DurationPolicy == ENiosStatusEffectDurationPolicy::Duration", EditConditionHides))
    float Duration = 0.f;

    /** Only used by non-instant effects that contain PeriodicAttributeDelta operations. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (ClampMin = "0", EditCondition = "DurationPolicy != ENiosStatusEffectDurationPolicy::Instant", EditConditionHides))
    float TickInterval = 0.f;

    /** Stacking is irrelevant for Instant effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (ClampMin = "1", EditCondition = "DurationPolicy != ENiosStatusEffectDurationPolicy::Instant", EditConditionHides))
    int32 MaxStacks = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (EditCondition = "DurationPolicy != ENiosStatusEffectDurationPolicy::Instant", EditConditionHides))
    ENiosStatusEffectStackPolicy StackPolicy = ENiosStatusEffectStackPolicy::RefreshDuration;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect")
    FGameplayTagContainer GrantedTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect")
    TArray<FNiosStatusEffectOperation> Operations;

    UFUNCTION(BlueprintPure, Category = "Nios|StatusEffect")
    bool IsDurationBased() const { return DurationPolicy == ENiosStatusEffectDurationPolicy::Duration && Duration > 0.f; }

    UFUNCTION(BlueprintPure, Category = "Nios|StatusEffect")
    bool IsInfinite() const { return DurationPolicy == ENiosStatusEffectDurationPolicy::Infinite; }

    UFUNCTION(BlueprintPure, Category = "Nios|StatusEffect")
    bool HasPeriodicOperations() const;
};

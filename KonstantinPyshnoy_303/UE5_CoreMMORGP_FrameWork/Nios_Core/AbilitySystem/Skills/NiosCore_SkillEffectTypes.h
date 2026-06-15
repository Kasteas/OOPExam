#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "NiosCore_SkillEffectTypes.generated.h"

UENUM(BlueprintType)
enum class ENiosSkillEffectType : uint8
{
    Damage UMETA(DisplayName = "Damage"),
    Heal UMETA(DisplayName = "Heal"),
    ResourceDelta UMETA(DisplayName = "Resource Delta"),
    AttributeDelta UMETA(DisplayName = "Attribute Delta"),
    ApplyStatusEffect UMETA(DisplayName = "Apply Status Effect"),
    RemoveStatusEffect UMETA(DisplayName = "Remove Status Effect")
};

UENUM(BlueprintType)
enum class ENiosSkillEffectTarget : uint8
{
    Auto UMETA(DisplayName = "Auto From Skill Target"),
    Source UMETA(DisplayName = "Source"),
    SelectedTarget UMETA(DisplayName = "Selected Target"),
    AllCachedTargets UMETA(DisplayName = "All Cached Targets")
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosSkillStatScaling
{
    GENERATED_BODY()

    /** Final runtime attribute to read from the source ASC, for example PhysicalModifier, Strength, Stamina. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill")
    FGameplayAttribute SourceAttribute;

    /** Added value = SourceAttributeValue * Scale. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill")
    float Scale = 0.f;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosSkillEffectDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill")
    ENiosSkillEffectType EffectType = ENiosSkillEffectType::Damage;

    /**
     * Auto is the normal GD-facing choice:
     * - Self skill => Source
     * - Actor skill => SelectedTarget
     * - None skill => Source
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill")
    ENiosSkillEffectTarget Target = ENiosSkillEffectTarget::Auto;

    /** Attribute modified by this effect. For Damage/Heal this should usually be Health. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill",
        meta = (EditCondition = "EffectType != ENiosSkillEffectType::ApplyStatusEffect && EffectType != ENiosSkillEffectType::RemoveStatusEffect", EditConditionHides))
    FGameplayAttribute Attribute;

    /** Base absolute value before scaling. Damage/Heal are configured as positive values. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill",
        meta = (EditCondition = "EffectType != ENiosSkillEffectType::ApplyStatusEffect && EffectType != ENiosSkillEffectType::RemoveStatusEffect", EditConditionHides))
    float BaseValue = 0.f;

    /** Optional source stat scalings. Example: PhysicalModifier * 1.0, Strength * 2.0. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill",
        meta = (EditCondition = "EffectType != ENiosSkillEffectType::ApplyStatusEffect && EffectType != ENiosSkillEffectType::RemoveStatusEffect", EditConditionHides))
    TArray<FNiosSkillStatScaling> Scalings;

    /**
     * Per-effect target filtering after Delivery target collection.
     * Auto defaults:
     * - Damage => EnemiesOnly
     * - Heal => FriendliesOnly
     * - Harm skill effect/status => EnemiesOnly
     * - Help skill effect/status => FriendliesOnly
     * - Neutral skill effect/status => SourceOnly unless explicitly overridden
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Targeting")
    ENiosCoreTargetApplicationPolicy TargetPolicy = ENiosCoreTargetApplicationPolicy::AutoFromEffectType;

    /** Main reference path: resolve StatusEffectID through StatusEffectCatalog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|StatusEffect",
        meta = (EditCondition = "EffectType == ENiosSkillEffectType::ApplyStatusEffect || EffectType == ENiosSkillEffectType::RemoveStatusEffect", EditConditionHides))
    FName StatusEffectID = NAME_None;

    /** Optional debug/UI/source name for designer readability. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill", meta = (AdvancedDisplay))
    FName EffectID = NAME_None;

    FORCEINLINE bool IsValid() const
    {
        if (EffectType == ENiosSkillEffectType::ApplyStatusEffect || EffectType == ENiosSkillEffectType::RemoveStatusEffect)
        {
            return !StatusEffectID.IsNone();
        }

        return Attribute.IsValid() && (!FMath::IsNearlyZero(BaseValue) || Scalings.Num() > 0);
    }
};

/** Generic resource cost. Use Mana/Energy/Rage/etc attributes. */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosSkillCost
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill")
    FGameplayAttribute ResourceAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill")
    float Value = 0.f;

    FORCEINLINE bool IsValid() const
    {
        return ResourceAttribute.IsValid() && Value > KINDA_SMALL_NUMBER;
    }
};

/**
 * GameplayCue overrides are optional. Keep bUseDefaultCueTags=true for normal skills.
 * Empty / hidden tags fall back to generic affect-type cues, for example:
 * GameplayCue.Skill.Help.CastLoop, GameplayCue.Skill.Harm.Execute, GameplayCue.Skill.Harm.Impact.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosSkillCueConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cues")
    bool bUseDefaultCueTags = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cues",
        meta = (EditCondition = "!bUseDefaultCueTags", EditConditionHides))
    FGameplayTag CastLoopCueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cues",
        meta = (EditCondition = "!bUseDefaultCueTags", EditConditionHides))
    FGameplayTag ExecuteCueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cues",
        meta = (EditCondition = "!bUseDefaultCueTags", EditConditionHides))
    FGameplayTag ImpactCueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cues",
        meta = (EditCondition = "!bUseDefaultCueTags", EditConditionHides))
    FGameplayTag FailCueTag;
};

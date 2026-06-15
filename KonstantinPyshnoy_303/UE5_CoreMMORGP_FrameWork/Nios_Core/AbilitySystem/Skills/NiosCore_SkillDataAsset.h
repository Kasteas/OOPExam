#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimMontage.h"
#include "Sound/SoundBase.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeDelta.h"
#include "AbilitySystem/Skills/NiosCore_SkillEffectTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillDeliveryTypes.h"
#include "AbilitySystem/Pipeline/NiosCore_GroundTargetValidator.h"
#include "Data/Items/NiosCore_ItemDataTypes.h"
#include "NiosCore_SkillDataAsset.generated.h"

class UNiosAbilityExecution;
class UGameplayAbility;
class UTexture2D;

UENUM(BlueprintType)
enum class ENiosAbilityAffectType : uint8
{
    Neutral,
    Harm,
    Help
};

UENUM(BlueprintType)
enum class ENiosAbilityCastType : uint8
{
    Instant,
    CastTime
};

UENUM(BlueprintType)
enum class ENiosAbilityTargetType : uint8
{
    None,
    Self,
    Actor,

    /** Future-ready ground/location target. Current pipeline can pass target location without requiring selected actor. */
    GroundPoint
};

UENUM(BlueprintType)
enum class ENiosAbilityExecutionPolicy : uint8
{
    DefaultDataDriven UMETA(DisplayName = "Default Data Driven"),
    CustomExecutions UMETA(DisplayName = "Custom Executions")
};

USTRUCT(BlueprintType)
struct FNiosSkillCostDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cost")
    FGameplayAttribute ResourceAttribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cost", meta = (ClampMin = "0"))
    float Value = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Cost")
    bool bSpendResource = true;

    FORCEINLINE bool IsValid() const
    {
        return ResourceAttribute.IsValid() && Value > KINDA_SMALL_NUMBER;
    }
};

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_SkillDataAsset : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FGameplayTagContainer SkillTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
    FGameplayTagContainer RequiredTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements")
    FGameplayTagContainer BlockedTags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "State")
    FGameplayTagContainer GrantedTagsWhileActive;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting")
    ENiosAbilityAffectType AffectType = ENiosAbilityAffectType::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting")
    ENiosAbilityTargetType TargetType = ENiosAbilityTargetType::None;

    /**
     * Actor-target maximum usable range in unreal units.
     * <= 0 keeps legacy behavior and disables actor range validation for this skill.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Range",
        meta = (EditCondition = "TargetType == ENiosAbilityTargetType::Actor", EditConditionHides, ClampMin = "0", UIMin = "0"))
    float Range = 0.f;

    /**
     * Server-side leniency added to Range during authoritative validation.
     * This absorbs replication latency, movement prediction and capsule/contact edge cases without increasing the displayed design range.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Range",
        meta = (EditCondition = "TargetType == ENiosAbilityTargetType::Actor && Range > 0", EditConditionHides, ClampMin = "0", UIMin = "0"))
    float ServerRangeTolerance = 75.f;

    /** Use XY distance for actor range checks. This is usually correct for MMO combat on uneven terrain. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Range",
        meta = (EditCondition = "TargetType == ENiosAbilityTargetType::Actor && Range > 0", EditConditionHides))
    bool bUse2DRangeCheck = true;

    /**
     * Measure approximate edge-to-edge distance instead of actor origin-to-origin distance.
     * Runtime first uses an explicit actor CombatRangeRadiusOverride when present, then actor bounds, then collision radius.
     * This makes large monsters/bosses feel fair while the server remains authoritative.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Range",
        meta = (EditCondition = "TargetType == ENiosAbilityTargetType::Actor && Range > 0", EditConditionHides))
    bool bSubtractActorCollisionRadiusFromRange = true;

    /**
     * When edge-to-edge range is enabled, include actor visual bounds as a radius fallback.
     * Useful for wide bosses where the capsule/pivot is much smaller than the attackable body.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Range",
        meta = (EditCondition = "TargetType == ENiosAbilityTargetType::Actor && Range > 0 && bSubtractActorCollisionRadiusFromRange", EditConditionHides))
    bool bUseActorBoundsForRangeRadius = true;

    /**
     * Optional safety clamp for source/target radius contribution during range checks.
     * <= 0 means no clamp. Use this if attachments/VFX make actor bounds too large.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Range",
        meta = (EditCondition = "TargetType == ENiosAbilityTargetType::Actor && Range > 0 && bSubtractActorCollisionRadiusFromRange", EditConditionHides, ClampMin = "0", UIMin = "0"))
    float MaxActorRangeRadiusContribution = 0.f;

    /** Server-side validation rules for GroundPoint skills. Client preview is cosmetic; the server validates this once on confirm. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Targeting|Ground",
        meta = (EditCondition = "TargetType == ENiosAbilityTargetType::GroundPoint", EditConditionHides))
    FNiosCoreGroundTargetValidationSettings GroundTargetValidation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast")
    ENiosAbilityCastType CastType = ENiosAbilityCastType::Instant;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides, ClampMin = "0"))
    float CastTime = 0.f;

    /** If true, moving too far from the cast start location interrupts CastTime skills before cost/cooldown/effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast|Interrupt",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides))
    bool bCancelCastOnMove = false;

    /** Distance in unreal units allowed during a movement-cancellable cast. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast|Interrupt",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime && bCancelCastOnMove", EditConditionHides, ClampMin = "0"))
    float CastMoveCancelDistance = 50.f;

    /** If true, death interrupts CastTime skills before cost/cooldown/effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast|Interrupt",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides))
    bool bCancelCastOnDeath = true;

    /** If true, the locked cast target is revalidated during the cast. Changing selected target does not cancel the cast. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast|Interrupt",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides))
    bool bCancelCastOnTargetInvalid = true;

    /** Optional tags on the caster ASC that interrupt CastTime skills, e.g. Stun/Silence. Keep this narrow; do not use tags for typed requirements. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast|Interrupt",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides))
    FGameplayTagContainer CastInterruptTags;

    /** Server polling interval for CastTime interrupt checks. No cooldown/resource ticking depends on this. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cast|Interrupt",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides, ClampMin = "0.02", UIMin = "0.02"))
    float CastInterruptCheckInterval = 0.1f;

    /** Typed weapon requirement layer. Prefer enums here over gameplay tags to avoid tag clutter. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements|Weapon")
    bool bRequireWeaponInHands = false;

    /** Empty means any equipped weapon can satisfy the requirement. Otherwise one equipped weapon must match one of these types. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements|Weapon",
        meta = (EditCondition = "bRequireWeaponInHands", EditConditionHides))
    TArray<ENiosWeaponType> RequiredWeaponTypes;

    /** If weapon is equipped but sheathed, auto-activate the toggle/draw ability and fail this skill without cost/cooldown/effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements|Weapon",
        meta = (EditCondition = "bRequireWeaponInHands", EditConditionHides))
    bool bAutoToggleWeaponReadyIfEquipped = true;

    /** Ability class used for auto draw/sheath resolution, usually GA_ToggleWeaponReady. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Requirements|Weapon",
        meta = (EditCondition = "bRequireWeaponInHands && bAutoToggleWeaponReadyIfEquipped", EditConditionHides))
    TSubclassOf<UGameplayAbility> AutoToggleWeaponReadyAbilityClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
    TArray<FNiosSkillCostDefinition> Costs;

    /** Local/personal cooldown for this skill. Stored as absolute end time by the cooldown component at runtime. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (ClampMin = "0"))
    float Cooldown = 0.f;

    /** If true, this skill also starts/checks the player's global skill cooldown. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown")
    bool bUseGlobalCooldown = true;

    /**
     * Optional per-skill global cooldown override.
     * < 0: use CooldownComponent.GlobalSkillCooldown.
     * = 0: do not start global cooldown for this skill.
     * > 0: use this value as global cooldown duration.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cooldown", meta = (EditCondition = "bUseGlobalCooldown", ClampMin = "-1"))
    float GlobalCooldownOverride = -1.f;

    /** Normal skills do not need Execution classes. Use CustomExecutions only for special mechanics. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution")
    ENiosAbilityExecutionPolicy ExecutionPolicy = ENiosAbilityExecutionPolicy::DefaultDataDriven;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Execution",
        meta = (EditCondition = "ExecutionPolicy == ENiosAbilityExecutionPolicy::CustomExecutions", EditConditionHides))
    TArray<TSubclassOf<UNiosAbilityExecution>> Executions;

    /** Designer-authored gameplay effects. Supports target damage + self heal + stat scaling in one skill. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Effects")
    TArray<FNiosSkillEffectDefinition> SkillEffects;

    /** Optional delivery configs for fake projectile / delayed impact / AoE logic. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Delivery")
    TArray<FNiosSkillDeliveryDefinition> Deliveries;

    /** FNiosSkillCueConfig is declared in NiosCore_SkillEffectTypes.h. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cue")
    FNiosSkillCueConfig CueConfig;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual")
    TSoftObjectPtr<UAnimMontage> ExecuteMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> ExecuteSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visual|Cast",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides))
    TSoftObjectPtr<UAnimMontage> CastLoopMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio|Cast",
        meta = (EditCondition = "CastType == ENiosAbilityCastType::CastTime", EditConditionHides))
    TSoftObjectPtr<USoundBase> CastLoopSound;

public:
    bool IsCastTimeSkill() const;
    bool HasManaCost() const;
    bool HasCooldown() const;
    bool UsesGlobalCooldown() const;
    float GetLocalCooldownDuration() const;
    float GetGlobalCooldownOverride() const;
    bool HasRange() const;
    float GetRange() const;
    float GetServerRangeTolerance() const;
    float GetServerValidatedRange() const;
    UAnimMontage* LoadCastLoopMontageSynchronous() const;
    UAnimMontage* LoadExecuteMontageSynchronous() const;
    USoundBase* LoadCastLoopSoundSynchronous() const;
    USoundBase* LoadExecuteSoundSynchronous() const;
};

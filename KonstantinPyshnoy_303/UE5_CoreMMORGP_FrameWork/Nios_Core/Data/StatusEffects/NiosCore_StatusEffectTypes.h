#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "Engine/Texture2D.h"
#include "Data/Stats/NiosCore_StatTypes.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "NiosCore_StatusEffectTypes.generated.h"

UENUM(BlueprintType)
enum class ENiosStatusEffectVisibility : uint8
{
    Hidden UMETA(DisplayName = "Hidden"),
    Personal UMETA(DisplayName = "Personal"),
    Public UMETA(DisplayName = "Public")
};

UENUM(BlueprintType)
enum class ENiosStatusEffectSourceType : uint8
{
    Skill UMETA(DisplayName = "Skill"),
    Item UMETA(DisplayName = "Item"),
    Equipment UMETA(DisplayName = "Equipment"),
    Passive UMETA(DisplayName = "Passive"),
    Biome UMETA(DisplayName = "Biome"),
    Quest UMETA(DisplayName = "Quest"),
    Script UMETA(DisplayName = "Script"),
    Aura UMETA(DisplayName = "Aura")
};

UENUM(BlueprintType)
enum class ENiosStatusEffectDurationPolicy : uint8
{
    Instant UMETA(DisplayName = "Instant"),
    Duration UMETA(DisplayName = "Duration"),
    Infinite UMETA(DisplayName = "Infinite")
};

UENUM(BlueprintType)
enum class ENiosStatusEffectStackPolicy : uint8
{
    RefreshDuration UMETA(DisplayName = "Refresh Duration"),
    AddStack UMETA(DisplayName = "Add Stack"),
    Replace UMETA(DisplayName = "Replace"),
    Ignore UMETA(DisplayName = "Ignore")
};


UENUM(BlueprintType)
enum class ENiosCrowdControlType : uint8
{
    None UMETA(DisplayName = "None"),
    Stun UMETA(DisplayName = "Stun"),
    Fear UMETA(DisplayName = "Fear"),
    Sleep UMETA(DisplayName = "Sleep"),
    Charm UMETA(DisplayName = "Charm"),
    Worship UMETA(DisplayName = "Worship"),
    Root UMETA(DisplayName = "Root"),
    Silence UMETA(DisplayName = "Silence"),
    Knockdown UMETA(DisplayName = "Knockdown")
};

UENUM(BlueprintType)
enum class ENiosStatusEffectOperationType : uint8
{
    AttributeModifier UMETA(DisplayName = "Attribute Modifier"),
    PeriodicAttributeDelta UMETA(DisplayName = "Periodic Attribute Delta"),
    GrantTag UMETA(DisplayName = "Grant Tag"),
    RemoveTag UMETA(DisplayName = "Remove Tag"),
    CrowdControl UMETA(DisplayName = "Crowd Control"),
    Aura UMETA(DisplayName = "Aura")
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosStatusEffectStatScaling
{
    GENERATED_BODY()

    /** Final runtime attribute to read from the source/caster ASC. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect")
    FGameplayAttribute SourceAttribute;

    /** Added value = SourceAttributeValue * Scale. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect")
    float Scale = 0.f;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosStatusEffectOperation
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect")
    ENiosStatusEffectOperationType Type = ENiosStatusEffectOperationType::PeriodicAttributeDelta;

    /** Used only by AttributeModifier through StatsComponent. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::AttributeModifier", EditConditionHides))
    ENiosStatType StatType = ENiosStatType::None;

    /** Used only by PeriodicAttributeDelta through ASC. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::PeriodicAttributeDelta", EditConditionHides))
    FGameplayAttribute Attribute;

    /** Base value before source/caster stat scaling. Damage-like periodic effects should use a negative value here. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::AttributeModifier || Type == ENiosStatusEffectOperationType::PeriodicAttributeDelta", EditConditionHides))
    float Value = 0.f;

    /** Optional source/caster stat scaling. Snapshot is calculated when the status effect is applied/refreshed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::AttributeModifier || Type == ENiosStatusEffectOperationType::PeriodicAttributeDelta", EditConditionHides))
    TArray<FNiosStatusEffectStatScaling> Scalings;

    /** Layer used only by AttributeModifier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::AttributeModifier", EditConditionHides))
    ENiosStatModifierLayer ModifierLayer = ENiosStatModifierLayer::Buff;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::GrantTag || Type == ENiosStatusEffectOperationType::RemoveTag || Type == ENiosStatusEffectOperationType::CrowdControl", EditConditionHides))
    FGameplayTagContainer Tags;

    /** Gameplay control type. The effect component owns gameplay state; visual presentation is resolved on clients by profile/catalog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|CrowdControl",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::CrowdControl", EditConditionHides))
    ENiosCrowdControlType CrowdControlType = ENiosCrowdControlType::None;

    /** Higher priority CC wins for loop visual selection when multiple control effects are active. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|CrowdControl",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::CrowdControl", EditConditionHides))
    int32 CrowdControlPriority = 0;

    /** Optional presentation key. If empty, visual layer may use CC type, e.g. CC.Stun. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|CrowdControl",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::CrowdControl", EditConditionHides))
    FName CrowdControlPresentationID = NAME_None;

    /** StatusEffectID granted by this aura to valid actors inside radius. Resolved through StatusEffectCatalog. Producer effects should grant consumer effects, not other auras. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|Aura",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::Aura", EditConditionHides, DisplayName = "Granted Status Effect ID"))
    FName GrantedStatusEffectID = NAME_None;

    /** Aura radius around the owner/avatar of the active effect. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|Aura",
        meta = (ClampMin = "0", EditCondition = "Type == ENiosStatusEffectOperationType::Aura", EditConditionHides))
    float AuraRadius = 400.f;

    /** How often the single StatusEffectComponent tick scans this aura. No per-aura timers are created. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|Aura",
        meta = (ClampMin = "0.05", EditCondition = "Type == ENiosStatusEffectOperationType::Aura", EditConditionHides))
    float AuraScanInterval = 0.5f;

    /** Maximum affected targets per scan. 0 means unlimited. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|Aura",
        meta = (ClampMin = "0", EditCondition = "Type == ENiosStatusEffectOperationType::Aura", EditConditionHides))
    int32 AuraMaxTargets = 0;

    /** Who can receive the child aura effect. Neutral targets are blocked unless explicitly allowed. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|Aura",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::Aura", EditConditionHides))
    ENiosCoreTargetApplicationPolicy AuraTargetPolicy = ENiosCoreTargetApplicationPolicy::FriendliesOnly;

    /** If true, child effects applied by this aura are removed as soon as the target leaves the aura radius. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffect|Aura",
        meta = (EditCondition = "Type == ENiosStatusEffectOperationType::Aura", EditConditionHides))
    bool bAuraRemoveOnExit = true;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosReplicatedStatusEffect
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    FName EffectID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    FText Description;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    ENiosStatusEffectVisibility Visibility = ENiosStatusEffectVisibility::Personal;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    int32 StackCount = 1;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    float StartTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    float EndTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect")
    FGameplayTagContainer Tags;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect|CrowdControl")
    bool bHasCrowdControl = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect|CrowdControl")
    ENiosCrowdControlType CrowdControlType = ENiosCrowdControlType::None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect|CrowdControl")
    int32 CrowdControlPriority = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffect|CrowdControl")
    FName CrowdControlPresentationID = NAME_None;
};

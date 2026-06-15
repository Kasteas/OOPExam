#pragma once

#include "CoreMinimal.h"
#include "NiosCore_TargetRelationTypes.generated.h"

UENUM(BlueprintType)
enum class ENiosCoreTargetRelation : uint8
{
    Neutral  UMETA(DisplayName = "Neutral"),
    Friendly UMETA(DisplayName = "Friendly"),
    Enemy    UMETA(DisplayName = "Enemy")
};

/**
 * Per-effect target policy. Delivery may find a broad target set; each effect then decides
 * which relation it is allowed to touch. This keeps mixed AoE skills data-driven.
 */
UENUM(BlueprintType)
enum class ENiosCoreTargetApplicationPolicy : uint8
{
    /** Use safe defaults from EffectType / StatusEffect / Skill AffectType. */
    AutoFromEffectType UMETA(DisplayName = "Auto From Effect Type"),

    /** Damage/debuff default. Never affects neutral targets. */
    EnemiesOnly UMETA(DisplayName = "Enemies Only"),

    /** Heal/buff default. Includes self. Never affects neutral targets. */
    FriendliesOnly UMETA(DisplayName = "Friendlies Only"),

    /** Only source/caster. Useful for passives, self buffs, movement setup. */
    SourceOnly UMETA(DisplayName = "Source Only"),

    /** Allows friendly and enemy targets, blocks neutral targets. */
    NonNeutralOnly UMETA(DisplayName = "Non Neutral Only"),

    /** Explicit opt-in. Use for neutral non-stat effects such as movement/interaction. */
    AnyIncludingNeutral UMETA(DisplayName = "Any Including Neutral")
};

namespace NiosCoreTeams
{
    static const FName Player = TEXT("Player");
    static const FName Friendly = TEXT("Player");
    static const FName Enemy = TEXT("Enemy");
    static const FName Neutral = TEXT("Neutral");
    static const FName PlayerPvP = TEXT("PlayerPvP");
}

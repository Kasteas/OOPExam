#pragma once

#include "CoreMinimal.h"
class AActor;

#include "NiosCore_ActionFeedbackTypes.generated.h"

/**
 * High-level reason why a local player's gameplay action was rejected or interrupted.
 *
 * Keep this enum type-safe and UI-friendly. Gameplay systems should emit a reason enum,
 * while UI/audio/localization decide how to present it to the player.
 *
 * Examples:
 * - Skill pipeline: NotEnoughMana, InvalidTarget, TargetOutOfRange, OnCooldown.
 * - Weapon toggle: NoEquippedWeapon, WeaponBusy.
 * - Items/shop/professions later: MissingItem, NotEnoughCurrency, MissingProfession.
 */
UENUM(BlueprintType)
enum class ENiosCoreActionFailReason : uint8
{
    None UMETA(DisplayName = "None"),

    // Resource / payment
    NotEnoughMana UMETA(DisplayName = "Not Enough Mana"),
    NotEnoughEnergy UMETA(DisplayName = "Not Enough Energy"),
    NotEnoughStamina UMETA(DisplayName = "Not Enough Stamina"),
    NotEnoughRage UMETA(DisplayName = "Not Enough Rage"),
    NotEnoughCurrency UMETA(DisplayName = "Not Enough Currency"),

    // Targeting
    NoTarget UMETA(DisplayName = "No Target"),
    InvalidTarget UMETA(DisplayName = "Invalid Target"),
    TargetDead UMETA(DisplayName = "Target Dead"),
    TargetOutOfRange UMETA(DisplayName = "Target Out Of Range"),
    TargetInvisible UMETA(DisplayName = "Target Invisible"),
    TargetNotFriendly UMETA(DisplayName = "Target Not Friendly"),
    TargetNotEnemy UMETA(DisplayName = "Target Not Enemy"),
    TargetNeutral UMETA(DisplayName = "Target Neutral"),

    // Equipment / weapon readiness
    NoEquippedWeapon UMETA(DisplayName = "No Equipped Weapon"),
    WeaponNotInHands UMETA(DisplayName = "Weapon Not In Hands"),
    WrongWeaponType UMETA(DisplayName = "Wrong Weapon Type"),
    WeaponBusy UMETA(DisplayName = "Weapon Busy"),

    // Actor state
    Dead UMETA(DisplayName = "Dead"),
    Stunned UMETA(DisplayName = "Stunned"),
    Silenced UMETA(DisplayName = "Silenced"),
    Casting UMETA(DisplayName = "Casting"),
    Mounted UMETA(DisplayName = "Mounted"),
    Swimming UMETA(DisplayName = "Swimming"),
    InCombat UMETA(DisplayName = "In Combat"),
    InSafeZone UMETA(DisplayName = "In Safe Zone"),

    // Generic requirements / systems
    MissingItem UMETA(DisplayName = "Missing Item"),
    MissingProfession UMETA(DisplayName = "Missing Profession"),
    MissingQuest UMETA(DisplayName = "Missing Quest"),
    MissingPermission UMETA(DisplayName = "Missing Permission"),
    OnCooldown UMETA(DisplayName = "On Cooldown"),
    AbilityBlocked UMETA(DisplayName = "Ability Blocked"),
    RequirementHandledByAbility UMETA(DisplayName = "Requirement Handled By Ability"),
    ServerRejected UMETA(DisplayName = "Server Rejected"),
    Interrupted UMETA(DisplayName = "Interrupted"),
    Moved UMETA(DisplayName = "Moved"),
    Unknown UMETA(DisplayName = "Unknown")
};

/**
 * Generic action failure payload sent from server gameplay systems to the owning local client.
 *
 * Do not put localized text here. This is a gameplay/UI contract:
 * - Reason: stable enum for UI/audio/VO lookup.
 * - ContextId: optional skill/item/action id, row name, ability name, etc.
 * - RequiredValue/CurrentValue: optional numeric details for messages like "20/50 mana".
 * - SourceActor/TargetActor: optional context for highlights or debug.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreActionFailResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Action Feedback")
    ENiosCoreActionFailReason Reason = ENiosCoreActionFailReason::None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Action Feedback")
    FName ContextId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Action Feedback")
    float RequiredValue = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Action Feedback")
    float CurrentValue = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Action Feedback")
    TObjectPtr<AActor> SourceActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Action Feedback")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Action Feedback")
    FName DebugReason = NAME_None;

    FNiosCoreActionFailResult() = default;

    explicit FNiosCoreActionFailResult(ENiosCoreActionFailReason InReason)
        : Reason(InReason)
    {
    }

    bool IsValidFailure() const
    {
        return Reason != ENiosCoreActionFailReason::None;
    }
};

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "NiosCore_ToggleWeaponReadyAbility.generated.h"

class UAbilityTask_WaitDelay;
class UNiosCore_EquipmentComponent;

UENUM(BlueprintType)
enum class ENiosWeaponToggleFailure : uint8
{
    None UMETA(DisplayName = "None"),
    MissingActorInfo UMETA(DisplayName = "Missing Actor Info"),
    MissingEquipmentComponent UMETA(DisplayName = "Missing Equipment Component"),
    NoEquippedWeapon UMETA(DisplayName = "No Equipped Weapon"),
    AlreadyTransitioning UMETA(DisplayName = "Already Drawing/Sheathing"),
    LostWeaponDuringToggle UMETA(DisplayName = "Lost Weapon During Toggle")
};

/**
 * Generic GAS action for weapon draw/sheathe.
 *
 * Usage:
 * - Hotkey can activate this ability directly to toggle weapon readiness.
 * - SkillRequirementResolver can auto-activate it when a skill requires weapon in hands.
 * - The original skill is NOT queued. Player presses the original skill again after draw finishes.
 *
 * This ability owns gameplay state transitions only:
 *   Sheathed -> Drawing -> InHands
 *   InHands  -> Sheathing -> Sheathed
 *
 * Tech-art/BP side should bind to EquipmentComponent weapon visual events or override the BP_* events below
 * to play montages, debug prints, and attach weapon visuals on animation notifies.
 */
UCLASS(Blueprintable)
class NIOS_CORE_API UNiosCore_ToggleWeaponReadyAbility : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UNiosCore_ToggleWeaponReadyAbility();

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nios|Weapon Ready", meta = (ClampMin = "0"))
    float DrawDuration = 0.45f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nios|Weapon Ready", meta = (ClampMin = "0"))
    float SheatheDuration = 0.35f;

    /** If true, already Drawing/Sheathing ignores repeated toggle requests. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Nios|Weapon Ready")
    bool bIgnoreWhileTransitioning = true;

    /** Last failure reason for UI/debug. Resets on every activation. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Weapon Ready|Debug")
    ENiosWeaponToggleFailure LastFailureReason = ENiosWeaponToggleFailure::None;

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    UFUNCTION()
    void OnToggleDelayFinished();

    void FinishToggleNow();
    void EndToggleAbility(bool bWasCancelled);
    void FailToggle(ENiosWeaponToggleFailure FailureReason, const TCHAR* DebugReason);

    UNiosCore_EquipmentComponent* ResolveEquipmentComponent() const;

    /** BP hook for debug/UI when toggle fails, e.g. "No weapon equipped". */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Weapon Ready")
    void BP_OnWeaponToggleFailed(ENiosWeaponToggleFailure FailureReason);

    /** BP hook when draw/sheathe starts. Good place to play temporary debug widgets or local montage proxy. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Weapon Ready")
    void BP_OnWeaponToggleStarted(bool bIsDrawing, EEquipSlot ActiveSlot, const FReplicatedEquipmentSlot& ActiveSlotData);

    /** BP hook when draw/sheathe finishes successfully. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Weapon Ready")
    void BP_OnWeaponToggleFinished(bool bNowInHands, EEquipSlot ActiveSlot, const FReplicatedEquipmentSlot& ActiveSlotData);

private:
    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitDelay> ToggleDelayTask;

    UPROPERTY()
    TObjectPtr<UNiosCore_EquipmentComponent> CachedEquipmentComponent;

    bool bDrawing = false;
    bool bSheathing = false;
};

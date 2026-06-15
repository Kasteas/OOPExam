#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NiosGASActorInterface.generated.h"

class UAbilitySystemComponent;
class UNiosCore_AbilitySystemComponent;
class UNiosCore_AttributeSet;
class UNiosCore_StatsComponent;
class UNiosCore_EquipmentComponent;
class UNiosCore_InventoryComponent;
class UNiosCore_AbilityLoadoutComponent;
class UNiosCore_CooldownComponent;
class UNiosCore_StatusEffectComponent;

UINTERFACE(BlueprintType)
class NIOS_CORE_API UNiosGASActorInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Common GAS interaction contract.
 *
 * Important:
 * - PlayerState owns long-lived player core components: ASC, stats, inventory, equipment, loadout, cooldowns.
 * - Pawns/Characters are avatar/visual shells and may forward to PlayerState.
 * - AI, dummies, chests and destructibles may own ASC / Stats directly.
 * - Ability/pipeline code should depend on this interface instead of casting to specific actor classes.
 */
class NIOS_CORE_API INiosGASActorInterface
{
    GENERATED_BODY()

public:
    /** Named GetNiosASC to avoid conflicting with existing GetNiosAbilitySystemComponent() helpers. */
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_AbilitySystemComponent* GetNiosASC() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UAbilitySystemComponent* GetAbilitySystemComponentGeneric() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_AttributeSet* GetNiosAttributeSet() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_StatsComponent* GetNiosStatsComponent() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_EquipmentComponent* GetEquipmentComponent() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_InventoryComponent* GetInventoryComponent() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_AbilityLoadoutComponent* GetAbilityLoadoutComponent() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_CooldownComponent* GetCooldownComponent() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    UNiosCore_StatusEffectComponent* GetStatusEffectComponentGeneric() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    bool IsAlive() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    bool CanBeTargetedBy(AActor* SourceActor) const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    FName GetTeamID() const;

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|GAS")
    void HandleDeath(AActor* Killer);
};

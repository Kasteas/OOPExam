// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "Interfaces/NiosMissionManagerInterface.h"
#include "MissionProgressionProviderInterface.h"
#include "NiosCore_PlayerState.generated.h"

class UNiosCore_StatsComponent;
class UNiosCore_EquipmentComponent;
class UNiosCore_InventoryComponent;
class UNiosCore_AbilityLoadoutComponent;
class UNiosCore_CooldownComponent;
class UNiosCore_StatusEffectComponent;
class UMOMissionsManager;
class UNiosCore_GameplayEventRouterComponent;

UCLASS()
class NIOS_CORE_API ANiosCore_PlayerState : public APlayerState, public IAbilitySystemInterface, public INiosGASActorInterface, public INiosMissionManagerInterface, public IMOPlayerProgressionProviderInterface
{
    GENERATED_BODY()

public:
    ANiosCore_PlayerState();
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // INiosGASActorInterface.
    // PlayerState is the long-lived owner of core gameplay components in Nios.
    // Pawns/Characters may still act as visual avatars and can forward to these getters.
    virtual UNiosCore_AbilitySystemComponent* GetNiosASC_Implementation() const override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponentGeneric_Implementation() const override;
    virtual UNiosCore_AttributeSet* GetNiosAttributeSet_Implementation() const override;
    virtual UNiosCore_StatsComponent* GetNiosStatsComponent_Implementation() const override;
    virtual UNiosCore_EquipmentComponent* GetEquipmentComponent_Implementation() const override;
    virtual UNiosCore_InventoryComponent* GetInventoryComponent_Implementation() const override;
    virtual UNiosCore_AbilityLoadoutComponent* GetAbilityLoadoutComponent_Implementation() const override;
    virtual UNiosCore_CooldownComponent* GetCooldownComponent_Implementation() const override;
    virtual UNiosCore_StatusEffectComponent* GetStatusEffectComponentGeneric_Implementation() const override;
    virtual bool IsAlive_Implementation() const override;
    virtual bool CanBeTargetedBy_Implementation(AActor* SourceActor) const override;
    virtual FName GetTeamID_Implementation() const override;
    virtual void HandleDeath_Implementation(AActor* Killer) override;

    /** Forwarded revive hook for cases where the PlayerState-owned ASC is the lifecycle actor. */
    UFUNCTION(BlueprintCallable, Category = "Nios|State")
    void HandleRevive();

    // INiosMissionManagerInterface. PlayerState is the authoritative long-lived owner.
    virtual UMOMissionsManager* GetNiosMissionManager_Implementation() const override;

    // IMOPlayerProgressionProviderInterface. Used by MissionObjectives start requirements.
    virtual int32 GetMissionCurrentLevel_Implementation() const override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Equipment")
    TObjectPtr<UNiosCore_EquipmentComponent> EquipmentComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    TObjectPtr<UNiosCore_StatsComponent> StatsComponent;

    /**
     * Persistent player-owned GAS ability grants.
     * Lives on PlayerState, not Pawn, so utility/item/emote/mount/profession abilities survive pawn swaps.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Ability Loadout")
    TObjectPtr<UNiosCore_AbilityLoadoutComponent> AbilityLoadoutComponent;

    /**
     * Timestamp-based cooldown manager.
     * No server tick: stores authoritative cooldown end times and exposes local UI helpers.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Cooldown")
    TObjectPtr<UNiosCore_CooldownComponent> CooldownComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffects")
    TObjectPtr<UNiosCore_StatusEffectComponent> StatusEffectComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Missions")
    TObjectPtr<UMOMissionsManager> MissionsManagerComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Gameplay Events")
    TObjectPtr<UNiosCore_GameplayEventRouterComponent> GameplayEventRouterComponent;

    // Plain C++ convenience getters.
    // Do not mark these with UFUNCTION using the same names as INiosGASActorInterface events;
    // UHT requires interface event implementations to use the *_Implementation methods above.
    UNiosCore_AbilityLoadoutComponent* GetAbilityLoadoutComponent() const { return AbilityLoadoutComponent; }
    UNiosCore_CooldownComponent* GetCooldownComponent() const { return CooldownComponent; }
    UNiosCore_StatusEffectComponent* GetStatusEffectComponent() const { return StatusEffectComponent; }
    UMOMissionsManager* GetMissionManager() const { return MissionsManagerComponent; }
    UNiosCore_GameplayEventRouterComponent* GetGameplayEventRouterComponent() const { return GameplayEventRouterComponent; }
    UNiosCore_EquipmentComponent* GetEquipmentComponent() const { return EquipmentComponent; }
    UNiosCore_InventoryComponent* GetInventoryComponent() const;

    // Blueprint-facing accessors with names that do not collide with the interface events.
    UFUNCTION(BlueprintPure, Category = "Nios|Ability Loadout")
    UNiosCore_AbilityLoadoutComponent* GetNiosAbilityLoadoutComponent() const { return AbilityLoadoutComponent; }

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown")
    UNiosCore_CooldownComponent* GetNiosCooldownComponent() const { return CooldownComponent; }

    UFUNCTION(BlueprintPure, Category = "Nios|StatusEffects")
    UNiosCore_StatusEffectComponent* GetNiosStatusEffectComponent() const { return StatusEffectComponent; }

    UFUNCTION(BlueprintPure, Category = "Nios|Missions")
    UMOMissionsManager* GetNiosMissionsManager() const { return MissionsManagerComponent; }

    UFUNCTION(BlueprintPure, Category = "Nios|Equipment")
    UNiosCore_EquipmentComponent* GetNiosEquipmentComponent() const { return EquipmentComponent; }

    UFUNCTION(BlueprintPure, Category = "Nios|Stats")
    UNiosCore_StatsComponent* GetStatsComponent() const { return StatsComponent; }

    UFUNCTION(BlueprintPure, Category = "Nios|Gameplay Events")
    UNiosCore_GameplayEventRouterComponent* GetNiosGameplayEventRouterComponent() const { return GameplayEventRouterComponent; }

    UFUNCTION(BlueprintPure, Category = "Nios|Inventory")
    UNiosCore_InventoryComponent* GetNiosInventoryComponent() const { return InventoryComponent; }

    UNiosCore_AttributeSet* GetStatsAttributeSet() const;
    virtual void BeginPlay() override;

    UPROPERTY()
    TObjectPtr<UNiosCore_AbilitySystemComponent> AbilitySystemComponent;

    UFUNCTION(BlueprintCallable)
    void InitAvatar(AActor* Pawn);

    UFUNCTION(BlueprintCallable, Category = "Nios|Ability")
    UNiosCore_AbilitySystemComponent* GetNiosAbilitySystemComponent() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|PvP")
    void SetPvPEnabled(bool bNewPvPEnabled);

    UFUNCTION(Server, Reliable)
    void ServerSetPvPEnabled(bool bNewPvPEnabled);

    UFUNCTION(BlueprintPure, Category = "Nios|PvP")
    bool IsPvPEnabled() const { return bPvPEnabled; }

    UFUNCTION(BlueprintPure, Category = "Nios|PvP")
    bool CanTogglePvP() const { return bAllowPvPToggle; }

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|PvP")
    void BP_OnPvPModeChanged(bool bNewPvPEnabled);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, ReplicatedUsing = OnRep_PvPEnabled, Category = "Nios|PvP")
    bool bPvPEnabled = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|PvP")
    bool bAllowPvPToggle = true;

    UFUNCTION()
    void OnRep_PvPEnabled();

    void ApplyPvPEnabled(bool bNewPvPEnabled);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Inventory")
    TObjectPtr<UNiosCore_InventoryComponent> InventoryComponent = nullptr;

private:
    UPROPERTY()
    TObjectPtr<UNiosCore_AttributeSet> StatsAttributeSet;
};

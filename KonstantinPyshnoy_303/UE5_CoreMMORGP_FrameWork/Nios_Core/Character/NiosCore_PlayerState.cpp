// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/NiosCore_PlayerState.h"

#include "Components/Inventory/NiosCore_InventoryComponent.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Components/Abilities/NiosCore_AbilityLoadoutComponent.h"
#include "Components/Cooldown/NiosCore_CooldownComponent.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "MOMissionsManager.h"
#include "Components/GameplayEvents/NiosCore_GameplayEventRouterComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Controller.h"

ANiosCore_PlayerState::ANiosCore_PlayerState()
{
    AbilitySystemComponent = CreateDefaultSubobject<UNiosCore_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    StatsAttributeSet = CreateDefaultSubobject<UNiosCore_AttributeSet>(TEXT("StatsAttributeSet"));
    InventoryComponent = CreateDefaultSubobject<UNiosCore_InventoryComponent>(TEXT("InventoryComponent"));
    EquipmentComponent = CreateDefaultSubobject<UNiosCore_EquipmentComponent>(TEXT("EquipmentComponent"));
    StatsComponent = CreateDefaultSubobject<UNiosCore_StatsComponent>(TEXT("StatsComponent"));
    AbilityLoadoutComponent = CreateDefaultSubobject<UNiosCore_AbilityLoadoutComponent>(TEXT("AbilityLoadoutComponent"));
    CooldownComponent = CreateDefaultSubobject<UNiosCore_CooldownComponent>(TEXT("CooldownComponent"));
    StatusEffectComponent = CreateDefaultSubobject<UNiosCore_StatusEffectComponent>(TEXT("StatusEffectComponent"));
    MissionsManagerComponent = CreateDefaultSubobject<UMOMissionsManager>(TEXT("MissionsManagerComponent"));
    GameplayEventRouterComponent = CreateDefaultSubobject<UNiosCore_GameplayEventRouterComponent>(TEXT("GameplayEventRouterComponent"));
}

UAbilitySystemComponent* ANiosCore_PlayerState::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ANiosCore_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ANiosCore_PlayerState, bPvPEnabled);
}

UNiosCore_AbilitySystemComponent* ANiosCore_PlayerState::GetNiosASC_Implementation() const
{
    return AbilitySystemComponent;
}

UAbilitySystemComponent* ANiosCore_PlayerState::GetAbilitySystemComponentGeneric_Implementation() const
{
    return AbilitySystemComponent;
}

UNiosCore_AttributeSet* ANiosCore_PlayerState::GetNiosAttributeSet_Implementation() const
{
    return StatsAttributeSet;
}

UNiosCore_StatsComponent* ANiosCore_PlayerState::GetNiosStatsComponent_Implementation() const
{
    return StatsComponent;
}

UNiosCore_EquipmentComponent* ANiosCore_PlayerState::GetEquipmentComponent_Implementation() const
{
    return EquipmentComponent;
}

UNiosCore_InventoryComponent* ANiosCore_PlayerState::GetInventoryComponent_Implementation() const
{
    return InventoryComponent;
}

UNiosCore_AbilityLoadoutComponent* ANiosCore_PlayerState::GetAbilityLoadoutComponent_Implementation() const
{
    return AbilityLoadoutComponent;
}

UNiosCore_CooldownComponent* ANiosCore_PlayerState::GetCooldownComponent_Implementation() const
{
    return CooldownComponent;
}

UNiosCore_StatusEffectComponent* ANiosCore_PlayerState::GetStatusEffectComponentGeneric_Implementation() const
{
    return StatusEffectComponent;
}

bool ANiosCore_PlayerState::IsAlive_Implementation() const
{
    if (const UNiosCore_AttributeSet* AttributeSet = StatsAttributeSet)
    {
        return AttributeSet->GetHealth() > 0.f;
    }

    return true;
}

bool ANiosCore_PlayerState::CanBeTargetedBy_Implementation(AActor* SourceActor) const
{
    // PlayerState owns long-lived gameplay state, but it is not a world target.
    // Targeting should use the current Pawn/Character avatar.
    return false;
}

FName ANiosCore_PlayerState::GetTeamID_Implementation() const
{
    static const FName PlayerTeamName(TEXT("Player"));
    return PlayerTeamName;
}

void ANiosCore_PlayerState::HandleDeath_Implementation(AActor* Killer)
{
    // Death visuals/lifecycle belong to the current avatar.
    // PlayerState intentionally keeps persistent player data alive across pawn swaps,
    // but PlayerState-owned ASC can still be the actor that receives Health writes.
    AActor* AvatarActor = AbilitySystemComponent ? AbilitySystemComponent->GetAvatarActor() : nullptr;

    if (!AvatarActor)
    {
        if (const AController* OwningController = Cast<AController>(GetOwner()))
        {
            AvatarActor = OwningController->GetPawn();
        }
    }

    if (AvatarActor && AvatarActor != this && AvatarActor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
    {
        INiosGASActorInterface::Execute_HandleDeath(AvatarActor, Killer);
    }
}

void ANiosCore_PlayerState::HandleRevive()
{
    AActor* AvatarActor = AbilitySystemComponent ? AbilitySystemComponent->GetAvatarActor() : nullptr;

    if (!AvatarActor)
    {
        if (const AController* OwningController = Cast<AController>(GetOwner()))
        {
            AvatarActor = OwningController->GetPawn();
        }
    }

    if (!AvatarActor || AvatarActor == this)
    {
        return;
    }

    static const FName HandleReviveName(TEXT("HandleRevive"));
    if (UFunction* ReviveFunction = AvatarActor->FindFunction(HandleReviveName))
    {
        AvatarActor->ProcessEvent(ReviveFunction, nullptr);
    }
}

UMOMissionsManager* ANiosCore_PlayerState::GetNiosMissionManager_Implementation() const
{
    return MissionsManagerComponent;
}

UNiosCore_AbilitySystemComponent* ANiosCore_PlayerState::GetNiosAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UNiosCore_InventoryComponent* ANiosCore_PlayerState::GetInventoryComponent() const
{
    return InventoryComponent;
}

UNiosCore_AttributeSet* ANiosCore_PlayerState::GetStatsAttributeSet() const
{
    return StatsAttributeSet;
}

void ANiosCore_PlayerState::BeginPlay()
{
    Super::BeginPlay();
}

void ANiosCore_PlayerState::InitAvatar(AActor* Pawn)
{
    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitOwnerAvatar(this, Pawn, StatsAttributeSet);

        if (AbilityLoadoutComponent)
        {
            AbilityLoadoutComponent->GrantStartupAbilities(AbilitySystemComponent);
        }
    }
}


void ANiosCore_PlayerState::SetPvPEnabled(bool bNewPvPEnabled)
{
    if (!bAllowPvPToggle)
    {
        return;
    }

    if (HasAuthority())
    {
        ApplyPvPEnabled(bNewPvPEnabled);
        return;
    }

    ServerSetPvPEnabled(bNewPvPEnabled);
}

void ANiosCore_PlayerState::ServerSetPvPEnabled_Implementation(bool bNewPvPEnabled)
{
    SetPvPEnabled(bNewPvPEnabled);
}

void ANiosCore_PlayerState::OnRep_PvPEnabled()
{
    BP_OnPvPModeChanged(bPvPEnabled);
}

void ANiosCore_PlayerState::ApplyPvPEnabled(bool bNewPvPEnabled)
{
    if (bPvPEnabled == bNewPvPEnabled)
    {
        return;
    }

    bPvPEnabled = bNewPvPEnabled;
    BP_OnPvPModeChanged(bPvPEnabled);
}

int32 ANiosCore_PlayerState::GetMissionCurrentLevel_Implementation() const
{
    if (StatsComponent)
    {
        return FMath::Max(1, StatsComponent->CurrentLevel);
    }

    if (StatsAttributeSet)
    {
        return FMath::Max(1, FMath::RoundToInt(StatsAttributeSet->GetPlayerLevel()));
    }

    return 1;
}

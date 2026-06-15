#include "Components/Combat/NiosCore_CombatIdentityComponent.h"

UNiosCore_CombatIdentityComponent::UNiosCore_CombatIdentityComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UNiosCore_CombatIdentityComponent::SetImmortal(bool bNewImmortal)
{
    bImmortal = bNewImmortal;
}

void UNiosCore_CombatIdentityComponent::SetCanBeTargeted(bool bNewCanBeTargeted)
{
    bCanBeTargeted = bNewCanBeTargeted;
}

void UNiosCore_CombatIdentityComponent::SetCanDealDamage(bool bNewCanDealDamage)
{
    bCanDealDamage = bNewCanDealDamage;
}

void UNiosCore_CombatIdentityComponent::SetTeamID(FName NewTeamID, bool bEnableOverride)
{
    TeamID = NewTeamID;
    bOverrideTeamID = bEnableOverride;
}

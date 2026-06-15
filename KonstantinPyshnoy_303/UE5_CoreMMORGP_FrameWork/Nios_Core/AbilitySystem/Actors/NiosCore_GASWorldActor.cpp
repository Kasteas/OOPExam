#include "AbilitySystem/Actors/NiosCore_GASWorldActor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "Components/Targeting/NiosCore_TargetFeedbackComponent.h"

ANiosCore_GASWorldActor::ANiosCore_GASWorldActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    AbilitySystemComponent = CreateDefaultSubobject<UNiosCore_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UNiosCore_AttributeSet>(TEXT("AttributeSet"));
    StatsComponent = CreateDefaultSubobject<UNiosCore_StatsComponent>(TEXT("StatsComponent"));
    StatusEffectComponent = CreateDefaultSubobject<UNiosCore_StatusEffectComponent>(TEXT("StatusEffectComponent"));
    TargetFeedbackComponent = CreateDefaultSubobject<UNiosCore_TargetFeedbackComponent>(TEXT("TargetFeedbackComponent"));
}

void ANiosCore_GASWorldActor::BeginPlay()
{
    Super::BeginPlay();

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitAbilityActorInfo(this, this);
    }

    if (StatsComponent)
    {
        StatsComponent->InitializeAbilitySystemComponent(AbilitySystemComponent);
        StatsComponent->InitializeStatsComponent();
    }

    if (StatusEffectComponent)
    {
        StatusEffectComponent->InitializeStatusEffectComponent(AbilitySystemComponent, StatsComponent);
    }
}

UAbilitySystemComponent* ANiosCore_GASWorldActor::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UNiosCore_AbilitySystemComponent* ANiosCore_GASWorldActor::GetNiosAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ANiosCore_GASWorldActor::SetRelationToPlayer(ENiosCoreTargetRelation NewRelation)
{
    RelationToPlayer = NewRelation;
}

void ANiosCore_GASWorldActor::SetImmortal(bool bNewImmortal)
{
    bImmortal = bNewImmortal;
}

void ANiosCore_GASWorldActor::SetCanBeTargeted(bool bNewCanBeTargeted)
{
    bCanBeTargeted = bNewCanBeTargeted;
}

void ANiosCore_GASWorldActor::SetCanDealDamage(bool bNewCanDealDamage)
{
    bCanDealDamage = bNewCanDealDamage;
}

UNiosCore_AbilitySystemComponent* ANiosCore_GASWorldActor::GetNiosASC_Implementation() const
{
    return AbilitySystemComponent;
}

UAbilitySystemComponent* ANiosCore_GASWorldActor::GetAbilitySystemComponentGeneric_Implementation() const
{
    return AbilitySystemComponent;
}

UNiosCore_AttributeSet* ANiosCore_GASWorldActor::GetNiosAttributeSet_Implementation() const
{
    return AttributeSet;
}

UNiosCore_StatsComponent* ANiosCore_GASWorldActor::GetNiosStatsComponent_Implementation() const
{
    return StatsComponent;
}

UNiosCore_StatusEffectComponent* ANiosCore_GASWorldActor::GetStatusEffectComponentGeneric_Implementation() const
{
    return StatusEffectComponent;
}

bool ANiosCore_GASWorldActor::IsAlive_Implementation() const
{
    if (bDead)
    {
        return false;
    }

    if (!AttributeSet)
    {
        return true;
    }

    const float Health = AttributeSet->GetHealth();
    return Health > 0.f || bTreatZeroHealthAsAliveForTesting;
}

bool ANiosCore_GASWorldActor::CanBeTargetedBy_Implementation(AActor* SourceActor) const
{
    return bCanBeTargeted && !bImmortal && SourceActor != this && IsAlive_Implementation();
}

FName ANiosCore_GASWorldActor::GetTeamID_Implementation() const
{
    return ResolveTeamIDFromRelation();
}

void ANiosCore_GASWorldActor::HandleDeath_Implementation(AActor* Killer)
{
    if (bDead)
    {
        return;
    }

    bDead = true;
    BP_OnDeath(Killer);
}

void ANiosCore_GASWorldActor::HandleRevive()
{
    if (!bDead)
    {
        return;
    }

    bDead = false;

    if (HasAuthority() && StatusEffectComponent)
    {
        StatusEffectComponent->RefreshPersistentEffectsAfterRevive();
    }

    BP_OnRevive();
}

FName ANiosCore_GASWorldActor::ResolveTeamIDFromRelation() const
{
    switch (RelationToPlayer)
    {
    case ENiosCoreTargetRelation::Friendly:
        return NiosCoreTeams::Friendly;
    case ENiosCoreTargetRelation::Enemy:
        return NiosCoreTeams::Enemy;
    case ENiosCoreTargetRelation::Neutral:
    default:
        return NiosCoreTeams::Neutral;
    }
}

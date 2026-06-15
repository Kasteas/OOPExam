#include "Rules/Targeting/NiosTargetRulesSubsystem.h"

#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Character/NiosCore_GameMode.h"
#include "Character/NiosCore_PlayerState.h"
#include "Components/Combat/NiosCore_CombatIdentityComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

namespace
{
    static const FName TagImmortal(TEXT("Nios.Immortal"));
    static const FName TagNoCombat(TEXT("Nios.NoCombat"));
    static const FName TagNonAttackable(TEXT("Nios.NonAttackable"));
    static const FName TagNoDamage(TEXT("Nios.NoDamage"));
    static const FName TagTeamPlayer(TEXT("Nios.Team.Player"));
    static const FName TagTeamEnemy(TEXT("Nios.Team.Enemy"));
    static const FName TagTeamNeutral(TEXT("Nios.Team.Neutral"));
}

bool UNiosTargetRulesSubsystem::CanUseSkillOnTarget(
    AActor* SourceActor,
    AActor* TargetActor,
    const UNiosCore_SkillDataAsset* Skill) const
{
    if (!SourceActor || !TargetActor || !Skill)
    {
        return false;
    }

    switch (Skill->AffectType)
    {
    case ENiosAbilityAffectType::Neutral:
        return true;

    case ENiosAbilityAffectType::Harm:
        return CanHarm(SourceActor, TargetActor);

    case ENiosAbilityAffectType::Help:
        return CanHelp(SourceActor, TargetActor);

    default:
        return false;
    }
}

bool UNiosTargetRulesSubsystem::CanHarm(AActor* SourceActor, AActor* TargetActor) const
{
    if (!SourceActor || !TargetActor || SourceActor == TargetActor)
    {
        return false;
    }

    const FNiosCoreLevelCombatRules LevelRules = GetCurrentLevelRules();
    if (!LevelRules.bAllowHostileDamage)
    {
        return false;
    }

    const bool bSourcePlayer = IsPlayerActor(SourceActor);
    const bool bTargetPlayer = IsPlayerActor(TargetActor);
    if (bSourcePlayer && bTargetPlayer && !LevelRules.bAllowPvP)
    {
        return false;
    }

    if (!LevelRules.bAllowNPCCombat && (!bSourcePlayer || !bTargetPlayer))
    {
        return false;
    }

    if (!CanActorDealDamage(SourceActor) || !CanBeHostileTarget(SourceActor, TargetActor))
    {
        return false;
    }

    return GetRelation(SourceActor, TargetActor) == ENiosCoreTargetRelation::Enemy;
}

bool UNiosTargetRulesSubsystem::CanHelp(AActor* SourceActor, AActor* TargetActor) const
{
    if (!SourceActor || !TargetActor)
    {
        return false;
    }

    if (SourceActor == TargetActor)
    {
        return true;
    }

    return GetRelation(SourceActor, TargetActor) == ENiosCoreTargetRelation::Friendly;
}

bool UNiosTargetRulesSubsystem::CanTalk(AActor* SourceActor, AActor* TargetActor) const
{
    if (!SourceActor || !TargetActor || SourceActor == TargetActor)
    {
        return false;
    }

    return GetRelation(SourceActor, TargetActor) == ENiosCoreTargetRelation::Friendly;
}

ENiosCoreTargetRelation UNiosTargetRulesSubsystem::GetRelation(AActor* SourceActor, AActor* TargetActor) const
{
    if (!SourceActor || !TargetActor)
    {
        return ENiosCoreTargetRelation::Neutral;
    }

    if (SourceActor == TargetActor)
    {
        return ENiosCoreTargetRelation::Friendly;
    }

    const bool bSourcePlayer = IsPlayerActor(SourceActor);
    const bool bTargetPlayer = IsPlayerActor(TargetActor);
    if (bSourcePlayer && bTargetPlayer)
    {
        const FNiosCoreLevelCombatRules LevelRules = GetCurrentLevelRules();
        if (!LevelRules.bAllowPvP)
        {
            return ENiosCoreTargetRelation::Friendly;
        }

        // Starter rule: all players are friendly until either side enables PvP.
        // A PvP-enabled player becomes enemy to all players, and all players see them as enemy.
        return (IsPvPEnabled(SourceActor) || IsPvPEnabled(TargetActor))
            ? ENiosCoreTargetRelation::Enemy
            : ENiosCoreTargetRelation::Friendly;
    }

    const FName SourceTeam = ResolveTeamID(SourceActor);
    const FName TargetTeam = ResolveTeamID(TargetActor);

    if (SourceTeam.IsNone() || TargetTeam.IsNone() ||
        SourceTeam == NiosCoreTeams::Neutral || TargetTeam == NiosCoreTeams::Neutral)
    {
        return ENiosCoreTargetRelation::Neutral;
    }

    return SourceTeam == TargetTeam
        ? ENiosCoreTargetRelation::Friendly
        : ENiosCoreTargetRelation::Enemy;
}

bool UNiosTargetRulesSubsystem::CanBeHostileTarget(AActor* SourceActor, AActor* TargetActor) const
{
    if (!SourceActor || !TargetActor || SourceActor == TargetActor)
    {
        return false;
    }

    if (IsActorImmortal(TargetActor))
    {
        return false;
    }

    if (TargetActor->ActorHasTag(TagNoCombat) || TargetActor->ActorHasTag(TagNonAttackable))
    {
        return false;
    }

    if (const UNiosCore_CombatIdentityComponent* CombatIdentity = ResolveCombatIdentity(TargetActor))
    {
        if (!CombatIdentity->CanBeCombatTarget())
        {
            return false;
        }
    }

    if (TargetActor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
    {
        if (!INiosGASActorInterface::Execute_IsAlive(TargetActor) ||
            !INiosGASActorInterface::Execute_CanBeTargetedBy(TargetActor, SourceActor))
        {
            return false;
        }
    }

    return true;
}

bool UNiosTargetRulesSubsystem::IsPvPEnabled(AActor* Actor) const
{
    const ANiosCore_PlayerState* NiosPlayerState = ResolveNiosPlayerState(Actor);
    return NiosPlayerState ? NiosPlayerState->IsPvPEnabled() : false;
}

FNiosCoreLevelCombatRules UNiosTargetRulesSubsystem::GetCurrentLevelRules() const
{
    FNiosCoreLevelCombatRules Rules;

    UWorld* World = GetWorld();
    const ANiosCore_GameMode* NiosGameMode = World ? World->GetAuthGameMode<ANiosCore_GameMode>() : nullptr;
    const UNiosCore_CombatRulesDataAsset* RulesAsset = NiosGameMode ? NiosGameMode->GetCombatRulesDataAsset() : nullptr;
    if (RulesAsset)
    {
        RulesAsset->GetRulesForWorld(const_cast<UNiosTargetRulesSubsystem*>(this), Rules);
    }

    return Rules;
}

FName UNiosTargetRulesSubsystem::ResolveTeamID(AActor* Actor) const
{
    if (!Actor)
    {
        return NAME_None;
    }

    if (Actor->ActorHasTag(TagTeamPlayer))
    {
        return NiosCoreTeams::Player;
    }
    if (Actor->ActorHasTag(TagTeamEnemy))
    {
        return NiosCoreTeams::Enemy;
    }
    if (Actor->ActorHasTag(TagTeamNeutral))
    {
        return NiosCoreTeams::Neutral;
    }

    if (const UNiosCore_CombatIdentityComponent* CombatIdentity = ResolveCombatIdentity(Actor))
    {
        if (CombatIdentity->bOverrideTeamID)
        {
            return CombatIdentity->TeamID;
        }
    }

    if (IsPlayerActor(Actor))
    {
        return NiosCoreTeams::Player;
    }

    if (Actor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
    {
        return INiosGASActorInterface::Execute_GetTeamID(Actor);
    }

    return NAME_None;
}

bool UNiosTargetRulesSubsystem::CanActorDealDamage(AActor* Actor) const
{
    if (!Actor || Actor->ActorHasTag(TagNoCombat) || Actor->ActorHasTag(TagNoDamage))
    {
        return false;
    }

    if (const UNiosCore_CombatIdentityComponent* CombatIdentity = ResolveCombatIdentity(Actor))
    {
        if (!CombatIdentity->CanDealCombatDamage())
        {
            return false;
        }
    }

    // Native starter actors expose bCanDealDamage as a Blueprint property and CanDealDamage() function.
    static const FName CanDealDamageFunctionName(TEXT("CanDealDamage"));
    if (UFunction* CanDealDamageFunction = Actor->FindFunction(CanDealDamageFunctionName))
    {
        struct FNiosCanDealDamageParams
        {
            bool ReturnValue = true;
        };

        FNiosCanDealDamageParams Params;
        Actor->ProcessEvent(CanDealDamageFunction, &Params);
        if (!Params.ReturnValue)
        {
            return false;
        }
    }

    if (Actor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()) &&
        !INiosGASActorInterface::Execute_IsAlive(Actor))
    {
        return false;
    }

    return true;
}

bool UNiosTargetRulesSubsystem::IsActorImmortal(AActor* Actor) const
{
    if (!Actor)
    {
        return true;
    }

    if (Actor->ActorHasTag(TagImmortal) || Actor->ActorHasTag(TagNoCombat))
    {
        return true;
    }

    if (const UNiosCore_CombatIdentityComponent* CombatIdentity = ResolveCombatIdentity(Actor))
    {
        if (CombatIdentity->IsImmortal())
        {
            return true;
        }
    }

    static const FName IsImmortalFunctionName(TEXT("IsImmortal"));
    if (UFunction* IsImmortalFunction = Actor->FindFunction(IsImmortalFunctionName))
    {
        struct FNiosIsImmortalParams
        {
            bool ReturnValue = false;
        };

        FNiosIsImmortalParams Params;
        Actor->ProcessEvent(IsImmortalFunction, &Params);
        if (Params.ReturnValue)
        {
            return true;
        }
    }

    return false;
}

bool UNiosTargetRulesSubsystem::IsPlayerActor(AActor* Actor) const
{
    return ResolveNiosPlayerState(Actor) != nullptr;
}

ANiosCore_PlayerState* UNiosTargetRulesSubsystem::ResolveNiosPlayerState(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

    if (ANiosCore_PlayerState* NiosPlayerState = Cast<ANiosCore_PlayerState>(Actor))
    {
        return NiosPlayerState;
    }

    if (const APawn* Pawn = Cast<APawn>(Actor))
    {
        if (ANiosCore_PlayerState* NiosPlayerState = Pawn->GetPlayerState<ANiosCore_PlayerState>())
        {
            return NiosPlayerState;
        }
    }

    if (const AController* Controller = Cast<AController>(Actor))
    {
        return Controller->GetPlayerState<ANiosCore_PlayerState>();
    }

    if (const APawn* PawnOwner = Cast<APawn>(Actor->GetOwner()))
    {
        if (ANiosCore_PlayerState* NiosPlayerState = PawnOwner->GetPlayerState<ANiosCore_PlayerState>())
        {
            return NiosPlayerState;
        }
    }

    if (const AController* ControllerOwner = Cast<AController>(Actor->GetOwner()))
    {
        return ControllerOwner->GetPlayerState<ANiosCore_PlayerState>();
    }

    return nullptr;
}

const UNiosCore_CombatIdentityComponent* UNiosTargetRulesSubsystem::ResolveCombatIdentity(AActor* Actor) const
{
    return Actor ? Actor->FindComponentByClass<UNiosCore_CombatIdentityComponent>() : nullptr;
}

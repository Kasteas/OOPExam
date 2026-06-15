#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"

bool UNiosCore_SkillDataAsset::IsCastTimeSkill() const
{
    return CastType == ENiosAbilityCastType::CastTime
        && CastTime > KINDA_SMALL_NUMBER;
}

bool UNiosCore_SkillDataAsset::HasManaCost() const
{
    for (const FNiosSkillCostDefinition& Cost : Costs)
    {
        if (Cost.IsValid() && Cost.bSpendResource)
        {
            return true;
        }
    }

    return false;
}

bool UNiosCore_SkillDataAsset::HasCooldown() const
{
    return Cooldown > KINDA_SMALL_NUMBER || UsesGlobalCooldown();
}

bool UNiosCore_SkillDataAsset::UsesGlobalCooldown() const
{
    return bUseGlobalCooldown && !FMath::IsNearlyZero(GlobalCooldownOverride);
}

float UNiosCore_SkillDataAsset::GetLocalCooldownDuration() const
{
    return FMath::Max(0.f, Cooldown);
}

float UNiosCore_SkillDataAsset::GetGlobalCooldownOverride() const
{
    return GlobalCooldownOverride;
}

bool UNiosCore_SkillDataAsset::HasRange() const
{
    return TargetType == ENiosAbilityTargetType::Actor
        && GetRange() > KINDA_SMALL_NUMBER;
}

float UNiosCore_SkillDataAsset::GetRange() const
{
    return FMath::Max(0.f, Range);
}

float UNiosCore_SkillDataAsset::GetServerRangeTolerance() const
{
    return FMath::Max(0.f, ServerRangeTolerance);
}

float UNiosCore_SkillDataAsset::GetServerValidatedRange() const
{
    return GetRange() + GetServerRangeTolerance();
}

UAnimMontage* UNiosCore_SkillDataAsset::LoadCastLoopMontageSynchronous() const
{
    return CastLoopMontage.IsNull() ? nullptr : CastLoopMontage.LoadSynchronous();
}

UAnimMontage* UNiosCore_SkillDataAsset::LoadExecuteMontageSynchronous() const
{
    return ExecuteMontage.IsNull() ? nullptr : ExecuteMontage.LoadSynchronous();
}

USoundBase* UNiosCore_SkillDataAsset::LoadCastLoopSoundSynchronous() const
{
    return CastLoopSound.IsNull() ? nullptr : CastLoopSound.LoadSynchronous();
}

USoundBase* UNiosCore_SkillDataAsset::LoadExecuteSoundSynchronous() const
{
    return ExecuteSound.IsNull() ? nullptr : ExecuteSound.LoadSynchronous();
}
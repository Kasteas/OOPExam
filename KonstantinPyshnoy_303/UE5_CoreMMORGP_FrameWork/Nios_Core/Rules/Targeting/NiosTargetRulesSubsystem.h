#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "Data/Combat/NiosCore_CombatRulesDataAsset.h"

#include "NiosTargetRulesSubsystem.generated.h"

class UNiosCore_SkillDataAsset;
class UNiosCore_CombatIdentityComponent;
class ANiosCore_PlayerState;

UCLASS()
class NIOS_CORE_API UNiosTargetRulesSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules")
    bool CanUseSkillOnTarget(
        AActor* SourceActor,
        AActor* TargetActor,
        const UNiosCore_SkillDataAsset* Skill
    ) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules")
    bool CanHarm(AActor* SourceActor, AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules")
    bool CanHelp(AActor* SourceActor, AActor* TargetActor) const;

    /** Friendly-only interaction gate for dialogue/interact prompts. */
    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules")
    bool CanTalk(AActor* SourceActor, AActor* TargetActor) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules")
    ENiosCoreTargetRelation GetRelation(AActor* SourceActor, AActor* TargetActor) const;

    /** Returns false for immortal/non-attackable/dead actors. */
    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules")
    bool CanBeHostileTarget(AActor* SourceActor, AActor* TargetActor) const;

    /** Simple replicated player PvP state helper. */
    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules|PvP")
    bool IsPvPEnabled(AActor* Actor) const;

    /** Data-driven current level rules. Returns DefaultRules if no asset/rule is configured. */
    UFUNCTION(BlueprintCallable, Category = "Nios|TargetRules|Level")
    FNiosCoreLevelCombatRules GetCurrentLevelRules() const;

private:
    FName ResolveTeamID(AActor* Actor) const;
    bool CanActorDealDamage(AActor* Actor) const;
    bool IsActorImmortal(AActor* Actor) const;
    bool IsPlayerActor(AActor* Actor) const;
    ANiosCore_PlayerState* ResolveNiosPlayerState(AActor* Actor) const;
    const UNiosCore_CombatIdentityComponent* ResolveCombatIdentity(AActor* Actor) const;
};

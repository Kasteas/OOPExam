#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiosCore_CombatRulesDataAsset.generated.h"

/** Per-level combat restrictions. Keep this simple now; expand later into zone/city/war/arena rules. */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLevelCombatRules
{
    GENERATED_BODY()

    /** Empty in DefaultRules. In entries, should match the persistent/current map name, for example L_StarterTown. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Level")
    FName LevelName = NAME_None;

    /** Master switch. If false, no hostile damage is allowed on this level. Useful for peaceful cities. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bAllowHostileDamage = true;

    /** If false, player-vs-player damage/relation is disabled even when a player has PvP mode enabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bAllowPvP = true;

    /** Reserved for the arena/duel pass. For now it is data only. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bAllowDuels = false;

    /** If false, NPC/mob hostile damage is disabled on this level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat")
    bool bAllowNPCCombat = true;
};

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_CombatRulesDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Combat|Rules")
    FNiosCoreLevelCombatRules DefaultRules;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Combat|Rules")
    TArray<FNiosCoreLevelCombatRules> LevelRules;

    UFUNCTION(BlueprintPure, Category = "Nios|Combat|Rules")
    bool FindRulesForLevel(FName LevelName, FNiosCoreLevelCombatRules& OutRules) const;

    UFUNCTION(BlueprintPure, Category = "Nios|Combat|Rules", meta = (WorldContext = "WorldContextObject"))
    bool GetRulesForWorld(UObject* WorldContextObject, FNiosCoreLevelCombatRules& OutRules) const;
};

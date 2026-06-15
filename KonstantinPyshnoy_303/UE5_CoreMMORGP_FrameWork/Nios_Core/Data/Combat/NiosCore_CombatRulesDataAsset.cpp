#include "Data/Combat/NiosCore_CombatRulesDataAsset.h"

#include "Kismet/GameplayStatics.h"

bool UNiosCore_CombatRulesDataAsset::FindRulesForLevel(FName LevelName, FNiosCoreLevelCombatRules& OutRules) const
{
    OutRules = DefaultRules;

    if (LevelName.IsNone())
    {
        return false;
    }

    for (const FNiosCoreLevelCombatRules& Rules : LevelRules)
    {
        if (Rules.LevelName == LevelName)
        {
            OutRules = Rules;
            return true;
        }
    }

    return false;
}

bool UNiosCore_CombatRulesDataAsset::GetRulesForWorld(UObject* WorldContextObject, FNiosCoreLevelCombatRules& OutRules) const
{
    OutRules = DefaultRules;

    if (!WorldContextObject)
    {
        return false;
    }

    const FString LevelNameString = UGameplayStatics::GetCurrentLevelName(WorldContextObject, true);
    return FindRulesForLevel(FName(*LevelNameString), OutRules);
}

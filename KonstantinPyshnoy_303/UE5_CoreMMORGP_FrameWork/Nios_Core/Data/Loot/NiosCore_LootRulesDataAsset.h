#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Loot/NiosCore_LootDataTypes.h"
#include "NiosCore_LootRulesDataAsset.generated.h"

/** Designer-facing preset for chest/corpse/boss loot ownership and party distribution rules. */
UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_LootRulesDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot")
    FNiosCoreLootRules Rules;
};

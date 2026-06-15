#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/Loot/NiosCore_LootDataTypes.h"
#include "NiosCore_LootTableDataAsset.generated.h"

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_LootTableDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot")
    TArray<FNiosCoreLootEntry> Entries;
};

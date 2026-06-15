#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/UI/NiosCore_UIDataTypes.h"
#include "NiosCore_UIDataAsset.generated.h"

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_UIDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI|Stats")
    TArray<FNiosStatDisplayData> StatDisplayData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI|Items")
    TArray<FNiosNamedDisplayData> ItemTypeDisplayData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI|Items")
    TArray<FNiosNamedDisplayData> WeaponTypeDisplayData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI|Items")
    TArray<FNiosNamedDisplayData> ArmorTypeDisplayData;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI|Items")
    TArray<FNiosNamedDisplayData> ArmorSlotDisplayData;

    /** Generic labels used by tooltips: Price, Damage, AttackSpeed, Type, Slot, Bonuses, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI|Tooltip")
    TArray<FNiosNamedDisplayData> TooltipLabelDisplayData;
};

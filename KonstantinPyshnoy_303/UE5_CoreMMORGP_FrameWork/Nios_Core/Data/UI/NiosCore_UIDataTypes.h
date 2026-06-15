#pragma once

#include "CoreMinimal.h"
#include "Data/Stats/NiosCore_StatTypes.h"
#include "NiosCore_UIDataTypes.generated.h"

USTRUCT(BlueprintType)
struct FNiosStatDisplayData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI")
    ENiosStatType StatType = ENiosStatType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI")
    FLinearColor Color = FLinearColor::White;
};

USTRUCT(BlueprintType)
struct FNiosNamedDisplayData
{
    GENERATED_BODY()

    /** Internal key, e.g. TwoHandedSword, Weapon, Price, Damage. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI")
    FName ID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|UI")
    FText Description;
};

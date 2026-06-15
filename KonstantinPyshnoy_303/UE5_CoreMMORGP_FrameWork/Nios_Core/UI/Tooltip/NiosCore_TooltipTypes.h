#pragma once

#include "CoreMinimal.h"
#include "Engine/Texture2D.h"
#include "NiosCore_TooltipTypes.generated.h"

UENUM(BlueprintType)
enum class ENiosTooltipType : uint8
{
    None UMETA(DisplayName = "None"),
    Item UMETA(DisplayName = "Item"),
    Weapon UMETA(DisplayName = "Weapon"),
    Armor UMETA(DisplayName = "Armor"),
    Skill UMETA(DisplayName = "Skill"),
    Stat UMETA(DisplayName = "Stat"),
    Buff UMETA(DisplayName = "Buff"),
    StatusEffect UMETA(DisplayName = "Status Effect"),
    Debug UMETA(DisplayName = "Debug")
};

UENUM(BlueprintType)
enum class ENiosTooltipLineStyle : uint8
{
    Normal UMETA(DisplayName = "Normal"),
    Positive UMETA(DisplayName = "Positive"),
    Negative UMETA(DisplayName = "Negative"),
    Warning UMETA(DisplayName = "Warning"),
    Header UMETA(DisplayName = "Header")
};

USTRUCT(BlueprintType)
struct FNiosTooltipLine
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Label;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Value;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ENiosTooltipLineStyle Style = ENiosTooltipLineStyle::Normal;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bShowWhenZero = false;
};

USTRUCT(BlueprintType)
struct FNiosTooltipSection
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Header;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNiosTooltipLine> Lines;
};

USTRUCT(BlueprintType)
struct FNiosTooltipData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bValid = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ENiosTooltipType Type = ENiosTooltipType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SourceID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Subtitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Count = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bShowCount = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNiosTooltipLine> MainLines;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNiosTooltipSection> Sections;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FText> Tags;

    void Reset()
    {
        *this = FNiosTooltipData();
    }
};

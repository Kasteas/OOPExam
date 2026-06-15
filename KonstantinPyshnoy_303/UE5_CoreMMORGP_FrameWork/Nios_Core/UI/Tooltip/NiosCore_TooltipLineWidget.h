#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Tooltip/NiosCore_TooltipTypes.h"
#include "NiosCore_TooltipLineWidget.generated.h"

class UTextBlock;

/**
 * Base C++ widget for one tooltip line.
 * Create a BP child and bind LabelText / ValueText if you want automatic filling.
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API UNiosCore_TooltipLineWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    virtual void SetLineData(const FNiosTooltipLine& InLineData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Tooltip")
    FNiosTooltipLine GetLineData() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Tooltip")
    FSlateColor GetColorForStyle(ENiosTooltipLineStyle Style) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Tooltip")
    bool ShouldShowLine() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Tooltip")
    void BP_OnLineDataChanged(const FNiosTooltipLine& NewLineData);

    void ApplyLineDataToBoundWidgets();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Tooltip")
    FNiosTooltipLine LineData;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> LabelText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> ValueText = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Style")
    FSlateColor NormalColor = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Style")
    FSlateColor PositiveColor = FLinearColor(0.25f, 0.95f, 0.35f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Style")
    FSlateColor NegativeColor = FLinearColor(0.95f, 0.25f, 0.25f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Style")
    FSlateColor WarningColor = FLinearColor(1.f, 0.75f, 0.25f, 1.f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Style")
    FSlateColor HeaderColor = FLinearColor(0.95f, 0.85f, 0.55f, 1.f);
};

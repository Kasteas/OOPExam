#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Tooltip/NiosCore_TooltipTypes.h"
#include "NiosCore_TooltipSectionWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UNiosCore_TooltipLineWidget;

/**
 * Base C++ widget for a tooltip section.
 * A section has optional header and multiple line widgets.
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API UNiosCore_TooltipSectionWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    virtual void SetSectionData(const FNiosTooltipSection& InSectionData);

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    virtual void ClearSection();

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    void SetLineWidgetClass(TSubclassOf<UNiosCore_TooltipLineWidget> InLineWidgetClass);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Tooltip")
    FNiosTooltipSection GetSectionData() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Tooltip")
    void BP_OnSectionDataChanged(const FNiosTooltipSection& NewSectionData);

    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Tooltip")
    bool ShouldCreateLine(const FNiosTooltipLine& Line) const;
    virtual bool ShouldCreateLine_Implementation(const FNiosTooltipLine& Line) const;

    virtual void BuildLines();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Tooltip")
    FNiosTooltipSection SectionData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip")
    TSubclassOf<UNiosCore_TooltipLineWidget> LineWidgetClass;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> HeaderText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UPanelWidget> LinesBox = nullptr;
};

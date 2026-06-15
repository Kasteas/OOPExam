#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "NiosCore_TooltipTagWidget.generated.h"

class UTextBlock;

/** Small tag/pill widget for tooltip tags: Weapon, Soulbound, Two-Handed, etc. */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API UNiosCore_TooltipTagWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    virtual void SetTagText(const FText& InTagText);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Tooltip")
    FText GetTagText() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Tooltip")
    void BP_OnTagTextChanged(const FText& NewTagText);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Tooltip")
    FText TagTextValue;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> TagText = nullptr;
};

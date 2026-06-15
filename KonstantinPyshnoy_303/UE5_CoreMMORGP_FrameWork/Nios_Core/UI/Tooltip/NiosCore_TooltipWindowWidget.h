#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Tooltip/NiosCore_TooltipTypes.h"
#include "NiosCore_TooltipWindowWidget.generated.h"

class UImage;
class UPanelWidget;
class UTextBlock;
class UNiosCore_TooltipLineWidget;
class UNiosCore_TooltipSectionWidget;
class UNiosCore_TooltipTagWidget;

/**
 * Production-ready base tooltip window.
 * BP owns the look/layout. C++ owns applying FNiosTooltipData to bound widgets.
 *
 * Recommended BP child hierarchy:
 * - IconImage        optional UImage
 * - CountText        optional UTextBlock
 * - TitleText        optional UTextBlock
 * - SubtitleText     optional UTextBlock
 * - DescriptionText  optional UTextBlock
 * - MainLinesBox     optional VerticalBox/PanelWidget
 * - SectionsBox      optional VerticalBox/PanelWidget
 * - TagsBox          optional WrapBox/PanelWidget
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API UNiosCore_TooltipWindowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    virtual void SetTooltipData(const FNiosTooltipData& InTooltipData);

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    virtual void ClearTooltip();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Tooltip")
    FNiosTooltipData GetTooltipData() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Tooltip")
    bool HasValidTooltip() const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Tooltip")
    void BP_OnTooltipDataChanged(const FNiosTooltipData& NewTooltipData);

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Tooltip")
    void BP_OnTooltipCleared();

    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Tooltip")
    bool ShouldCreateLine(const FNiosTooltipLine& Line) const;
    virtual bool ShouldCreateLine_Implementation(const FNiosTooltipLine& Line) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Tooltip")
    bool ShouldCreateSection(const FNiosTooltipSection& Section) const;
    virtual bool ShouldCreateSection_Implementation(const FNiosTooltipSection& Section) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Tooltip")
    bool ShouldCreateTag(const FText& Tag) const;
    virtual bool ShouldCreateTag_Implementation(const FText& Tag) const;

    virtual void ApplyHeader();
    virtual void BuildMainLines();
    virtual void BuildSections();
    virtual void BuildTags();

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    void SetTextVisibilityByContent(UTextBlock* TextBlock, const FText& Text);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Tooltip")
    FNiosTooltipData TooltipData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Classes")
    TSubclassOf<UNiosCore_TooltipLineWidget> LineWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Classes")
    TSubclassOf<UNiosCore_TooltipSectionWidget> SectionWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip|Classes")
    TSubclassOf<UNiosCore_TooltipTagWidget> TagWidgetClass;

    /** Simple and safe default. Later you can replace with async icon loader if needed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Tooltip")
    bool bLoadIconSynchronously = true;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UImage> IconImage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> CountText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> TitleText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> SubtitleText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UTextBlock> DescriptionText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UPanelWidget> MainLinesBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UPanelWidget> SectionsBox = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|Tooltip")
    TObjectPtr<UPanelWidget> TagsBox = nullptr;
};

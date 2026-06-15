#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/StatusEffects/NiosCore_StatusEffectTypes.h"
#include "UI/Tooltip/NiosCore_TooltipTypes.h"
#include "NiosCore_StatusEffectIconWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * Base widget for a single status effect icon.
 * BP owns visual layout. C++ owns applying icon/count/time and building tooltip data.
 *
 * Recommended BP child hierarchy:
 * - IconImage      optional UImage
 * - StackText      optional UTextBlock
 * - RemainingText  optional UTextBlock
 * - TimeText       optional UTextBlock alias for RemainingText
 */
UCLASS(BlueprintType, Blueprintable, meta = (IsBlueprintBase = "true"))
class NIOS_CORE_API UNiosCore_StatusEffectIconWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects|UI")
    void SetStatusEffectData(const FNiosReplicatedStatusEffect& InEffectData);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    const FNiosReplicatedStatusEffect& GetStatusEffectData() const { return EffectData; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    bool HasValidStatusEffect() const { return !EffectData.EffectID.IsNone(); }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    float GetRemainingTime() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    float GetDuration() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    bool IsInfiniteDuration() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    bool BuildTooltipData(FNiosTooltipData& OutTooltipData) const;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|StatusEffects|UI")
    void BP_OnStatusEffectDataChanged(const FNiosReplicatedStatusEffect& NewEffectData);

    UFUNCTION(BlueprintNativeEvent, Category = "Nios|StatusEffects|UI")
    FText FormatRemainingTime(float RemainingSeconds) const;
    virtual FText FormatRemainingTime_Implementation(float RemainingSeconds) const;

    UFUNCTION(BlueprintNativeEvent, Category = "Nios|StatusEffects|UI")
    FText FormatDuration(float DurationSeconds) const;
    virtual FText FormatDuration_Implementation(float DurationSeconds) const;

    virtual void RefreshVisuals();
    virtual void RefreshRemainingTime();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    FNiosReplicatedStatusEffect EffectData;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI")
    bool bLoadIconSynchronously = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI")
    bool bShowRemainingTime = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI")
    bool bShowStacks = true;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    TObjectPtr<UImage> IconImage = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    TObjectPtr<UTextBlock> StackText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    TObjectPtr<UTextBlock> RemainingText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    TObjectPtr<UTextBlock> TimeText = nullptr;
};

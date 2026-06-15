#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/StatusEffects/NiosCore_StatusEffectTypes.h"
#include "NiosCore_StatusEffectGridWidget.generated.h"

class UUniformGridPanel;
class UWrapBox;
class UNiosCore_StatusEffectComponent;
class UNiosCore_StatusEffectIconWidget;

/**
 * Grid view for visible status effects from a provided StatusEffectComponent.
 * Use this for player buff bar, target debuff bar, raid mechanics panel, etc.
 *
 * Recommended BP child hierarchy:
 * - EffectsGrid optional UUniformGridPanel
 * - EffectsWrapBox optional UWrapBox, recommended for adaptive wrapping rows
 */
UCLASS(BlueprintType, Blueprintable, meta = (IsBlueprintBase = "true"))
class NIOS_CORE_API UNiosCore_StatusEffectGridWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects|UI")
    void SetStatusEffectComponent(UNiosCore_StatusEffectComponent* InStatusEffectComponent);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    UNiosCore_StatusEffectComponent* GetStatusEffectComponent() const { return StatusEffectComponent; }

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects|UI")
    void RefreshStatusEffects();

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects|UI")
    void ClearStatusEffects();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    int32 GetLastVisibleEffectCount() const { return LastVisibleEffectCount; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|UI")
    int32 GetLastInputEffectCount() const { return LastInputEffectCount; }

protected:
    UFUNCTION()
    void HandlePublicStatusEffectsChanged(const TArray<FNiosReplicatedStatusEffect>& Effects);

    UFUNCTION()
    void HandlePersonalStatusEffectsChanged(const TArray<FNiosReplicatedStatusEffect>& Effects);

    virtual void BindToComponent();
    virtual void UnbindFromComponent();
    virtual void BuildFromEffects(const TArray<FNiosReplicatedStatusEffect>& Effects);
    virtual bool ShouldShowEffect(const FNiosReplicatedStatusEffect& Effect) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|StatusEffects|UI")
    void BP_OnStatusEffectsRefreshed();

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    TObjectPtr<UNiosCore_StatusEffectComponent> StatusEffectComponent = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI")
    TSubclassOf<UNiosCore_StatusEffectIconWidget> StatusEffectIconWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI", meta = (ClampMin = "1"))
    int32 ColumnCount = 10;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI")
    bool bShowPersonalEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI")
    bool bShowPublicEffects = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI")
    bool bSortByRemainingTime = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|UI|Debug")
    bool bLogRefreshDebug = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    int32 LastInputEffectCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    int32 LastVisibleEffectCount = 0;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    TObjectPtr<UUniformGridPanel> EffectsGrid = nullptr;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|StatusEffects|UI")
    TObjectPtr<UWrapBox> EffectsWrapBox = nullptr;
};

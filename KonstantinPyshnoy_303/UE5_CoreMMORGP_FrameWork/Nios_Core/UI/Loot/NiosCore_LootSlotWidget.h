#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Loot/NiosCore_LootDataTypes.h"
#include "NiosCore_LootSlotWidget.generated.h"

class UButton;
class UTextBlock;
class UNiosCore_InventoryComponent;
class UNiosCore_LootSourceComponent;
class UNiosCore_LootWindowWidget;

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLootSlotViewData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    int32 LootSlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FName ItemID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    int32 Count = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    ENiosCoreLootItemRarity Rarity = ENiosCoreLootItemRarity::Common;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FText DisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FText CountText;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FText RarityText;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FText ReservedForText;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FText LockReasonText;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bCanLoot = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bPendingVote = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bReservedForRequester = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bReservedForOther = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bIsEmpty = true;
};

/**
 * One row in a loot window. Pure UI wrapper around a replicated loot slot.
 * The server still validates all loot requests through InventoryComponent -> LootSourceComponent.
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API UNiosCore_LootSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void BindLootSlot(UNiosCore_LootWindowWidget* InOwningLootWindow, UNiosCore_LootSourceComponent* InLootSourceComponent, UNiosCore_InventoryComponent* InInventoryComponent, const FNiosCoreLootSlot& InLootSlot, const FNiosCoreLootSlotViewData& InViewData);

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void RefreshLootSlot(const FNiosCoreLootSlot& InLootSlot, const FNiosCoreLootSlotViewData& InViewData);

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void RequestLootThisSlot(int32 Count = 0);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|UI")
    const FNiosCoreLootSlot& GetLootSlot() const { return CachedLootSlot; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|UI")
    const FNiosCoreLootSlotViewData& GetViewData() const { return CachedViewData; }

protected:
    virtual void NativeOnInitialized() override;

    UFUNCTION()
    void HandleLootButtonClicked();

    void ApplyViewDataToOptionalWidgets();

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Loot|UI")
    void BP_OnLootSlotDataChanged(const FNiosCoreLootSlotViewData& ViewData);

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ItemNameText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CountText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> RarityText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ReservedForText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LockReasonText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> LootButton = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    TObjectPtr<UNiosCore_LootWindowWidget> OwningLootWindow = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    TObjectPtr<UNiosCore_LootSourceComponent> LootSourceComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    TObjectPtr<UNiosCore_InventoryComponent> InventoryComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FNiosCoreLootSlot CachedLootSlot;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    FNiosCoreLootSlotViewData CachedViewData;
};

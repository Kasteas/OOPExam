#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Loot/NiosCore_LootDataTypes.h"
#include "UI/Loot/NiosCore_LootSlotWidget.h"
#include "NiosCore_LootWindowWidget.generated.h"

class AActor;
class APlayerController;
class UActionSlotResolverSubsystem;
class UButton;
class UPanelWidget;
class UTextBlock;
class UNiosCore_InventoryComponent;
class UNiosCore_LootSourceComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNiosCoreLootWindowRefreshed);

/**
 * Basic loot list window for chests/corpses/world loot.
 * Designer BP only needs a LootListPanel, LootSlotWidgetClass and optional buttons/text blocks.
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API UNiosCore_LootWindowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** Bind to an actor with NiosCore_LootSourceComponent. Inventory can be explicit or auto-resolved from owning player. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void OpenLootSource(AActor* LootSourceActor, UNiosCore_InventoryComponent* InInventoryComponent = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void BindLootSourceComponent(UNiosCore_LootSourceComponent* InLootSourceComponent, UNiosCore_InventoryComponent* InInventoryComponent = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void UnbindLootSource();

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void RefreshLootWindow();

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void RequestLootAll();

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void RequestLootSlot(int32 LootSlotIndex, int32 Count = 0);

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    void CloseLootWindow();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|UI")
    UNiosCore_LootSourceComponent* GetLootSourceComponent() const { return LootSourceComponent; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|UI")
    UNiosCore_InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|UI")
    bool HasVisibleLootRows() const { return VisibleLootRowCount > 0; }

    /** Builds a BP-friendly row payload for custom row widgets. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|UI")
    FNiosCoreLootSlotViewData BuildLootSlotViewData(const FNiosCoreLootSlot& LootSlot) const;

protected:
    virtual void NativeOnInitialized() override;
    virtual void NativeDestruct() override;

    UFUNCTION()
    void HandleLootChanged();

    UFUNCTION()
    void HandleLootRequestFinished(bool bSuccess, int32 AddedCount);

    UFUNCTION()
    void HandleLootAllButtonClicked();

    UFUNCTION()
    void HandleCloseButtonClicked();

    UNiosCore_InventoryComponent* ResolveInventoryComponentFromOwningPlayer() const;
    UActionSlotResolverSubsystem* ResolveResolverSubsystem() const;
    FText ResolveItemDisplayName(FName ItemID) const;
    FText GetRarityDisplayText(ENiosCoreLootItemRarity Rarity) const;
    FText GetRulesDisplayText() const;
    AActor* ResolveLocalLootingActor() const;
    void ApplyHeaderAndEmptyState();

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Loot|UI")
    void BP_OnLootWindowRefreshed();

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Loot|UI")
    void BP_OnLootRequestFinished(bool bSuccess, int32 AddedCount);

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|UI")
    TSubclassOf<UNiosCore_LootSlotWidget> LootSlotWidgetClass;

    /** Auto-resolve inventory from owning player if OpenLootSource was called without an inventory component. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bAutoResolveInventoryFromOwningPlayer = true;

    /** If true, removes itself from parent when source is empty after a refresh. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bCloseWhenEmpty = false;

    /** If true, hides empty state text while at least one visible loot row exists. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bAutoToggleEmptyStateText = true;

    /** Hide rows that the source already marked as empty/looted. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|UI")
    bool bHideEmptyLootSlots = true;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Loot|UI")
    FNiosCoreLootWindowRefreshed OnLootWindowRefreshed;

protected:
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UPanelWidget> LootListPanel = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LootTitleText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LootRulesText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> EmptyStateText = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> LootAllButton = nullptr;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> CloseButton = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    TObjectPtr<UNiosCore_LootSourceComponent> LootSourceComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    TObjectPtr<UNiosCore_InventoryComponent> InventoryComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    TArray<TObjectPtr<UNiosCore_LootSlotWidget>> LootSlotWidgets;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|UI")
    int32 VisibleLootRowCount = 0;
};

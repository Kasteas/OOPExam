#include "UI/Loot/NiosCore_LootSlotWidget.h"

#include "Components/Button.h"
#include "Components/Inventory/NiosCore_InventoryComponent.h"
#include "Components/Loot/NiosCore_LootSourceComponent.h"
#include "Components/TextBlock.h"
#include "UI/Loot/NiosCore_LootWindowWidget.h"

void UNiosCore_LootSlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (LootButton)
    {
        LootButton->OnClicked.RemoveDynamic(this, &UNiosCore_LootSlotWidget::HandleLootButtonClicked);
        LootButton->OnClicked.AddDynamic(this, &UNiosCore_LootSlotWidget::HandleLootButtonClicked);
    }
}

void UNiosCore_LootSlotWidget::BindLootSlot(
    UNiosCore_LootWindowWidget* InOwningLootWindow,
    UNiosCore_LootSourceComponent* InLootSourceComponent,
    UNiosCore_InventoryComponent* InInventoryComponent,
    const FNiosCoreLootSlot& InLootSlot,
    const FNiosCoreLootSlotViewData& InViewData)
{
    OwningLootWindow = InOwningLootWindow;
    LootSourceComponent = InLootSourceComponent;
    InventoryComponent = InInventoryComponent;

    RefreshLootSlot(InLootSlot, InViewData);
}

void UNiosCore_LootSlotWidget::RefreshLootSlot(const FNiosCoreLootSlot& InLootSlot, const FNiosCoreLootSlotViewData& InViewData)
{
    CachedLootSlot = InLootSlot;
    CachedViewData = InViewData;

    ApplyViewDataToOptionalWidgets();
    BP_OnLootSlotDataChanged(CachedViewData);
}

void UNiosCore_LootSlotWidget::RequestLootThisSlot(int32 Count)
{
    if (OwningLootWindow)
    {
        OwningLootWindow->RequestLootSlot(CachedLootSlot.LootSlotIndex, Count);
        return;
    }

    if (InventoryComponent && LootSourceComponent)
    {
        InventoryComponent->RequestLootSlotFromSource(LootSourceComponent->GetOwner(), CachedLootSlot.LootSlotIndex, Count);
    }
}

void UNiosCore_LootSlotWidget::HandleLootButtonClicked()
{
    RequestLootThisSlot(0);
}

void UNiosCore_LootSlotWidget::ApplyViewDataToOptionalWidgets()
{
    if (ItemNameText)
    {
        ItemNameText->SetText(CachedViewData.DisplayName);
    }

    if (CountText)
    {
        CountText->SetText(CachedViewData.CountText);
    }

    if (RarityText)
    {
        RarityText->SetText(CachedViewData.RarityText);
    }

    if (ReservedForText)
    {
        ReservedForText->SetText(CachedViewData.ReservedForText);
    }

    if (LockReasonText)
    {
        LockReasonText->SetText(CachedViewData.LockReasonText);
    }

    if (LootButton)
    {
        LootButton->SetIsEnabled(CachedViewData.bCanLoot && !CachedViewData.bIsEmpty);
    }
}

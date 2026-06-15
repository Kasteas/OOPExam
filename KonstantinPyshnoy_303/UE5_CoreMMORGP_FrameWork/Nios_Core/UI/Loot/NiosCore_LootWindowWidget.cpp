#include "UI/Loot/NiosCore_LootWindowWidget.h"

#include "Components/Button.h"
#include "Components/Inventory/NiosCore_InventoryComponent.h"
#include "Components/Loot/NiosCore_LootSourceComponent.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Data/Resolvers/ActionSlotResolverSubsystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"

namespace
{
    FString Nios_Loot_GetPlayerDisplayName(APlayerState* PlayerState)
    {
        if (!PlayerState)
        {
            return FString();
        }

        const FString PlayerName = PlayerState->GetPlayerName();
        return PlayerName.IsEmpty() ? GetNameSafe(PlayerState) : PlayerName;
    }
}

void UNiosCore_LootWindowWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();

    if (LootAllButton)
    {
        LootAllButton->OnClicked.RemoveDynamic(this, &UNiosCore_LootWindowWidget::HandleLootAllButtonClicked);
        LootAllButton->OnClicked.AddDynamic(this, &UNiosCore_LootWindowWidget::HandleLootAllButtonClicked);
    }

    if (CloseButton)
    {
        CloseButton->OnClicked.RemoveDynamic(this, &UNiosCore_LootWindowWidget::HandleCloseButtonClicked);
        CloseButton->OnClicked.AddDynamic(this, &UNiosCore_LootWindowWidget::HandleCloseButtonClicked);
    }
}

void UNiosCore_LootWindowWidget::NativeDestruct()
{
    UnbindLootSource();
    Super::NativeDestruct();
}

void UNiosCore_LootWindowWidget::OpenLootSource(AActor* LootSourceActor, UNiosCore_InventoryComponent* InInventoryComponent)
{
    UNiosCore_LootSourceComponent* SourceComponent = LootSourceActor ? LootSourceActor->FindComponentByClass<UNiosCore_LootSourceComponent>() : nullptr;
    BindLootSourceComponent(SourceComponent, InInventoryComponent);
}

void UNiosCore_LootWindowWidget::BindLootSourceComponent(UNiosCore_LootSourceComponent* InLootSourceComponent, UNiosCore_InventoryComponent* InInventoryComponent)
{
    if (LootSourceComponent == InLootSourceComponent && InventoryComponent == InInventoryComponent)
    {
        RefreshLootWindow();
        return;
    }

    UnbindLootSource();

    LootSourceComponent = InLootSourceComponent;
    InventoryComponent = InInventoryComponent ? InInventoryComponent : (bAutoResolveInventoryFromOwningPlayer ? ResolveInventoryComponentFromOwningPlayer() : nullptr);

    if (LootSourceComponent)
    {
        LootSourceComponent->OnLootChanged.AddDynamic(this, &UNiosCore_LootWindowWidget::HandleLootChanged);
    }

    if (InventoryComponent)
    {
        InventoryComponent->OnLootRequestFinished.AddDynamic(this, &UNiosCore_LootWindowWidget::HandleLootRequestFinished);
    }

    RefreshLootWindow();
}

void UNiosCore_LootWindowWidget::UnbindLootSource()
{
    if (LootSourceComponent)
    {
        LootSourceComponent->OnLootChanged.RemoveDynamic(this, &UNiosCore_LootWindowWidget::HandleLootChanged);
    }

    if (InventoryComponent)
    {
        InventoryComponent->OnLootRequestFinished.RemoveDynamic(this, &UNiosCore_LootWindowWidget::HandleLootRequestFinished);
    }

    LootSourceComponent = nullptr;
    InventoryComponent = nullptr;

    if (LootListPanel)
    {
        LootListPanel->ClearChildren();
    }

    LootSlotWidgets.Empty();
    VisibleLootRowCount = 0;
}

void UNiosCore_LootWindowWidget::RefreshLootWindow()
{
    VisibleLootRowCount = 0;
    LootSlotWidgets.Empty();

    if (LootListPanel)
    {
        LootListPanel->ClearChildren();
    }

    if (!LootSourceComponent || !LootSlotWidgetClass || !LootListPanel)
    {
        ApplyHeaderAndEmptyState();
        OnLootWindowRefreshed.Broadcast();
        BP_OnLootWindowRefreshed();
        return;
    }

    const TArray<FNiosCoreLootSlot>& LootSlots = LootSourceComponent->GetLootSlots();
    for (const FNiosCoreLootSlot& LootSlot : LootSlots)
    {
        if (bHideEmptyLootSlots && LootSlot.IsEmpty())
        {
            continue;
        }

        UNiosCore_LootSlotWidget* SlotWidget = CreateWidget<UNiosCore_LootSlotWidget>(GetOwningPlayer(), LootSlotWidgetClass);
        if (!SlotWidget)
        {
            continue;
        }

        const FNiosCoreLootSlotViewData ViewData = BuildLootSlotViewData(LootSlot);
        SlotWidget->BindLootSlot(this, LootSourceComponent, InventoryComponent, LootSlot, ViewData);
        LootListPanel->AddChild(SlotWidget);
        LootSlotWidgets.Add(SlotWidget);
        ++VisibleLootRowCount;
    }

    ApplyHeaderAndEmptyState();

    if (bCloseWhenEmpty && VisibleLootRowCount <= 0 && LootSourceComponent->IsGenerated())
    {
        CloseLootWindow();
        return;
    }

    OnLootWindowRefreshed.Broadcast();
    BP_OnLootWindowRefreshed();
}

void UNiosCore_LootWindowWidget::RequestLootAll()
{
    if (!InventoryComponent || !LootSourceComponent)
    {
        return;
    }

    InventoryComponent->RequestLootAllFromSource(LootSourceComponent->GetOwner());
}

void UNiosCore_LootWindowWidget::RequestLootSlot(int32 LootSlotIndex, int32 Count)
{
    if (!InventoryComponent || !LootSourceComponent)
    {
        return;
    }

    InventoryComponent->RequestLootSlotFromSource(LootSourceComponent->GetOwner(), LootSlotIndex, Count);
}

void UNiosCore_LootWindowWidget::CloseLootWindow()
{
    RemoveFromParent();
}

FNiosCoreLootSlotViewData UNiosCore_LootWindowWidget::BuildLootSlotViewData(const FNiosCoreLootSlot& LootSlot) const
{
    FNiosCoreLootSlotViewData ViewData;
    ViewData.LootSlotIndex = LootSlot.LootSlotIndex;
    ViewData.ItemID = LootSlot.ItemID;
    ViewData.Count = LootSlot.Count;
    ViewData.Rarity = LootSlot.Rarity;
    ViewData.DisplayName = ResolveItemDisplayName(LootSlot.ItemID);
    ViewData.CountText = LootSlot.Count > 1 ? FText::AsNumber(LootSlot.Count) : FText::GetEmpty();
    ViewData.RarityText = GetRarityDisplayText(LootSlot.Rarity);
    ViewData.bIsEmpty = LootSlot.IsEmpty();
    ViewData.bPendingVote = LootSlot.VoteState == ENiosCoreLootVoteState::PendingVote;

    if (LootSlot.ReservedForPlayerState)
    {
        ViewData.ReservedForText = FText::Format(NSLOCTEXT("NiosLoot", "ReservedFor", "Reserved: {0}"), FText::FromString(Nios_Loot_GetPlayerDisplayName(LootSlot.ReservedForPlayerState)));
    }

    FNiosCoreLootSlotAccessInfo AccessInfo;
    if (LootSourceComponent)
    {
        LootSourceComponent->GetLootSlotAccessInfo(ResolveLocalLootingActor(), LootSlot.LootSlotIndex, AccessInfo);
        ViewData.bCanLoot = AccessInfo.bCanLoot;
        ViewData.bReservedForRequester = AccessInfo.bReservedForRequester;
        ViewData.bReservedForOther = AccessInfo.bReservedForOther;
        ViewData.LockReasonText = AccessInfo.ReasonText;
    }

    if (ViewData.bCanLoot)
    {
        ViewData.LockReasonText = FText::GetEmpty();
    }

    return ViewData;
}

void UNiosCore_LootWindowWidget::HandleLootChanged()
{
    RefreshLootWindow();
}

void UNiosCore_LootWindowWidget::HandleLootRequestFinished(bool bSuccess, int32 AddedCount)
{
    BP_OnLootRequestFinished(bSuccess, AddedCount);
    RefreshLootWindow();
}

void UNiosCore_LootWindowWidget::HandleLootAllButtonClicked()
{
    RequestLootAll();
}

void UNiosCore_LootWindowWidget::HandleCloseButtonClicked()
{
    CloseLootWindow();
}

UNiosCore_InventoryComponent* UNiosCore_LootWindowWidget::ResolveInventoryComponentFromOwningPlayer() const
{
    if (APawn* OwningPawn = GetOwningPlayerPawn())
    {
        if (UNiosCore_InventoryComponent* Inventory = OwningPawn->FindComponentByClass<UNiosCore_InventoryComponent>())
        {
            return Inventory;
        }
    }

    if (APlayerController* OwningPlayer = GetOwningPlayer())
    {
        if (UNiosCore_InventoryComponent* Inventory = OwningPlayer->FindComponentByClass<UNiosCore_InventoryComponent>())
        {
            return Inventory;
        }

        if (APlayerState* PlayerState = OwningPlayer->PlayerState)
        {
            if (UNiosCore_InventoryComponent* Inventory = PlayerState->FindComponentByClass<UNiosCore_InventoryComponent>())
            {
                return Inventory;
            }
        }
    }

    return nullptr;
}

UActionSlotResolverSubsystem* UNiosCore_LootWindowWidget::ResolveResolverSubsystem() const
{
    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UActionSlotResolverSubsystem>();
        }
    }

    return nullptr;
}

FText UNiosCore_LootWindowWidget::ResolveItemDisplayName(FName ItemID) const
{
    if (ItemID.IsNone())
    {
        return FText::GetEmpty();
    }

    if (UActionSlotResolverSubsystem* ResolverSubsystem = ResolveResolverSubsystem())
    {
        FNiosActionSlotData SlotData;
        SlotData.Type = ENiosActionSlotType::Item;
        SlotData.ActionID = ItemID;
        SlotData.Count = 1;

        FNiosActionSlotVisualData VisualData;
        if (ResolverSubsystem->ResolveVisualData(SlotData, VisualData) && !VisualData.Name.IsEmpty())
        {
            return VisualData.Name;
        }
    }

    return FText::FromName(ItemID);
}

FText UNiosCore_LootWindowWidget::GetRarityDisplayText(ENiosCoreLootItemRarity Rarity) const
{
    switch (Rarity)
    {
    case ENiosCoreLootItemRarity::Uncommon:
        return NSLOCTEXT("NiosLoot", "RarityUncommon", "Uncommon");
    case ENiosCoreLootItemRarity::Rare:
        return NSLOCTEXT("NiosLoot", "RarityRare", "Rare");
    case ENiosCoreLootItemRarity::Epic:
        return NSLOCTEXT("NiosLoot", "RarityEpic", "Epic");
    case ENiosCoreLootItemRarity::Legendary:
        return NSLOCTEXT("NiosLoot", "RarityLegendary", "Legendary");
    case ENiosCoreLootItemRarity::Common:
    default:
        return NSLOCTEXT("NiosLoot", "RarityCommon", "Common");
    }
}

FText UNiosCore_LootWindowWidget::GetRulesDisplayText() const
{
    if (!LootSourceComponent)
    {
        return FText::GetEmpty();
    }

    const FNiosCoreLootRules Rules = LootSourceComponent->GetEffectiveLootRules();

    const UEnum* OwnershipEnum = StaticEnum<ENiosCoreLootOwnershipRule>();
    const UEnum* DistributionEnum = StaticEnum<ENiosCoreLootDistributionRule>();
    const FText OwnershipText = OwnershipEnum ? OwnershipEnum->GetDisplayNameTextByValue(static_cast<int64>(Rules.OwnershipRule)) : FText::GetEmpty();
    const FText DistributionText = DistributionEnum ? DistributionEnum->GetDisplayNameTextByValue(static_cast<int64>(Rules.DistributionRule)) : FText::GetEmpty();

    return FText::Format(NSLOCTEXT("NiosLoot", "RulesFormat", "{0} / {1}"), OwnershipText, DistributionText);
}

AActor* UNiosCore_LootWindowWidget::ResolveLocalLootingActor() const
{
    if (InventoryComponent)
    {
        return InventoryComponent->GetOwner();
    }

    if (APawn* OwningPawn = GetOwningPlayerPawn())
    {
        return OwningPawn;
    }

    return GetOwningPlayer();
}

void UNiosCore_LootWindowWidget::ApplyHeaderAndEmptyState()
{
    if (LootTitleText)
    {
        const FText Title = LootSourceComponent && LootSourceComponent->GetOwner()
            ? FText::FromString(GetNameSafe(LootSourceComponent->GetOwner()))
            : NSLOCTEXT("NiosLoot", "DefaultLootTitle", "Loot");
        LootTitleText->SetText(Title);
    }

    if (LootRulesText)
    {
        LootRulesText->SetText(GetRulesDisplayText());
    }

    if (EmptyStateText && bAutoToggleEmptyStateText)
    {
        EmptyStateText->SetVisibility(VisibleLootRowCount > 0 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    if (LootAllButton)
    {
        LootAllButton->SetIsEnabled(VisibleLootRowCount > 0 && InventoryComponent != nullptr && LootSourceComponent != nullptr);
    }
}

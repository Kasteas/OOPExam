#include "UI/Inventory/NiosCore_InventoryWidget.h"
#include "UI/ActionSlots/NiosCore_ActionSlotWidget.h"
#include "Components/Inventory/NiosCore_InventoryComponent.h"
#include "Data/Resolvers/ActionSlotResolverSubsystem.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "GameFramework/Actor.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/GameInstance.h"


namespace
{
    TCHAR NiosNormalizeInventorySearchChar(TCHAR InChar)
    {
        // FString::ToLower/Contains(IgnoreCase) may be platform/locale dependent for Cyrillic in editor builds.
        // Keep inventory search deterministic for Russian display names by normalizing the common alphabets manually.
        if (InChar >= TEXT('A') && InChar <= TEXT('Z'))
        {
            return InChar + (TEXT('a') - TEXT('A'));
        }

        // Cyrillic А-Я -> а-я.
        if (InChar >= static_cast<TCHAR>(0x0410) && InChar <= static_cast<TCHAR>(0x042F))
        {
            return InChar + static_cast<TCHAR>(0x20);
        }

        // Normalize Ё/ё to е so both "меч" and names containing Ё remain searchable without exact spelling.
        if (InChar == static_cast<TCHAR>(0x0401) || InChar == static_cast<TCHAR>(0x0451))
        {
            return static_cast<TCHAR>(0x0435);
        }

        if (InChar == TEXT('_') || InChar == TEXT('-') || InChar == TEXT('.') || InChar == TEXT(','))
        {
            return TEXT(' ');
        }

        return FChar::ToLower(InChar);
    }

    FString NiosNormalizeInventorySearchString(const FString& InValue)
    {
        const FString TrimmedValue = InValue.TrimStartAndEnd();

        FString Result;
        Result.Reserve(TrimmedValue.Len());

        bool bLastWasSpace = false;
        for (const TCHAR RawChar : TrimmedValue)
        {
            TCHAR NormalizedChar = NiosNormalizeInventorySearchChar(RawChar);
            if (FChar::IsWhitespace(NormalizedChar))
            {
                NormalizedChar = TEXT(' ');
            }

            if (NormalizedChar == TEXT(' '))
            {
                if (!bLastWasSpace && !Result.IsEmpty())
                {
                    Result.AppendChar(TEXT(' '));
                    bLastWasSpace = true;
                }
                continue;
            }

            Result.AppendChar(NormalizedChar);
            bLastWasSpace = false;
        }

        return Result.TrimStartAndEnd();
    }

    bool NiosInventorySearchMatches(const FString& InRawSearch, const FText& InDisplayName, FName InItemID)
    {
        const FString NormalizedSearch = NiosNormalizeInventorySearchString(InRawSearch);
        if (NormalizedSearch.IsEmpty())
        {
            return true;
        }

        const FString NormalizedName = NiosNormalizeInventorySearchString(InDisplayName.ToString());
        const FString NormalizedItemID = NiosNormalizeInventorySearchString(InItemID.ToString());
        const FString Haystack = FString::Printf(TEXT("%s %s"), *NormalizedName, *NormalizedItemID).TrimStartAndEnd();

        if (Haystack.Contains(NormalizedSearch, ESearchCase::CaseSensitive))
        {
            return true;
        }

        TArray<FString> SearchTokens;
        NormalizedSearch.ParseIntoArrayWS(SearchTokens, nullptr, true);
        if (SearchTokens.Num() == 0)
        {
            return true;
        }

        for (const FString& Token : SearchTokens)
        {
            if (!Token.IsEmpty() && !Haystack.Contains(Token, ESearchCase::CaseSensitive))
            {
                return false;
            }
        }

        return true;
    }
}

void UNiosCore_InventoryWidget::BindInventoryComponent(UNiosCore_InventoryComponent* InInventoryComponent)
{
    if (InventoryComponent == InInventoryComponent)
    {
        RefreshFromInventory();
        return;
    }

    UnbindInventoryComponent();
    InventoryComponent = InInventoryComponent;

    if (!InventoryComponent)
        return;

    ResolverSubsystem = ResolveResolverSubsystem();
    if (!ResolverSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("InventoryWidget: ResolverSubsystem not found"));
        return;
    }

    InventoryComponent->OnInventoryChanged.AddDynamic(this, &UNiosCore_InventoryWidget::HandleInventoryChanged);
    InventoryComponent->OnInventorySlotChanged.AddDynamic(this, &UNiosCore_InventoryWidget::HandleInventorySlotChanged);

    EquipmentComponent = nullptr;

    if (InventoryComponent)
    {
        if (AActor* Owner = InventoryComponent->GetOwner())
        {
            EquipmentComponent =
                Owner->FindComponentByClass<UNiosCore_EquipmentComponent>();
        }
    }

    RefreshFromInventory();
}

void UNiosCore_InventoryWidget::UnbindInventoryComponent()
{
    if (InventoryComponent)
    {
        InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UNiosCore_InventoryWidget::HandleInventoryChanged);
        InventoryComponent->OnInventorySlotChanged.RemoveDynamic(this, &UNiosCore_InventoryWidget::HandleInventorySlotChanged);
    }
    InventoryComponent = nullptr;
}

void UNiosCore_InventoryWidget::RefreshFromInventory()
{
    if (!InventoryComponent)
        return;

    if (!ResolverSubsystem)
        ResolverSubsystem = ResolveResolverSubsystem();
    if (!ResolverSubsystem)
        return;

    RebuildCompactInventory();
}


void UNiosCore_InventoryWidget::SetItemTypeFilter(ENiosInventoryItemTypeFilter InFilter)
{
    if (ItemTypeFilter == InFilter)
    {
        RefreshFromInventory();
        return;
    }

    ItemTypeFilter = InFilter;
    RefreshFromInventory();
}

void UNiosCore_InventoryWidget::ShowAllItems()
{
    SetItemTypeFilter(ENiosInventoryItemTypeFilter::All);
}

void UNiosCore_InventoryWidget::ShowEquipmentItems()
{
    SetItemTypeFilter(ENiosInventoryItemTypeFilter::Equipment);
}

void UNiosCore_InventoryWidget::ShowUsableItems()
{
    SetItemTypeFilter(ENiosInventoryItemTypeFilter::Usable);
}

void UNiosCore_InventoryWidget::ShowMiscItems()
{
    SetItemTypeFilter(ENiosInventoryItemTypeFilter::Misc);
}

void UNiosCore_InventoryWidget::SetInventorySearchText(const FText& InSearchText)
{
    SearchText = InSearchText;
    RefreshFromInventory();
}

void UNiosCore_InventoryWidget::ClearInventorySearchText()
{
    SearchText = FText::GetEmpty();
    RefreshFromInventory();
}

void UNiosCore_InventoryWidget::SetInventorySortMode(ENiosInventorySortMode InSortMode)
{
    if (SortMode == InSortMode)
    {
        RefreshFromInventory();
        return;
    }

    SortMode = InSortMode;
    RefreshFromInventory();
}

void UNiosCore_InventoryWidget::SortBySlot()
{
    SetInventorySortMode(ENiosInventorySortMode::Slot);
}

void UNiosCore_InventoryWidget::SortByItemType()
{
    SetInventorySortMode(ENiosInventorySortMode::ItemType);
}

void UNiosCore_InventoryWidget::SortByName()
{
    SetInventorySortMode(ENiosInventorySortMode::Name);
}

void UNiosCore_InventoryWidget::RefreshSlot(int32 InventoryIndex)
{
    if (!InventoryComponent || !ResolverSubsystem)
        return;

    UNiosCore_ActionSlotWidget* SlotWidget = GetSlotByIndex(InventoryIndex);
    if (!SlotWidget) return;

    if (!InventoryComponent->InventorySlots.IsValidIndex(InventoryIndex) ||
        InventoryComponent->InventorySlots[InventoryIndex].IsEmpty())
    {
        SlotWidget->ClearSlot();
        return;
    }

    const FNiosInventorySlot& CurrentSlot = InventoryComponent->InventorySlots[InventoryIndex];
    const FNiosActionSlotData SlotData = MakeActionSlotDataFromInventorySlot(CurrentSlot);
    SlotWidget->SetSlotData(SlotData);
    SlotWidget->RefreshVisualDataFromResolver(ResolverSubsystem);
    SlotWidget->SetEquippedState(InventoryComponent->IsItemEquippedAt(InventoryIndex));

}

void UNiosCore_InventoryWidget::ClearSlot(int32 Index)
{
    if (!Slots.IsValidIndex(Index))
        return;

    if (UNiosCore_ActionSlotWidget* TargetSlot = Slots[Index].Get())
        TargetSlot->ClearSlot();
}

UNiosCore_ActionSlotWidget* UNiosCore_InventoryWidget::GetSlotByIndex(int32 InventoryIndex) const
{
    if (const TObjectPtr<UNiosCore_ActionSlotWidget>* FoundSlot = SlotWidgetByInventoryIndex.Find(InventoryIndex))
        return FoundSlot->Get();
    return nullptr;
}

void UNiosCore_InventoryWidget::NativeDestruct()
{
    UnbindInventoryComponent();
    Super::NativeDestruct();
}

void UNiosCore_InventoryWidget::RebuildCompactInventory()
{
    if (!InventoryComponent || !GridSlots || !SlotWidgetClass)
        return;

    if (!ResolverSubsystem)
        ResolverSubsystem = ResolveResolverSubsystem();

    GridSlots->ClearChildren();
    Slots.Empty();
    SlotWidgetByInventoryIndex.Empty();

    struct FNiosInventoryVisibleSlot
    {
        int32 InventoryIndex = INDEX_NONE;
        FText DisplayName;
        ENiosItemType ItemType = ENiosItemType::None;
    };

    TArray<FNiosInventoryVisibleSlot> VisibleSlots;

    for (int32 InventoryIndex = 0; InventoryIndex < InventoryComponent->InventorySlots.Num(); ++InventoryIndex)
    {
        const FNiosInventorySlot& CurrentSlot = InventoryComponent->InventorySlots[InventoryIndex];
        if (CurrentSlot.IsEmpty())
            continue;

        FNiosInventoryVisibleSlot VisibleSlot;
        VisibleSlot.InventoryIndex = InventoryIndex;

        if (!ShouldDisplayInventorySlot(CurrentSlot, &VisibleSlot.DisplayName, &VisibleSlot.ItemType))
        {
            continue;
        }

        VisibleSlots.Add(VisibleSlot);
    }

    if (SortMode == ENiosInventorySortMode::ItemType)
    {
        VisibleSlots.Sort(
            [this](const FNiosInventoryVisibleSlot& A, const FNiosInventoryVisibleSlot& B)
            {
                const int32 TypeA = GetItemTypeSortPriority(A.ItemType);
                const int32 TypeB = GetItemTypeSortPriority(B.ItemType);
                if (TypeA != TypeB)
                {
                    return TypeA < TypeB;
                }

                const int32 NameCompare = A.DisplayName.ToString().Compare(B.DisplayName.ToString(), ESearchCase::IgnoreCase);
                if (NameCompare != 0)
                {
                    return NameCompare < 0;
                }

                return A.InventoryIndex < B.InventoryIndex;
            });
    }
    else if (SortMode == ENiosInventorySortMode::Name)
    {
        VisibleSlots.Sort(
            [this](const FNiosInventoryVisibleSlot& A, const FNiosInventoryVisibleSlot& B)
            {
                const int32 NameCompare = A.DisplayName.ToString().Compare(B.DisplayName.ToString(), ESearchCase::IgnoreCase);
                if (NameCompare != 0)
                {
                    return NameCompare < 0;
                }

                const int32 TypeA = GetItemTypeSortPriority(A.ItemType);
                const int32 TypeB = GetItemTypeSortPriority(B.ItemType);
                if (TypeA != TypeB)
                {
                    return TypeA < TypeB;
                }

                return A.InventoryIndex < B.InventoryIndex;
            });
    }

    int32 VisualIndex = 0;
    const int32 SafeColumns = FMath::Max(1, Columns);

    for (const FNiosInventoryVisibleSlot& VisibleSlot : VisibleSlots)
    {
        const FNiosInventorySlot& CurrentSlot = InventoryComponent->InventorySlots[VisibleSlot.InventoryIndex];

        UNiosCore_ActionSlotWidget* SlotWidget = CreateInventorySlotWidget(VisualIndex, VisibleSlot.InventoryIndex);
        if (!SlotWidget)
            continue;

        ApplyInventorySlotToWidget(SlotWidget, CurrentSlot, VisibleSlot.InventoryIndex);

        if (UUniformGridSlot* GridSlot = GridSlots->AddChildToUniformGrid(SlotWidget))
        {
            GridSlot->SetColumn(VisualIndex % SafeColumns);
            GridSlot->SetRow(VisualIndex / SafeColumns);
        }

        Slots.Add(SlotWidget);
        SlotWidgetByInventoryIndex.Add(VisibleSlot.InventoryIndex, SlotWidget);

        ++VisualIndex;
    }

    SlotCount = VisualIndex;
}

bool UNiosCore_InventoryWidget::ShouldDisplayInventorySlot(
    const FNiosInventorySlot& InventorySlot,
    FText* OutResolvedName,
    ENiosItemType* OutItemType) const
{
    if (InventorySlot.IsEmpty())
    {
        return false;
    }

    FNiosResolvedInventoryItemData ItemData;
    if (!ResolverSubsystem || !ResolverSubsystem->ResolveInventoryItemData(InventorySlot.ItemID, ItemData) || !ItemData.bValid)
    {
        return false;
    }

    const ENiosItemType ItemType = ItemData.ItemType;
    if (OutItemType)
    {
        *OutItemType = ItemType;
    }

    bool bPassesTypeFilter = true;
    switch (ItemTypeFilter)
    {
    case ENiosInventoryItemTypeFilter::All:
        bPassesTypeFilter = true;
        break;

    case ENiosInventoryItemTypeFilter::Equipment:
        bPassesTypeFilter = ItemType == ENiosItemType::Weapon || ItemType == ENiosItemType::Armor;
        break;

    case ENiosInventoryItemTypeFilter::Weapon:
        bPassesTypeFilter = ItemType == ENiosItemType::Weapon;
        break;

    case ENiosInventoryItemTypeFilter::Armor:
        bPassesTypeFilter = ItemType == ENiosItemType::Armor;
        break;

    case ENiosInventoryItemTypeFilter::Usable:
        bPassesTypeFilter = ItemType == ENiosItemType::Usable;
        break;

    case ENiosInventoryItemTypeFilter::Misc:
        bPassesTypeFilter = ItemType == ENiosItemType::Misc;
        break;

    default:
        bPassesTypeFilter = true;
        break;
    }

    if (!bPassesTypeFilter)
    {
        return false;
    }

    const FText DisplayName = ResolveItemDisplayName(InventorySlot.ItemID);
    if (OutResolvedName)
    {
        *OutResolvedName = DisplayName;
    }

    const FString Search = SearchText.ToString();
    if (!NiosInventorySearchMatches(Search, DisplayName, InventorySlot.ItemID))
    {
        return false;
    }

    return true;
}

FText UNiosCore_InventoryWidget::ResolveItemDisplayName(FName ItemID) const
{
    if (ItemID.IsNone())
    {
        return FText::GetEmpty();
    }

    if (ResolverSubsystem)
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

int32 UNiosCore_InventoryWidget::GetItemTypeSortPriority(ENiosItemType ItemType) const
{
    switch (ItemType)
    {
    case ENiosItemType::Weapon:
        return 0;
    case ENiosItemType::Armor:
        return 1;
    case ENiosItemType::Usable:
        return 2;
    case ENiosItemType::Misc:
        return 3;
    case ENiosItemType::None:
    default:
        return 99;
    }
}

bool UNiosCore_InventoryWidget::IsFilteredOrSortedViewActive() const
{
    return ItemTypeFilter != ENiosInventoryItemTypeFilter::All ||
        SortMode != ENiosInventorySortMode::Slot ||
        !SearchText.ToString().TrimStartAndEnd().IsEmpty();
}

UNiosCore_ActionSlotWidget* UNiosCore_InventoryWidget::CreateInventorySlotWidget(int32 VisualIndex, int32 InventoryIndex)
{
    if (!SlotWidgetClass || !GridSlots)
        return nullptr;

    UNiosCore_ActionSlotWidget* NewSlot = CreateWidget<UNiosCore_ActionSlotWidget>(GetOwningPlayer(), SlotWidgetClass);
    if (!NewSlot) return nullptr;

    NewSlot->SetSlotIndex(InventoryIndex);
    NewSlot->InteractionProfile = ENiosSlotInteractionProfile::Inventory;
    NewSlot->SetAllowedTypes({ ENiosActionSlotType::Item });

    // �������� ������ �� ��������� ���������
    NewSlot->DefaultSlotSize = SlotSize;

    return NewSlot;
}

void UNiosCore_InventoryWidget::ApplyInventorySlotToWidget(UNiosCore_ActionSlotWidget* SlotWidget, const FNiosInventorySlot& InventorySlot, int32 ItemIndex)
{
    if (!SlotWidget) return;

    if (InventorySlot.IsEmpty())
    {
        SlotWidget->ClearSlot();
        return;
    }

    FNiosActionSlotData Data;
    Data.ActionID = InventorySlot.ItemID;
    Data.Count = InventorySlot.Count;
    Data.Type = ENiosActionSlotType::Item;
    Data.Payload.InventorySlotIndex = ItemIndex;

    SlotWidget->SetSlotData(Data);
    SlotWidget->RefreshVisualDataFromResolver(ResolverSubsystem);
    SlotWidget->SetEquippedState(InventoryComponent ? InventoryComponent->IsItemEquippedAt(ItemIndex) : InventorySlot.bEquipped);
}

FNiosActionSlotData UNiosCore_InventoryWidget::MakeActionSlotDataFromInventorySlot(const FNiosInventorySlot& InventorySlot) const
{
    FNiosActionSlotData SlotData;
    SlotData.Type = ENiosActionSlotType::Item;
    SlotData.ActionID = InventorySlot.ItemID;
    SlotData.Count = InventorySlot.Count;
    SlotData.Payload.InventorySlotIndex = InventorySlot.SlotIndex;
    return SlotData;
}

void UNiosCore_InventoryWidget::BindEquipmentComponent(UNiosCore_EquipmentComponent* InEquipmentComponent)
{
    EquipmentComponent = InEquipmentComponent;
}



void UNiosCore_InventoryWidget::HandleInventoryChanged()
{
    RefreshFromInventory();
}

void UNiosCore_InventoryWidget::HandleInventorySlotChanged(int32 SlotIndex)
{
    if (IsFilteredOrSortedViewActive())
    {
        RefreshFromInventory();
        return;
    }

    RefreshSlot(SlotIndex);
}

UActionSlotResolverSubsystem* UNiosCore_InventoryWidget::ResolveResolverSubsystem()
{
    if (ResolverSubsystem)
        return ResolverSubsystem;

    if (UWorld* World = GetWorld())
        if (UGameInstance* GI = World->GetGameInstance())
            ResolverSubsystem = GI->GetSubsystem<UActionSlotResolverSubsystem>();

    return ResolverSubsystem;
}
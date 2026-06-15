#include "Components/Inventory/NiosCore_InventoryComponent.h"

#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "Data/Resolvers/ActionSlotResolverSubsystem.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "Components/GameplayEvents/NiosCore_GameplayEventRouterComponent.h"
#include "Components/Loot/NiosCore_LootSourceComponent.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UNiosCore_InventoryComponent::UNiosCore_InventoryComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);

}

void UNiosCore_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UNiosCore_InventoryComponent, InventorySlots);
}






UNiosCore_LootSourceComponent* UNiosCore_InventoryComponent::ResolveLootSourceComponent(AActor* LootSourceActor) const
{
    return LootSourceActor ? LootSourceActor->FindComponentByClass<UNiosCore_LootSourceComponent>() : nullptr;
}

bool UNiosCore_InventoryComponent::LootSlotFromSource_Internal(
    AActor* LootSourceActor,
    int32 LootSlotIndex,
    int32 Count,
    int32& OutAddedCount)
{
    OutAddedCount = 0;

    if (!LootSourceActor)
    {
        return false;
    }

    AActor* InventoryOwnerActor = GetOwner();
    if (!InventoryOwnerActor || !InventoryOwnerActor->HasAuthority())
    {
        return false;
    }

    UNiosCore_LootSourceComponent* LootSource = ResolveLootSourceComponent(LootSourceActor);
    if (!LootSource)
    {
        return false;
    }

    return LootSource->LootSlotToInventory(LootSlotIndex, this, Count, OutAddedCount);
}

bool UNiosCore_InventoryComponent::LootAllFromSource_Internal(
    AActor* LootSourceActor,
    int32& OutAddedCount)
{
    OutAddedCount = 0;

    if (!LootSourceActor)
    {
        return false;
    }

    AActor* InventoryOwnerActor = GetOwner();
    if (!InventoryOwnerActor || !InventoryOwnerActor->HasAuthority())
    {
        return false;
    }

    UNiosCore_LootSourceComponent* LootSource = ResolveLootSourceComponent(LootSourceActor);
    if (!LootSource)
    {
        return false;
    }

    return LootSource->LootAllToInventory(this, OutAddedCount);
}

void UNiosCore_InventoryComponent::RequestLootSlotFromSource(
    AActor* LootSourceActor,
    int32 LootSlotIndex,
    int32 Count)
{
    if (!LootSourceActor)
    {
        OnLootRequestFinished.Broadcast(false, 0);
        return;
    }

    AActor* InventoryOwnerActor = GetOwner();
    if (InventoryOwnerActor && InventoryOwnerActor->HasAuthority())
    {
        int32 AddedCount = 0;
        const bool bSuccess = LootSlotFromSource_Internal(LootSourceActor, LootSlotIndex, Count, AddedCount);
        OnLootRequestFinished.Broadcast(bSuccess, AddedCount);
        return;
    }

    Server_LootSlotFromSource(LootSourceActor, LootSlotIndex, Count);
}

void UNiosCore_InventoryComponent::RequestLootAllFromSource(AActor* LootSourceActor)
{
    if (!LootSourceActor)
    {
        OnLootRequestFinished.Broadcast(false, 0);
        return;
    }

    AActor* InventoryOwnerActor = GetOwner();
    if (InventoryOwnerActor && InventoryOwnerActor->HasAuthority())
    {
        int32 AddedCount = 0;
        const bool bSuccess = LootAllFromSource_Internal(LootSourceActor, AddedCount);
        OnLootRequestFinished.Broadcast(bSuccess, AddedCount);
        return;
    }

    Server_LootAllFromSource(LootSourceActor);
}

void UNiosCore_InventoryComponent::Server_LootSlotFromSource_Implementation(
    AActor* LootSourceActor,
    int32 LootSlotIndex,
    int32 Count)
{
    int32 AddedCount = 0;
    const bool bSuccess = LootSlotFromSource_Internal(LootSourceActor, LootSlotIndex, Count, AddedCount);
    Client_LootRequestFinished(bSuccess, AddedCount);
}

void UNiosCore_InventoryComponent::Server_LootAllFromSource_Implementation(AActor* LootSourceActor)
{
    int32 AddedCount = 0;
    const bool bSuccess = LootAllFromSource_Internal(LootSourceActor, AddedCount);
    Client_LootRequestFinished(bSuccess, AddedCount);
}

void UNiosCore_InventoryComponent::Client_LootRequestFinished_Implementation(bool bSuccess, int32 AddedCount)
{
    OnLootRequestFinished.Broadcast(bSuccess, AddedCount);
}

void UNiosCore_InventoryComponent::BeginPlay()
{
    Super::BeginPlay();

    ResolveResolverSubsystem();

    InitializeInventory(MaxSlots);
}

UActionSlotResolverSubsystem* UNiosCore_InventoryComponent::ResolveResolverSubsystem()
{
    if (ResolverSubsystem)
    {
        return ResolverSubsystem;
    }

    UWorld* World = GetWorld();

    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();

    if (!GameInstance)
    {
        return nullptr;
    }

    ResolverSubsystem =
        GameInstance->GetSubsystem<UActionSlotResolverSubsystem>();

    return ResolverSubsystem;
}

void UNiosCore_InventoryComponent::InitializeInventory(int32 InMaxSlots)
{
    MaxSlots = FMath::Max(0, InMaxSlots);

    InventorySlots.Empty();
    InventorySlots.SetNum(MaxSlots);

    FreeSlots.Empty();
    FreeItemInstanceIndices.Empty();
    ItemCacheArray.Empty();

    for (int32 i = MaxSlots - 1; i >= 0; --i)
    {
        InventorySlots[i].Clear();
        InventorySlots[i].SlotIndex = i;

        FreeSlots.Add(i);
        FreeItemInstanceIndices.Add(i);
    }

    OnInventoryChanged.Broadcast();
}

FNiosInventoryItemCache* UNiosCore_InventoryComponent::FindItemCache(FName ItemID)
{
    return ItemCacheArray.FindByPredicate(
        [ItemID](const FNiosInventoryItemCache& Cache)
        {
            return Cache.ItemID == ItemID;
        }
    );
}

int32 UNiosCore_InventoryComponent::GetItemCount(FName ItemID) const
{
    if (ItemID.IsNone())
    {
        return 0;
    }

    const FNiosInventoryItemCache* Cache = ItemCacheArray.FindByPredicate(
        [ItemID](const FNiosInventoryItemCache& Entry)
        {
            return Entry.ItemID == ItemID;
        }
    );

    return Cache ? FMath::Max(0, Cache->Count) : 0;
}

UNiosCore_GameplayEventRouterComponent* UNiosCore_InventoryComponent::ResolveGameplayEventRouter() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    if (UNiosCore_GameplayEventRouterComponent* Router = OwnerActor->FindComponentByClass<UNiosCore_GameplayEventRouterComponent>())
    {
        return Router;
    }

    if (APawn* Pawn = Cast<APawn>(OwnerActor))
    {
        if (APlayerState* PlayerState = Pawn->GetPlayerState())
        {
            return PlayerState->FindComponentByClass<UNiosCore_GameplayEventRouterComponent>();
        }
    }

    return nullptr;
}

void UNiosCore_InventoryComponent::BroadcastInventoryItemCount(FName ItemID)
{
    if (ItemID.IsNone())
    {
        return;
    }

    if (UNiosCore_GameplayEventRouterComponent* Router = ResolveGameplayEventRouter())
    {
        Router->ReportItemCountChanged(ItemID, GetItemCount(ItemID));
    }
}

void UNiosCore_InventoryComponent::BroadcastInventorySnapshot()
{
    TSet<FName> BroadcastedItems;
    for (const FNiosInventoryItemCache& Cache : ItemCacheArray)
    {
        if (!Cache.ItemID.IsNone())
        {
            BroadcastedItems.Add(Cache.ItemID);
            BroadcastInventoryItemCount(Cache.ItemID);
        }
    }

    // Empty inventories intentionally do not broadcast every possible item id.
    // Individual removals broadcast their changed item before/after cache mutation.
}

void UNiosCore_InventoryComponent::RebuildRuntimeCaches()
{
    FreeSlots.Empty();
    FreeItemInstanceIndices.Empty();
    ItemCacheArray.Empty();

    TSet<int32> UsedItemInstanceIndices;

    for (int32 i = InventorySlots.Num() - 1; i >= 0; --i)
    {
        InventorySlots[i].SlotIndex = i;

        if (InventorySlots[i].IsEmpty())
        {
            FreeSlots.Add(i);
        }
        else
        {
            UsedItemInstanceIndices.Add(InventorySlots[i].ItemInstanceIndex);
        }
    }

    for (int32 i = InventorySlots.Num() - 1; i >= 0; --i)
    {
        if (!UsedItemInstanceIndices.Contains(i))
        {
            FreeItemInstanceIndices.Add(i);
        }
    }

    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (!InventorySlots[i].IsEmpty())
        {
            AddSlotToCache(InventorySlots[i].ItemID, i);
        }
    }
}

void UNiosCore_InventoryComponent::AddSlotToCache(
    FName ItemID,
    int32 SlotIndex)
{
    if (ItemID.IsNone())
    {
        return;
    }

    if (!InventorySlots.IsValidIndex(SlotIndex))
    {
        return;
    }

    FNiosInventorySlot& Slot = InventorySlots[SlotIndex];

    if (Slot.IsEmpty())
    {
        return;
    }

    FNiosInventoryItemCache* Cache = FindItemCache(ItemID);

    if (!Cache)
    {
        FNiosInventoryItemCache NewCache;
        NewCache.ItemID = ItemID;
        NewCache.Count = Slot.Count;
        NewCache.SlotIndices.Add(SlotIndex);

        ItemCacheArray.Add(NewCache);
    }
    else
    {
        Cache->Count += Slot.Count;
        Cache->SlotIndices.AddUnique(SlotIndex);
    }
}

void UNiosCore_InventoryComponent::RemoveSlotFromCache(
    FName ItemID,
    int32 SlotIndex,
    int32 RemovedCount,
    bool bSlotBecameEmpty)
{
    if (ItemID.IsNone())
    {
        return;
    }

    if (RemovedCount <= 0)
    {
        return;
    }

    for (int32 i = 0; i < ItemCacheArray.Num(); ++i)
    {
        FNiosInventoryItemCache& Cache = ItemCacheArray[i];

        if (Cache.ItemID != ItemID)
        {
            continue;
        }

        Cache.Count -= RemovedCount;

        if (bSlotBecameEmpty)
        {
            Cache.SlotIndices.Remove(SlotIndex);
        }

        if (Cache.Count <= 0 || Cache.SlotIndices.Num() <= 0)
        {
            ItemCacheArray.RemoveAt(i);
        }

        return;
    }
}

bool UNiosCore_InventoryComponent::AddItem(
    FName ItemID,
    int32 Count,
    int32& OutAddedCount)
{
    OutAddedCount = 0;

    if (ItemID.IsNone())
    {
        return false;
    }

    if (Count <= 0)
    {
        return false;
    }

    UActionSlotResolverSubsystem* Resolver = ResolveResolverSubsystem();

    if (!Resolver)
    {
        return false;
    }

    FNiosResolvedInventoryItemData ItemData;

    if (!Resolver->ResolveInventoryItemData(ItemID, ItemData))
    {
        return false;
    }

    if (!ItemData.bValid)
    {
        return false;
    }

    int32 Remaining = Count;

    const int32 StackLimit = ItemData.bStackable
        ? FMath::Max(1, ItemData.MaxStack)
        : 1;

    FNiosInventoryItemCache* Cache = FindItemCache(ItemID);

    // 1. ������� ���������� � ������������ �����.
    if (Cache && ItemData.bStackable)
    {
        for (int32 SlotIndex : Cache->SlotIndices)
        {
            if (!InventorySlots.IsValidIndex(SlotIndex))
            {
                continue;
            }

            FNiosInventorySlot& Slot = InventorySlots[SlotIndex];

            if (Slot.IsEmpty())
            {
                continue;
            }

            if (Slot.ItemID != ItemID)
            {
                continue;
            }

            const int32 Space = StackLimit - Slot.Count;

            if (Space <= 0)
            {
                continue;
            }

            const int32 ToAdd = FMath::Min(Space, Remaining);

            Slot.Count += ToAdd;
            Cache->Count += ToAdd;

            Remaining -= ToAdd;
            OutAddedCount += ToAdd;

            OnInventorySlotChanged.Broadcast(SlotIndex);

            if (Remaining <= 0)
            {
                OnInventoryChanged.Broadcast();
                BroadcastInventoryItemCount(ItemID);
                return true;
            }
        }
    }

    // 2. ����� ������ ����� ����� � ��������� ������.
    while (Remaining > 0 && FreeSlots.Num() > 0)
    {
        const int32 FreeIndex = FreeSlots.Pop();

        if (!InventorySlots.IsValidIndex(FreeIndex))
        {
            continue;
        }

        FNiosInventorySlot& Slot = InventorySlots[FreeIndex];

        if (!Slot.IsEmpty())
        {
            continue;
        }

        const int32 ToAdd = FMath::Min(Remaining, StackLimit);

        Slot.ItemInstanceIndex = AllocateItemInstanceIndex();
        Slot.ItemID = ItemID;
        Slot.Count = ToAdd;
        Slot.SlotIndex = FreeIndex;

        AddSlotToCache(ItemID, FreeIndex);

        Remaining -= ToAdd;
        OutAddedCount += ToAdd;

        OnInventorySlotChanged.Broadcast(FreeIndex);
    }

    if (OutAddedCount > 0)
    {
        OnInventoryChanged.Broadcast();
        BroadcastInventoryItemCount(ItemID);
        return true;
    }

    return false;
}

bool UNiosCore_InventoryComponent::RemoveItemAt(
    int32 SlotIndex,
    int32 Count)
{
    if (!InventorySlots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    if (Count <= 0)
    {
        return false;
    }

    FNiosInventorySlot& Slot = InventorySlots[SlotIndex];

    if (Slot.IsEmpty())
    {
        return false;
    }

    const FName RemovedItemID = Slot.ItemID;
    const int32 RemovedItemInstanceIndex = Slot.ItemInstanceIndex;
    const int32 ToRemove = FMath::Min(Count, Slot.Count);

    Slot.Count -= ToRemove;

    const bool bSlotBecameEmpty = Slot.Count <= 0;

    RemoveSlotFromCache(
        RemovedItemID,
        SlotIndex,
        ToRemove,
        bSlotBecameEmpty
    );

    if (bSlotBecameEmpty)
    {
        if (UNiosCore_EquipmentComponent* EquipmentComponent = ResolveEquipmentComponent())
        {
            if (EquipmentComponent->IsItemInstanceEquipped(RemovedItemInstanceIndex))
            {
                EquipmentComponent->UnequipItemInstance(RemovedItemInstanceIndex);
            }
        }

        ReleaseItemInstanceIndex(RemovedItemInstanceIndex);

        Slot.Clear();
        Slot.SlotIndex = SlotIndex;

        FreeSlots.AddUnique(SlotIndex);
    }

    OnInventorySlotChanged.Broadcast(SlotIndex);
    OnInventoryChanged.Broadcast();
    BroadcastInventoryItemCount(RemovedItemID);

    return true;
}

bool UNiosCore_InventoryComponent::RemoveItemById(
    FName ItemID,
    int32 Count,
    int32& OutRemovedCount)
{
    OutRemovedCount = 0;

    if (ItemID.IsNone() || Count <= 0)
    {
        return false;
    }

    int32 Remaining = Count;

    while (Remaining > 0)
    {
        int32 MatchingSlotIndex = INDEX_NONE;
        int32 MatchingSlotCount = 0;

        for (int32 SlotIndex = 0; SlotIndex < InventorySlots.Num(); ++SlotIndex)
        {
            const FNiosInventorySlot& Slot = InventorySlots[SlotIndex];
            if (!Slot.IsEmpty() && Slot.ItemID == ItemID && Slot.Count > 0)
            {
                MatchingSlotIndex = SlotIndex;
                MatchingSlotCount = Slot.Count;
                break;
            }
        }

        if (MatchingSlotIndex == INDEX_NONE)
        {
            break;
        }

        const int32 ToRemove = FMath::Min(Remaining, MatchingSlotCount);
        if (!RemoveItemAt(MatchingSlotIndex, ToRemove))
        {
            break;
        }

        Remaining -= ToRemove;
        OutRemovedCount += ToRemove;
    }

    if (OutRemovedCount > 0)
    {
        BroadcastInventoryItemCount(ItemID);
    }

    return OutRemovedCount > 0;
}

void UNiosCore_InventoryComponent::HandleUseSlot(FNiosInventorySlot& Slot)
{
    if (Slot.IsEmpty())
    {
        return;
    }

    UActionSlotResolverSubsystem* Resolver = ResolveResolverSubsystem();

    if (!Resolver)
    {
        return;
    }

    FNiosResolvedInventoryItemData ItemData;

    if (!Resolver->ResolveInventoryItemData(Slot.ItemID, ItemData))
    {
        return;
    }

    if (!ItemData.bValid)
    {
        return;
    }

    switch (ItemData.ItemType)
    {
    case ENiosItemType::Usable:
    {
        UNiosCore_AbilitySystemComponent* ASC = ResolveASC();
        if (!ASC)
        {
            return;
        }

        if (ItemData.AbilityData)
        {
            ASC->PreloadSkillAssets(ItemData.AbilityData.Get());
        }

        if (!ItemData.SkillID.IsNone())
        {
            const bool bActivated =
                ASC->TryActivateSkillByID(ItemData.SkillID);

            if (bActivated && ItemData.bConsumeOnUse)
            {
                RemoveItemAt(
                    Slot.SlotIndex,
                    FMath::Max(1, ItemData.ConsumeCount)
                );
            }
        }

        break;
    }

    case ENiosItemType::Weapon:
    case ENiosItemType::Armor:
    {
        if (UNiosCore_EquipmentComponent* EquipmentComponent = ResolveEquipmentComponent())
        {
            EquipmentComponent->AutoEquipItemFromInventorySlot(Slot.SlotIndex);
        }
        break;
    }

    default:
    {
        break;
    }
    }
}

bool UNiosCore_InventoryComponent::UseItemAt(int32 SlotIndex)
{
    if (!InventorySlots.IsValidIndex(SlotIndex))
    {
        return false;
    }

    if (InventorySlots[SlotIndex].IsEmpty())
    {
        return false;
    }

    HandleUseSlot(InventorySlots[SlotIndex]);

    return true;
}

void UNiosCore_InventoryComponent::Server_UseItemAt_Implementation(
    int32 SlotIndex)
{
    UseItemAt(SlotIndex);
}

void UNiosCore_InventoryComponent::OnRep_InventorySlots()
{
    RebuildRuntimeCaches();
    OnInventoryChanged.Broadcast();
    BroadcastInventorySnapshot();
}

int32 UNiosCore_InventoryComponent::AllocateItemInstanceIndex()
{
    if (FreeItemInstanceIndices.Num() > 0)
    {
        return FreeItemInstanceIndices.Pop();
    }

    int32 Candidate = 0;
    while (FindSlotIndexByItemInstanceIndex(Candidate) != INDEX_NONE)
    {
        ++Candidate;
    }

    return Candidate;
}

void UNiosCore_InventoryComponent::ReleaseItemInstanceIndex(int32 ItemInstanceIndex)
{
    if (ItemInstanceIndex != INDEX_NONE)
    {
        FreeItemInstanceIndices.AddUnique(ItemInstanceIndex);
        FreeItemInstanceIndices.Sort([](const int32 A, const int32 B) { return A > B; });
    }
}

int32 UNiosCore_InventoryComponent::FindSlotIndexByItemInstanceIndex(int32 ItemInstanceIndex) const
{
    if (ItemInstanceIndex == INDEX_NONE)
    {
        return INDEX_NONE;
    }

    for (int32 i = 0; i < InventorySlots.Num(); ++i)
    {
        if (!InventorySlots[i].IsEmpty() && InventorySlots[i].ItemInstanceIndex == ItemInstanceIndex)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

void UNiosCore_InventoryComponent::SetItemEquippedState(int32 ItemInstanceIndex, bool bEquipped)
{
    const int32 SlotIndex = FindSlotIndexByItemInstanceIndex(ItemInstanceIndex);
    if (!InventorySlots.IsValidIndex(SlotIndex))
    {
        return;
    }

    FNiosInventorySlot& Slot = InventorySlots[SlotIndex];
    if (Slot.bEquipped == bEquipped)
    {
        return;
    }

    Slot.bEquipped = bEquipped;
    OnInventorySlotChanged.Broadcast(SlotIndex);
}

bool UNiosCore_InventoryComponent::IsItemEquippedAt(int32 SlotIndex) const
{
    if (!InventorySlots.IsValidIndex(SlotIndex) || InventorySlots[SlotIndex].IsEmpty())
    {
        return false;
    }

    const UNiosCore_EquipmentComponent* EquipmentComponent = ResolveEquipmentComponent();
    return EquipmentComponent && EquipmentComponent->IsItemInstanceEquipped(InventorySlots[SlotIndex].ItemInstanceIndex);
}

UNiosCore_EquipmentComponent* UNiosCore_InventoryComponent::ResolveEquipmentComponent() const
{
    AActor* Owner = GetOwner();
    return Owner ? Owner->FindComponentByClass<UNiosCore_EquipmentComponent>() : nullptr;
}

UNiosCore_AbilitySystemComponent* UNiosCore_InventoryComponent::ResolveASC() const
{
    AActor* Owner = GetOwner();

    if (!Owner)
    {
        return nullptr;
    }

    return Owner->FindComponentByClass<UNiosCore_AbilitySystemComponent>();
}
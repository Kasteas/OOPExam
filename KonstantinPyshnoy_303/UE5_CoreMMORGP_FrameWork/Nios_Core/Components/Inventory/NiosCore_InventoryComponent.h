#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Items/NiosCore_ItemDataTypes.h"
#include "Net/UnrealNetwork.h"
#include "NiosCore_InventoryComponent.generated.h"

class UActionSlotResolverSubsystem;
class UNiosCore_AbilitySystemComponent;
class UNiosCore_EquipmentComponent;
class UNiosCore_GameplayEventRouterComponent;
class UNiosCore_LootSourceComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNiosInventoryChanged);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosInventorySlotChanged,
    int32,
    SlotIndex
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosInventoryLootRequestFinished,
    bool,
    bSuccess,
    int32,
    AddedCount
);

UCLASS(ClassGroup = (Nios),
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_InventoryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_InventoryComponent();

protected:
    virtual void BeginPlay() override;

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void InitializeInventory(int32 InMaxSlots);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    bool AddItem(FName ItemID, int32 Count, int32& OutAddedCount);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    bool RemoveItemAt(int32 SlotIndex, int32 Count);

    /** Removes Count items by ItemID across inventory slots. Server-authoritative helper used by quest turn-in flows. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    bool RemoveItemById(FName ItemID, int32 Count, int32& OutRemovedCount);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    bool UseItemAt(int32 SlotIndex);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Inventory")
    bool IsItemEquippedAt(int32 SlotIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Inventory")
    int32 FindSlotIndexByItemInstanceIndex(int32 ItemInstanceIndex) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Inventory")
    int32 GetItemCount(FName ItemID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Quest Events")
    void BroadcastInventoryItemCount(FName ItemID);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Quest Events")
    void BroadcastInventorySnapshot();

    /** Mirrors EquipmentComponent state into the inventory slot for simple UI display. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void SetItemEquippedState(int32 ItemInstanceIndex, bool bEquipped);

    UFUNCTION(Server, Reliable)
    void Server_UseItemAt(int32 SlotIndex);

    /** Client-safe request: loot one slot from a chest/corpse/loot source actor. Count <= 0 means whole stack. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Loot")
    void RequestLootSlotFromSource(AActor* LootSourceActor, int32 LootSlotIndex, int32 Count = 0);

    /** Client-safe request: loot every available item from a chest/corpse/loot source actor. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Loot")
    void RequestLootAllFromSource(AActor* LootSourceActor);

    UFUNCTION(Server, Reliable)
    void Server_LootSlotFromSource(AActor* LootSourceActor, int32 LootSlotIndex, int32 Count);

    UFUNCTION(Server, Reliable)
    void Server_LootAllFromSource(AActor* LootSourceActor);

    UFUNCTION(Client, Reliable)
    void Client_LootRequestFinished(bool bSuccess, int32 AddedCount);


protected:
    void HandleUseSlot(FNiosInventorySlot& Slot);

    FNiosInventoryItemCache* FindItemCache(FName ItemID);

    void RebuildRuntimeCaches();

    void AddSlotToCache(FName ItemID, int32 SlotIndex);

    void RemoveSlotFromCache(
        FName ItemID,
        int32 SlotIndex,
        int32 RemovedCount,
        bool bSlotBecameEmpty
    );

    UNiosCore_AbilitySystemComponent* ResolveASC() const;

    UNiosCore_EquipmentComponent* ResolveEquipmentComponent() const;

    UNiosCore_GameplayEventRouterComponent* ResolveGameplayEventRouter() const;

    bool LootSlotFromSource_Internal(AActor* LootSourceActor, int32 LootSlotIndex, int32 Count, int32& OutAddedCount);

    bool LootAllFromSource_Internal(AActor* LootSourceActor, int32& OutAddedCount);

    UNiosCore_LootSourceComponent* ResolveLootSourceComponent(AActor* LootSourceActor) const;

    UActionSlotResolverSubsystem* ResolveResolverSubsystem();

    int32 AllocateItemInstanceIndex();

    void ReleaseItemInstanceIndex(int32 ItemInstanceIndex);

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Inventory")
    int32 MaxSlots = 30;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_InventorySlots, Category = "Nios|Inventory")
    TArray<FNiosInventorySlot> InventorySlots;

    UFUNCTION()
    void OnRep_InventorySlots();

    UPROPERTY(BlueprintAssignable, Category = "Nios|Inventory")
    FNiosInventoryChanged OnInventoryChanged;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Inventory")
    FNiosInventorySlotChanged OnInventorySlotChanged;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Inventory|Loot")
    FNiosInventoryLootRequestFinished OnLootRequestFinished;


protected:
    UPROPERTY()
    TArray<int32> FreeSlots;

    UPROPERTY()
    TArray<int32> FreeItemInstanceIndices;

    UPROPERTY()
    TArray<FNiosInventoryItemCache> ItemCacheArray;

    UPROPERTY(Transient)
    TObjectPtr<UActionSlotResolverSubsystem> ResolverSubsystem = nullptr;
};
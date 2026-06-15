#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Items/NiosCore_ItemDataTypes.h"
#include "NiosCore_EquipmentComponent.generated.h"

class UActionSlotResolverSubsystem;
class UNiosCore_InventoryComponent;
class UNiosCore_StatsComponent;
class UNiosCore_StatusEffectComponent;
class UStaticMesh;
class USkeletalMesh;



UENUM(BlueprintType)
enum class ENiosWeaponReadyState : uint8
{
    None UMETA(DisplayName = "None"),
    Sheathed UMETA(DisplayName = "Sheathed"),
    Drawing UMETA(DisplayName = "Drawing"),
    InHands UMETA(DisplayName = "In Hands"),
    Sheathing UMETA(DisplayName = "Sheathing")
};

UENUM(BlueprintType)
enum class ENiosWeaponVisualSocketState : uint8
{
    None UMETA(DisplayName = "None"),
    Back UMETA(DisplayName = "Back"),
    Hip UMETA(DisplayName = "Hip"),
    Hand UMETA(DisplayName = "Hand"),
    RangedBack UMETA(DisplayName = "Ranged Back")
};

USTRUCT(BlueprintType)
struct FNiosWeaponVisualState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ENiosWeaponReadyState ReadyState = ENiosWeaponReadyState::None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    ENiosWeaponVisualSocketState SocketState = ENiosWeaponVisualSocketState::None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EEquipSlot ActiveWeaponSlot = EEquipSlot::None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    FName ItemID = NAME_None;

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 ItemInstanceIndex = INDEX_NONE;

    FORCEINLINE bool HasWeapon() const
    {
        return !ItemID.IsNone() && ActiveWeaponSlot != EEquipSlot::None;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEquipmentChanged, EEquipSlot, Slot);

/** Fired when a single equipment slot should refresh its attached visual mesh/components. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosEquipmentVisualSlotChanged,
    EEquipSlot, ChangedSlot,
    FReplicatedEquipmentSlot, EquipmentSlotData
);

/** Fired when all equipment visuals should be rebuilt, usually after replication/load. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosEquipmentVisualsChanged,
    TArray<FReplicatedEquipmentSlot>, InEquippedItems
);

/** Fired after C++ async-loads the visual mesh for a slot. BP receives ready-to-use mesh pointers.
 * If both meshes are null, the slot visual should be cleared. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
    FNiosEquipmentMeshReady,
    EEquipSlot, Slot,
    FReplicatedEquipmentSlot, EquipmentSlotData,
    UStaticMesh*, StaticMesh,
    USkeletalMesh*, SkeletalMesh
);

/** Fired when persistent weapon ready/socket state changes. Use this for debug UI and BP visual sync. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosWeaponVisualStateChanged,
    ENiosWeaponReadyState, NewReadyState,
    FNiosWeaponVisualState, VisualState
);

/** Fired when BP should attach currently equipped weapon visual to a socket category. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FNiosWeaponAttachRequested,
    ENiosWeaponVisualSocketState, SocketState,
    FReplicatedEquipmentSlot, EquipmentSlotData,
    FNiosWeaponVisualState, VisualState
);


UCLASS(ClassGroup = (Nios),
    BlueprintType,
    Blueprintable,
    meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_EquipmentComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_EquipmentComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Реплицируемый массив слотов экипировки. Компонент является source of truth. */
    UPROPERTY(ReplicatedUsing = OnRep_EquipmentChanged)
    TArray<FReplicatedEquipmentSlot> EquippedItems;

    /** Persistent replicated weapon visual/readiness state. Rarely changes, but late-join clients need it. */
    UPROPERTY(ReplicatedUsing = OnRep_WeaponVisualState, BlueprintReadOnly, Category = "Nios|Equipment|Weapon")
    FNiosWeaponVisualState WeaponVisualState;



    UPROPERTY(BlueprintAssignable)
    FOnEquipmentChanged OnEquipmentChanged;

    /** Use this in Character BP to update only one visual slot. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment|Visuals")
    FNiosEquipmentVisualSlotChanged OnEquipmentVisualSlotChanged;

    /** Use this in Character BP to rebuild all equipment visuals on initial load/OnRep. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment|Visuals")
    FNiosEquipmentVisualsChanged OnEquipmentVisualsChanged;

    /** C++ async-loads equipment mesh and gives BP ready UStaticMesh/USkeletalMesh pointers. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment|Visuals")
    FNiosEquipmentMeshReady OnEquipmentMeshReady;

    /** Bind in BP/Character to update debug widgets or trigger visual sync after OnRep/server state changes. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment|Weapon")
    FNiosWeaponVisualStateChanged OnWeaponVisualStateChanged;

    /** Bind in BP/Character to attach/snap weapon mesh to hand/back/hip sockets. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment|Weapon")
    FNiosWeaponAttachRequested OnWeaponAttachRequested;

    /** Явно связать экипировку с инвентарём. Если не вызвать, компонент попробует найти InventoryComponent на Owner. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment")
    void InitializeInventoryComponent(UNiosCore_InventoryComponent* InInventoryComponent);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment")
    UNiosCore_InventoryComponent* GetInventoryComponent() const;

    /** Надеть предмет в первый подходящий слот. Старый API оставлен для совместимости, но без inventory reference. */
    UFUNCTION(BlueprintCallable)
    bool EquipItem(const FName ItemID);

    /** Надеть предмет в конкретный слот экипировки. Старый API оставлен для совместимости, но без inventory reference. */
    UFUNCTION(BlueprintCallable)
    bool EquipItemToSlot(const FName ItemID, EEquipSlot TargetSlot);

    /** Надеть конкретный экземпляр предмета из инвентаря в первый подходящий слот. */
    UFUNCTION(BlueprintCallable)
    bool EquipInventoryItem(int32 ItemInstanceIndex);

    /** Надеть конкретный экземпляр предмета из инвентаря в конкретный слот экипировки. */
    UFUNCTION(BlueprintCallable)
    bool EquipInventoryItemToSlot(int32 ItemInstanceIndex, EEquipSlot TargetSlot);

    /** Надеть предмет из конкретного inventory slot в первый подходящий/свободный слот. Удобно для ПКМ/Use.
     *  Client-safe: on owning clients this sends the authoritative server request and does not mutate local state. */
    UFUNCTION(BlueprintCallable)
    bool AutoEquipItemFromInventorySlot(int32 InventorySlotIndex);

    UFUNCTION(Server, Reliable)
    void Server_AutoEquipItemFromInventorySlot(int32 InventorySlotIndex);

    /** Надеть предмет из конкретного inventory slot в конкретный equipment slot. Удобно для drag/drop из UI.
     *  Client-safe: on owning clients this sends the authoritative server request and does not mutate local state. */
    UFUNCTION(BlueprintCallable)
    bool EquipItemFromInventorySlot(int32 InventorySlotIndex, EEquipSlot TargetSlot);

    UFUNCTION(Server, Reliable)
    void Server_EquipItemFromInventorySlot(int32 InventorySlotIndex, EEquipSlot TargetSlot);

    /** То же самое, но одной структурой для Messages / Blueprint payload. */
    UFUNCTION(BlueprintCallable)
    bool EquipItemFromInventorySlotRequest(const FNiosEquipmentSlotRequest& Request);

    /** Можно ли сейчас менять экипировку. False while the owning gameplay actor is dead. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment")
    bool CanModifyEquipmentNow() const;

    /** Можно ли надеть предмет в конкретный слот. */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool CanEquipItemToSlot(const FName ItemID, EEquipSlot TargetSlot) const;

    /** Надет ли конкретный экземпляр предмета. */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsItemInstanceEquipped(int32 ItemInstanceIndex) const;

    /** Надет ли предмет, лежащий в inventory slot. */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    bool IsInventorySlotEquipped(int32 InventorySlotIndex) const;

    /** Найти inventory slot по ItemInstanceIndex. Возвращает INDEX_NONE, если экземпляр не найден. */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    int32 ResolveInventorySlotIndexForItemInstance(int32 ItemInstanceIndex) const;

    /** Снять предмет по ItemID. Старый API: снимет все совпадения по шаблону. */
    UFUNCTION(BlueprintCallable)
    bool UnequipItem(const FName ItemID);

    /** Снять конкретный экземпляр предмета. */
    UFUNCTION(BlueprintCallable)
    bool UnequipItemInstance(int32 ItemInstanceIndex);

    /** Снять предмет из конкретного слота.
     *  Client-safe: on owning clients this sends the authoritative server request and does not mutate local state. */
    UFUNCTION(BlueprintCallable)
    bool UnequipSlot(EEquipSlot Slot);

    UFUNCTION(Server, Reliable)
    void Server_UnequipSlot(EEquipSlot Slot);

    /** Получить состояние конкретного слота. */
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FReplicatedEquipmentSlot GetEquipmentSlot(EEquipSlot Slot) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment")
    bool GetEquippedSlotByItemInstance(
        int32 ItemInstanceIndex,
        FReplicatedEquipmentSlot& OutEquipmentSlot
    ) const;
    /** Проверить все equipped ссылки и снять предметы, которых уже нет в инвентаре. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment")
    void ValidateEquipmentAgainstInventory();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment")
    TArray<FReplicatedEquipmentSlot> GetEquippedItems() const;

    /** True for weapon types that occupy both hand slots using the same ItemInstanceIndex. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment")
    bool IsTwoHandedWeaponType(ENiosWeaponType WeaponType) const;

    /** True for shields, orbs, torches and other secondary left-hand utility items. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment")
    bool IsOffHandWeaponType(ENiosWeaponType WeaponType) const;

    /** Rebuilds equipment-granted status effects after a death/revive cleanup pass. Server only. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Lifecycle")
    void RefreshEquipmentStatusEffectsAfterRevive();

    /** Manually request a full visuals rebuild from BP, useful after binding events late. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Visuals")
    void BroadcastEquipmentVisualsChanged();

    /** Manually request a single visual slot refresh from BP. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Visuals")
    void BroadcastEquipmentVisualSlotChanged(EEquipSlot Slot);


    /** Async-load and broadcast ready visual mesh for one slot. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Visuals")
    void RequestEquipmentMeshForSlot(EEquipSlot Slot);

    /** Async-load and broadcast ready visual meshes for all known equipment slots. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Visuals")
    void RequestAllEquipmentMeshes();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Weapon")
    FNiosWeaponVisualState GetWeaponVisualState() const { return WeaponVisualState; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Weapon")
    bool IsWeaponInHands() const { return WeaponVisualState.ReadyState == ENiosWeaponReadyState::InHands; }

    /** True if any equipped weapon exists and optionally matches one of RequiredWeaponTypes. Empty array means any weapon. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Weapon")
    bool HasEquippedWeaponMatchingTypes(const TArray<ENiosWeaponType>& RequiredWeaponTypes) const;

    /** True if the active/required weapon is in hands. Empty array means any weapon. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Weapon")
    bool IsWeaponInHandsMatchingTypes(const TArray<ENiosWeaponType>& RequiredWeaponTypes) const;

    /** Resolve the preferred equipped weapon slot for a requirement. Empty RequiredWeaponTypes means current active/default weapon. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Weapon")
    bool ResolveWeaponSlotMatchingTypes(const TArray<ENiosWeaponType>& RequiredWeaponTypes, EEquipSlot& OutSlot, FReplicatedEquipmentSlot& OutSlotData) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Weapon")
    ENiosWeaponVisualSocketState GetDefaultSheathedSocketForSlot(EEquipSlot Slot) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Weapon")
    FReplicatedEquipmentSlot GetActiveWeaponEquipmentSlot() const;

    /** Server-authoritative state change. Use from DrawWeapon/SheatheWeapon ability. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Weapon")
    void SetWeaponVisualState(ENiosWeaponReadyState NewReadyState, ENiosWeaponVisualSocketState NewSocketState, EEquipSlot ActiveWeaponSlot = EEquipSlot::None);

    UFUNCTION(Server, Reliable)
    void Server_SetWeaponVisualState(ENiosWeaponReadyState NewReadyState, ENiosWeaponVisualSocketState NewSocketState, EEquipSlot ActiveWeaponSlot);

    /** Convenience functions for AnimNotify/BP debug buttons. They request socket sync without changing equipment. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Weapon")
    void AttachWeaponVisualToHand();

    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Weapon")
    void AttachWeaponVisualToBack();

    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Weapon")
    void AttachWeaponVisualToHip();

    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|Weapon")
    void AttachWeaponVisualToCurrentSocket();

    /** BP override point for quick debug/prototype snapping. Also broadcast through OnWeaponAttachRequested. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Equipment|Weapon")
    void BP_OnWeaponAttachRequested(ENiosWeaponVisualSocketState SocketState, FReplicatedEquipmentSlot EquipmentSlotData, FNiosWeaponVisualState VisualState);

    /** BP override point for state UI/debug. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Equipment|Weapon")
    void BP_OnWeaponVisualStateChanged(ENiosWeaponReadyState NewReadyState, FNiosWeaponVisualState VisualState);

    /** BP helper: resolve weapon table row by item id, e.g. to check WeaponType == TwoHanded. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Data")
    bool GetWeaponRowForItem(FName ItemID, FNiosWeaponRow& OutWeaponRow) const;

    /** BP helper: resolve armor table row by item id, e.g. to check ArmorSlot. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Equipment|Data")
    bool GetArmorRowForItem(FName ItemID, FNiosArmorRow& OutArmorRow) const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_InventoryComponent> InventoryComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_StatsComponent> StatsComponent = nullptr;

    UFUNCTION()
    void OnRep_EquipmentChanged();

    UFUNCTION()
    void OnRep_WeaponVisualState();

    UFUNCTION()
    void HandleLinkedInventoryChanged();

    UFUNCTION()
    void HandleLinkedInventorySlotChanged(int32 SlotIndex);

    void SetSlot(EEquipSlot Slot, const FReplicatedEquipmentSlot& Equipment);
    int32 FindSlotIndex(EEquipSlot Slot) const;

    UActionSlotResolverSubsystem* ResolveResolverSubsystem() const;

    UNiosCore_InventoryComponent* ResolveInventoryComponent() const;
    UNiosCore_StatsComponent* ResolveStatsComponent() const;
    UNiosCore_StatusEffectComponent* ResolveStatusEffectComponent() const;
    void BindInventoryEvents(UNiosCore_InventoryComponent* InInventoryComponent);
    void UnbindInventoryEvents();
    bool ResolveItemData(FName ItemID, FNiosResolvedInventoryItemData& OutItemData) const;
    const FNiosInventorySlot* FindInventorySlotByItemInstanceIndex(int32 ItemInstanceIndex) const;
    bool EquipInventoryItemToSlotFromData(const FName ItemID, int32 ItemInstanceIndex, EEquipSlot TargetSlot);
    bool ResolveWeaponRow(FName ItemID, FNiosWeaponRow& OutWeaponRow) const;
    bool ResolveArmorRow(FName ItemID, FNiosArmorRow& OutArmorRow) const;

    EEquipSlot FindDefaultSlotForItem(FName ItemID) const;
    bool IsArmorSlotCompatible(ENiosArmorSlot ArmorSlot, EEquipSlot TargetSlot) const;
    bool IsWeaponSlotCompatible(ENiosWeaponType WeaponType, EEquipSlot TargetSlot) const;
    bool IsOffHandArmorSlot(ENiosArmorSlot ArmorSlot) const;
    bool IsArtifactSlot(EEquipSlot Slot) const;
    bool IsHandSlot(EEquipSlot Slot) const;

    void ClearSlotInternal(EEquipSlot Slot);
    void NotifyInventorySlotForItemInstanceChanged(int32 ItemInstanceIndex) const;

    FName MakeEquipmentStatsSourceID(int32 ItemInstanceIndex) const;
    FName MakeEquipmentStatusEffectSourceID(int32 ItemInstanceIndex) const;
    void RemoveEquipmentStatsForItemInstance(int32 ItemInstanceIndex) const;
    void RemoveEquipmentStatusEffectsForItemInstance(int32 ItemInstanceIndex) const;
    void AddEquipmentStatsForSlot(const FReplicatedEquipmentSlot& EquipmentSlot) const;
    void AddEquipmentStatusEffectsForSlot(const FReplicatedEquipmentSlot& EquipmentSlot) const;
    void RebuildEquipmentStats() const;
    void RebuildEquipmentStatusEffects() const;
    bool IsValidEquipmentGrantedStatusEffect(FName EffectID) const;

    void BroadcastAllSlotsChanged();
    void BroadcastAllVisualSlotsChanged();

    EEquipSlot ResolveDefaultActiveWeaponSlot() const;
    void SyncWeaponVisualStateWithEquipment();
    void ApplyWeaponVisualState(const FNiosWeaponVisualState& NewState, bool bBroadcastAttach);
    void BroadcastWeaponVisualStateChanged(bool bBroadcastAttach);
};

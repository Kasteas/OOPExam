#pragma once

#include "CoreMinimal.h"
#include "UI/ActionSlots/NiosCore_ActionSlotWidget.h"
#include "Data/Items/NiosCore_ItemDataTypes.h"
#include "NiosCore_EquipmentSlotWidget.generated.h"

class UNiosCore_EquipmentComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosEquipmentSlotDropRequested,
    FNiosEquipmentSlotRequest,
    Request
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosEquipmentSlotUnequipRequested,
    EEquipSlot,
    Slot
);

UCLASS()
class NIOS_CORE_API UNiosCore_EquipmentSlotWidget : public UNiosCore_ActionSlotWidget
{
    GENERATED_BODY()

public:
    /** Инициализация слота для компонента экипировки. */
    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void InitializeEquipmentSlot(EEquipSlot InSlot, UNiosCore_EquipmentComponent* InComponent);

    UFUNCTION(BlueprintCallable, Category = "Equipment")
    void SetOccupied(bool bOccupied);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
    bool IsOccupied() const;

    /** True when this visual slot is showing a two-handed weapon mirrored from the other hand. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
    bool IsDisplayingTwoHandedMirror() const;

    /** Equipment slot type represented by this widget. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
    EEquipSlot GetEquipmentSlotType() const;

    /** Called by drag-drop operation when an equipped item drag is cancelled outside any drop target. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|DragDrop")
    bool RequestUnequipFromDragCancelled();

    /** UI request: drag/drop на конкретный equipment slot. Можно поймать в BP и отправить RPC. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment")
    FNiosEquipmentSlotDropRequested OnEquipmentSlotDropRequested;

    /** UI request: предмет из equipment slot был брошен в пустоту. Обычно это Unequip. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment")
    FNiosEquipmentSlotUnequipRequested OnEquipmentSlotUnequipRequested;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    UNiosCore_EquipmentComponent* EquipmentComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    EEquipSlot SlotType = EEquipSlot::None;

    virtual bool NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation
    ) override;

    virtual void NativeOnDragCancelled(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation
    ) override;

    void UpdateVisualFromEquipment();

    bool ResolveDisplayEquipmentSlot(FReplicatedEquipmentSlot& OutSlot, bool& bOutTwoHandedMirror) const;
    /** Returns the real authoritative hand slot for interactions when this widget is only rendering a two-handed mirror. */
    EEquipSlot ResolveInteractionEquipmentSlot() const;
    bool IsTwoHandedWeaponItem(FName ItemID) const;
    void SetTwoHandedMirrorDisplayState(bool bNewMirrored);

    /** Called when this slot starts/stops rendering the mirrored half of a two-handed weapon. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Equipment")
    void BP_OnTwoHandedMirrorDisplayChanged(bool bIsMirrored);

private:
    UPROPERTY()
    bool bSlotOccupied = false;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
    bool bDisplayingTwoHandedMirror = false;
};

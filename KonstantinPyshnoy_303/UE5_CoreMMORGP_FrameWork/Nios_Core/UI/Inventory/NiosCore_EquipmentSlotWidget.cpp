#include "UI/Inventory/NiosCore_EquipmentSlotWidget.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "UI/ActionSlots/NiosCore_DragVisualWidget.h" 
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/ActionSlots/Operations/NiosActionSlotDragDropOperation.h" 

void UNiosCore_EquipmentSlotWidget::InitializeEquipmentSlot(EEquipSlot InSlot, UNiosCore_EquipmentComponent* InComponent)
{
    SlotType = InSlot;
    EquipmentComponent = InComponent;
    UpdateVisualFromEquipment();
}

void UNiosCore_EquipmentSlotWidget::SetOccupied(bool bOccupied)
{
    bSlotOccupied = bOccupied;
}

bool UNiosCore_EquipmentSlotWidget::IsOccupied() const
{
    return bSlotOccupied;
}

bool UNiosCore_EquipmentSlotWidget::IsDisplayingTwoHandedMirror() const
{
    return bDisplayingTwoHandedMirror;
}

EEquipSlot UNiosCore_EquipmentSlotWidget::GetEquipmentSlotType() const
{
    return SlotType;
}

bool UNiosCore_EquipmentSlotWidget::RequestUnequipFromDragCancelled()
{
    // Dragging an equipped item out of every valid drop target means "unequip".
    // Do not use IsEmpty() here: two-handed mirror display and some UI refresh paths can
    // make visual data look empty/non-empty differently from the replicated equipment slot.
    const EEquipSlot InteractionSlot = ResolveInteractionEquipmentSlot();
    if (InteractionSlot == EEquipSlot::None)
    {
        return false;
    }

    if (OnEquipmentSlotUnequipRequested.IsBound())
    {
        OnEquipmentSlotUnequipRequested.Broadcast(InteractionSlot);
        return true;
    }

    if (!EquipmentComponent)
    {
        return false;
    }

    const bool bSuccess = EquipmentComponent->UnequipSlot(InteractionSlot);
    if (bSuccess)
    {
        UpdateVisualFromEquipment();
    }

    return bSuccess;
}

bool UNiosCore_EquipmentSlotWidget::NativeOnDrop(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    const UNiosActionSlotDragDropOperation* DragOp = Cast<UNiosActionSlotDragDropOperation>(InOperation);
    if (!DragOp || DragOp->SlotData.IsEmpty() || !EquipmentComponent)
        return false;

    if (DragOp->SlotData.Type != ENiosActionSlotType::Item)
        return false;

    const int32 InventorySlotIndex = DragOp->SlotData.Payload.InventorySlotIndex;
    if (InventorySlotIndex == INDEX_NONE)
        return false;

    FNiosEquipmentSlotRequest Request;
    Request.InventorySlotIndex = InventorySlotIndex;
    Request.TargetSlot = SlotType;

    if (OnEquipmentSlotDropRequested.IsBound())
    {
        OnEquipmentSlotDropRequested.Broadcast(Request);
        return true;
    }

    const bool bSuccess = EquipmentComponent->EquipItemFromInventorySlotRequest(Request);
    if (bSuccess)
        UpdateVisualFromEquipment();

    return bSuccess;
}


void UNiosCore_EquipmentSlotWidget::NativeOnDragCancelled(
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
    RequestUnequipFromDragCancelled();
}

void UNiosCore_EquipmentSlotWidget::UpdateVisualFromEquipment()
{
    if (!EquipmentComponent)
    {
        SetOccupied(false);
        SetTwoHandedMirrorDisplayState(false);
        ClearSlot();
        return;
    }

    FReplicatedEquipmentSlot DisplaySlot;
    bool bTwoHandedMirror = false;
    if (!ResolveDisplayEquipmentSlot(DisplaySlot, bTwoHandedMirror) || DisplaySlot.ItemID.IsNone())
    {
        SetOccupied(false);
        SetTwoHandedMirrorDisplayState(false);
        ClearSlot();
        return;
    }

    FNiosActionSlotData Data;
    Data.ActionID = DisplaySlot.ItemID;
    Data.Type = ENiosActionSlotType::Item;
    Data.Payload.InventorySlotIndex = EquipmentComponent->ResolveInventorySlotIndexForItemInstance(DisplaySlot.ItemInstanceIndex);
    Data.Count = 1;

    FNiosActionSlotVisualData Visual;
    Visual.Icon = nullptr;
    Visual.Name = FText::FromName(DisplaySlot.ItemID);
    Visual.Count = 1;
    Visual.bShowCount = false;
    Visual.bEnabled = true;
    Visual.bValid = true;

    SetSlotAndVisualData(static_cast<int32>(SlotType), Data, Visual);
    SetOccupied(true);
    SetTwoHandedMirrorDisplayState(bTwoHandedMirror);
}

bool UNiosCore_EquipmentSlotWidget::ResolveDisplayEquipmentSlot(
    FReplicatedEquipmentSlot& OutSlot,
    bool& bOutTwoHandedMirror) const
{
    OutSlot = FReplicatedEquipmentSlot();
    bOutTwoHandedMirror = false;

    if (!EquipmentComponent || SlotType == EEquipSlot::None)
    {
        return false;
    }

    const FReplicatedEquipmentSlot OwnSlot = EquipmentComponent->GetEquipmentSlot(SlotType);
    if (!OwnSlot.IsEmpty())
    {
        OutSlot = OwnSlot;
        bOutTwoHandedMirror = (SlotType == EEquipSlot::LeftHand && IsTwoHandedWeaponItem(OwnSlot.ItemID));
        return true;
    }

    // Backward/safety display path: older data or a replication race may only contain one hand slot
    // for a two-handed weapon. The UI should still show the weapon occupying both hands.
    if (SlotType != EEquipSlot::LeftHand && SlotType != EEquipSlot::RightHand)
    {
        return false;
    }

    const EEquipSlot OtherHandSlot =
        SlotType == EEquipSlot::LeftHand ? EEquipSlot::RightHand : EEquipSlot::LeftHand;

    FReplicatedEquipmentSlot OtherSlot = EquipmentComponent->GetEquipmentSlot(OtherHandSlot);
    if (OtherSlot.IsEmpty() || !IsTwoHandedWeaponItem(OtherSlot.ItemID))
    {
        return false;
    }

    OtherSlot.Slot = SlotType;
    OutSlot = OtherSlot;
    bOutTwoHandedMirror = true;
    return true;
}

EEquipSlot UNiosCore_EquipmentSlotWidget::ResolveInteractionEquipmentSlot() const
{
    if (!EquipmentComponent || SlotType == EEquipSlot::None)
    {
        return SlotType;
    }

    const FReplicatedEquipmentSlot OwnSlot = EquipmentComponent->GetEquipmentSlot(SlotType);
    if (!OwnSlot.IsEmpty())
    {
        return SlotType;
    }

    // If this widget only mirrors a two-handed item from the other hand, interactions must
    // target the real authoritative slot. This makes drag-out / unequip work from either hand.
    if (SlotType == EEquipSlot::LeftHand || SlotType == EEquipSlot::RightHand)
    {
        const EEquipSlot OtherHandSlot =
            SlotType == EEquipSlot::LeftHand ? EEquipSlot::RightHand : EEquipSlot::LeftHand;

        const FReplicatedEquipmentSlot OtherSlot = EquipmentComponent->GetEquipmentSlot(OtherHandSlot);
        if (!OtherSlot.IsEmpty() && IsTwoHandedWeaponItem(OtherSlot.ItemID))
        {
            return OtherHandSlot;
        }
    }

    return SlotType;
}

bool UNiosCore_EquipmentSlotWidget::IsTwoHandedWeaponItem(FName ItemID) const
{
    if (!EquipmentComponent || ItemID.IsNone())
    {
        return false;
    }

    FNiosWeaponRow WeaponRow;
    return EquipmentComponent->GetWeaponRowForItem(ItemID, WeaponRow)
        && EquipmentComponent->IsTwoHandedWeaponType(WeaponRow.WeaponType);
}

void UNiosCore_EquipmentSlotWidget::SetTwoHandedMirrorDisplayState(bool bNewMirrored)
{
    if (bDisplayingTwoHandedMirror == bNewMirrored)
    {
        return;
    }

    bDisplayingTwoHandedMirror = bNewMirrored;
    BP_OnTwoHandedMirrorDisplayChanged(bDisplayingTwoHandedMirror);
}

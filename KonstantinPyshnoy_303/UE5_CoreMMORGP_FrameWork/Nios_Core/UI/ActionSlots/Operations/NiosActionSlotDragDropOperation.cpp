#include "UI/ActionSlots/Operations/NiosActionSlotDragDropOperation.h"

#include "UI/ActionSlots/NiosCore_ActionSlotWidget.h"
#include "UI/Inventory/NiosCore_EquipmentSlotWidget.h"

void UNiosActionSlotDragDropOperation::DragCancelled_Implementation(
    const FPointerEvent& PointerEvent)
{
    Super::DragCancelled_Implementation(PointerEvent);

    UNiosCore_ActionSlotWidget* LocalSourceSlot = SourceSlot.Get();
    if (!LocalSourceSlot)
    {
        ReleaseRuntimeReferences();
        return;
    }

    if (!bSourceIsStatic)
    {
        if (LocalSourceSlot->InteractionProfile == ENiosSlotInteractionProfile::Hotbar)
        {
            LocalSourceSlot->ClearSlot();
        }
        else if (LocalSourceSlot->InteractionProfile == ENiosSlotInteractionProfile::Equipment)
        {
            if (UNiosCore_EquipmentSlotWidget* EquipmentSlot = Cast<UNiosCore_EquipmentSlotWidget>(LocalSourceSlot))
            {
                EquipmentSlot->RequestUnequipFromDragCancelled();
            }
        }
    }

    ReleaseRuntimeReferences();
}

void UNiosActionSlotDragDropOperation::Drop_Implementation(
    const FPointerEvent& PointerEvent)
{
    Super::Drop_Implementation(PointerEvent);
    ReleaseRuntimeReferences();
}

void UNiosActionSlotDragDropOperation::ReleaseRuntimeReferences()
{
    // Drag/drop operations may live for a short time after PIE stops in editor workflows.
    // Do not keep strong references to widgets/drag visuals that belong to a PIE world.
    SourceSlot = nullptr;
    DefaultDragVisual = nullptr;
}

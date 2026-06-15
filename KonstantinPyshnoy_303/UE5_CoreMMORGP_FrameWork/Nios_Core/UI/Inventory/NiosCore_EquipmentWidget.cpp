#include "UI/Inventory/NiosCore_EquipmentWidget.h"

#include "UI/Inventory/NiosCore_EquipmentSlotWidget.h"
#include "Character/NiosCore_PlayerState.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "UI/ActionSlots/Operations/NiosActionSlotDragDropOperation.h"

void UNiosCore_EquipmentWidget::NativeConstruct()
{
    Super::NativeConstruct();

    BuildSlotMap();
    BindSlotDropRequests();
    RefreshEquipment();
}

void UNiosCore_EquipmentWidget::NativeDestruct()
{
    UnbindSlotDropRequests();

    if (EquipmentComponent)
    {
        EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UNiosCore_EquipmentWidget::OnEquipmentChangedProxy);
    }

    Super::NativeDestruct();
}

void UNiosCore_EquipmentWidget::BuildSlotMap()
{
    AllSlots.Empty();
    SlotWidgetsByType.Empty();

    auto AddSlot = [this](EEquipSlot SlotType, UNiosCore_EquipmentSlotWidget* SlotWidget)
    {
        if (!SlotWidget)
        {
            return;
        }

        AllSlots.Add(SlotWidget);
        SlotWidgetsByType.Add(SlotType, SlotWidget);
    };

    AddSlot(EEquipSlot::Helmet, HelmetSlot);
    AddSlot(EEquipSlot::Torso, TorsoSlot);
    AddSlot(EEquipSlot::Legs, LegsSlot);
    AddSlot(EEquipSlot::Hands, HandsSlot);
    AddSlot(EEquipSlot::Feet, FeetSlot);
    AddSlot(EEquipSlot::Belt, BeltSlot);
    AddSlot(EEquipSlot::Cloak, CloakSlot);
    AddSlot(EEquipSlot::Accessory, AccessorySlot);

    AddSlot(EEquipSlot::Ring1, RingSlot1);
    AddSlot(EEquipSlot::Ring2, RingSlot2);

    AddSlot(EEquipSlot::Artifact1, ArtifactSlot1);
    AddSlot(EEquipSlot::Artifact2, ArtifactSlot2);
    AddSlot(EEquipSlot::Artifact3, ArtifactSlot3);
    AddSlot(EEquipSlot::Artifact4, ArtifactSlot4);

    AddSlot(EEquipSlot::LeftHand, LeftHandSlot);
    AddSlot(EEquipSlot::RightHand, RightHandSlot);
    AddSlot(EEquipSlot::RangedWeapon, RangedWeaponSlot);
}

void UNiosCore_EquipmentWidget::BindSlotDropRequests()
{
    for (UNiosCore_EquipmentSlotWidget* SlotWidget : AllSlots)
    {
        if (SlotWidget)
        {
            SlotWidget->OnEquipmentSlotDropRequested.AddUniqueDynamic(this, &UNiosCore_EquipmentWidget::HandleEquipmentSlotDropRequested);
            SlotWidget->OnEquipmentSlotUnequipRequested.AddUniqueDynamic(this, &UNiosCore_EquipmentWidget::HandleEquipmentSlotUnequipRequested);
        }
    }
}

void UNiosCore_EquipmentWidget::UnbindSlotDropRequests()
{
    for (UNiosCore_EquipmentSlotWidget* SlotWidget : AllSlots)
    {
        if (SlotWidget)
        {
            SlotWidget->OnEquipmentSlotDropRequested.RemoveDynamic(this, &UNiosCore_EquipmentWidget::HandleEquipmentSlotDropRequested);
            SlotWidget->OnEquipmentSlotUnequipRequested.RemoveDynamic(this, &UNiosCore_EquipmentWidget::HandleEquipmentSlotUnequipRequested);
        }
    }
}

void UNiosCore_EquipmentWidget::InitializeWidget(UNiosCore_EquipmentComponent* InEquipmentComponent)
{
    if (EquipmentComponent == InEquipmentComponent)
    {
        RefreshEquipment();
        return;
    }

    if (EquipmentComponent)
    {
        EquipmentComponent->OnEquipmentChanged.RemoveDynamic(this, &UNiosCore_EquipmentWidget::OnEquipmentChangedProxy);
    }

    EquipmentComponent = InEquipmentComponent;

    if (EquipmentComponent)
    {
        EquipmentComponent->OnEquipmentChanged.AddUniqueDynamic(this, &UNiosCore_EquipmentWidget::OnEquipmentChangedProxy);
    }

    RefreshEquipment();
}

void UNiosCore_EquipmentWidget::InitializeFromPlayerState(APlayerState* PS)
{
    if (!PS)
    {
        return;
    }

    ANiosCore_PlayerState* NiosPS = Cast<ANiosCore_PlayerState>(PS);
    if (!NiosPS)
    {
        return;
    }

    if (NiosPS->EquipmentComponent)
    {
        InitializeWidget(NiosPS->EquipmentComponent);
    }
}

void UNiosCore_EquipmentWidget::OnEquipmentChangedProxy(EEquipSlot ChangedSlot)
{
    RefreshEquipment();
}

void UNiosCore_EquipmentWidget::RefreshEquipment()
{
    if (SlotWidgetsByType.Num() == 0)
    {
        BuildSlotMap();
        BindSlotDropRequests();
    }

    for (const TPair<EEquipSlot, TObjectPtr<UNiosCore_EquipmentSlotWidget>>& Pair : SlotWidgetsByType)
    {
        if (Pair.Value)
        {
            Pair.Value->InitializeEquipmentSlot(Pair.Key, EquipmentComponent);
        }
    }
}

void UNiosCore_EquipmentWidget::HandleEquipmentSlotDropRequested(FNiosEquipmentSlotRequest Request)
{
    if (OnEquipmentSlotDropRequested.IsBound())
    {
        OnEquipmentSlotDropRequested.Broadcast(Request);
        return;
    }

    bool bAccepted = false;

    if (EquipmentComponent)
    {
        bAccepted = EquipmentComponent->EquipItemFromInventorySlotRequest(Request);

        // If the player dropped an item onto any child equipment slot that is not compatible
        // with the item, still allow the whole equipment panel to behave as an auto-equip area.
        // This covers the common UMG case where visible slot widgets cover the background panel,
        // so the parent EquipmentWidget never receives NativeOnDrop directly.
        if (!bAccepted && bFallbackToAutoEquipWhenSpecificSlotRejects && Request.InventorySlotIndex != INDEX_NONE)
        {
            bAccepted = EquipmentComponent->AutoEquipItemFromInventorySlot(Request.InventorySlotIndex);
        }
    }

    if (bAccepted)
    {
        RefreshEquipment();
    }
}


void UNiosCore_EquipmentWidget::HandleEquipmentSlotUnequipRequested(EEquipSlot InSlot)
{
    if (OnEquipmentSlotUnequipRequested.IsBound())
    {
        OnEquipmentSlotUnequipRequested.Broadcast(InSlot);
        return;
    }

    if (EquipmentComponent)
    {
        EquipmentComponent->UnequipSlot(InSlot);
    }
}


bool UNiosCore_EquipmentWidget::ExtractInventorySlotIndexFromDropOperation(
    UDragDropOperation* InOperation,
    int32& OutInventorySlotIndex) const
{
    OutInventorySlotIndex = INDEX_NONE;

    const UNiosActionSlotDragDropOperation* DragOp = Cast<UNiosActionSlotDragDropOperation>(InOperation);
    if (!DragOp || DragOp->SlotData.IsEmpty())
    {
        return false;
    }

    if (DragOp->SlotData.Type != ENiosActionSlotType::Item)
    {
        return false;
    }

    const int32 InventorySlotIndex = DragOp->SlotData.Payload.InventorySlotIndex;
    if (InventorySlotIndex == INDEX_NONE)
    {
        return false;
    }

    OutInventorySlotIndex = InventorySlotIndex;
    return true;
}

bool UNiosCore_EquipmentWidget::CanAutoEquipFromDropOperation(
    UDragDropOperation* InOperation,
    int32& OutInventorySlotIndex) const
{
    if (!bEnableAutoEquipAreaDrop)
    {
        OutInventorySlotIndex = INDEX_NONE;
        return false;
    }

    return ExtractInventorySlotIndexFromDropOperation(InOperation, OutInventorySlotIndex);
}

bool UNiosCore_EquipmentWidget::TryAutoEquipInventorySlotFromWidgetArea(int32 InventorySlotIndex)
{
    if (!bEnableAutoEquipAreaDrop || InventorySlotIndex == INDEX_NONE)
    {
        OnAutoEquipAreaDropHandled.Broadcast(InventorySlotIndex, false);
        return false;
    }

    // Preferred path: ask EquipmentComponent. On clients this now sends an authoritative server RPC
    // and does NOT mutate local equipment/inventory state, so it cannot create phantom equipped items.
    const bool bRequestAccepted = EquipmentComponent && EquipmentComponent->AutoEquipItemFromInventorySlot(InventorySlotIndex);

    // Optional BP hook for UI sounds/messages. It should no longer be required for the actual equip request.
    if (OnAutoEquipInventorySlotRequested.IsBound())
    {
        OnAutoEquipInventorySlotRequested.Broadcast(InventorySlotIndex);
    }

    OnAutoEquipAreaDropHandled.Broadcast(InventorySlotIndex, bRequestAccepted);

    if (bRequestAccepted && EquipmentComponent && EquipmentComponent->GetOwnerRole() == ROLE_Authority)
    {
        RefreshEquipment();
    }

    return bRequestAccepted;
}

bool UNiosCore_EquipmentWidget::NativeOnDragOver(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    if (!bConsumeAutoEquipAreaDragOver)
    {
        return Super::NativeOnDragOver(InGeometry, InDragDropEvent, InOperation);
    }

    int32 InventorySlotIndex = INDEX_NONE;
    return CanAutoEquipFromDropOperation(InOperation, InventorySlotIndex);
}

void UNiosCore_EquipmentWidget::NativeOnDragEnter(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    Super::NativeOnDragEnter(InGeometry, InDragDropEvent, InOperation);

    int32 InventorySlotIndex = INDEX_NONE;
    OnAutoEquipAreaDragStateChanged.Broadcast(CanAutoEquipFromDropOperation(InOperation, InventorySlotIndex));
}

void UNiosCore_EquipmentWidget::NativeOnDragLeave(
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    Super::NativeOnDragLeave(InDragDropEvent, InOperation);
    OnAutoEquipAreaDragStateChanged.Broadcast(false);
}

bool UNiosCore_EquipmentWidget::NativeOnDrop(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    int32 InventorySlotIndex = INDEX_NONE;
    if (!CanAutoEquipFromDropOperation(InOperation, InventorySlotIndex))
    {
        return false;
    }

    return TryAutoEquipInventorySlotFromWidgetArea(InventorySlotIndex);
}

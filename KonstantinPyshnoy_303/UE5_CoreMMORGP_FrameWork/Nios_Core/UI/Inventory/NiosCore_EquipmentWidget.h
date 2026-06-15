#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Items/NiosCore_ItemDataTypes.h"
#include "NiosCore_EquipmentWidget.generated.h"

class UNiosCore_EquipmentSlotWidget;
class UNiosCore_EquipmentComponent;
class UDragDropOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosEquipmentAutoEquipRequested,
    int32,
    InventorySlotIndex
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosEquipmentAutoEquipAreaDragStateChanged,
    bool,
    bCanAutoEquipDrop
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosEquipmentAutoEquipAreaDropHandled,
    int32,
    InventorySlotIndex,
    bool,
    bAccepted
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosEquipmentWidgetSlotDropRequested,
    FNiosEquipmentSlotRequest,
    Request
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosEquipmentWidgetUnequipRequested,
    EEquipSlot,
    Slot
);

UCLASS()
class NIOS_CORE_API UNiosCore_EquipmentWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** Returns true when this drag/drop operation contains an inventory item that EquipmentWidget can auto-equip. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|DragDrop")
    bool CanAutoEquipFromDropOperation(UDragDropOperation* InOperation, int32& OutInventorySlotIndex) const;

    /** Same as dropping an inventory item on the empty body of this equipment widget. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Equipment|DragDrop")
    bool TryAutoEquipInventorySlotFromWidgetArea(int32 InventorySlotIndex);

    UFUNCTION(BlueprintCallable)
    void InitializeWidget(UNiosCore_EquipmentComponent* InEquipmentComponent);

    UFUNCTION(BlueprintCallable)
    void InitializeFromPlayerState(APlayerState* PS);

    UFUNCTION()
    void OnEquipmentChangedProxy(EEquipSlot ChangedSlot);

    /** UI request: предмет сброшен на тело EquipmentWidget, без конкретного слота. Обычно это AutoEquip. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment")
    FNiosEquipmentAutoEquipRequested OnAutoEquipInventorySlotRequested;

    /** UI request: предмет сброшен на конкретный equipment slot. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment")
    FNiosEquipmentWidgetSlotDropRequested OnEquipmentSlotDropRequested;

    /** UI request: предмет из equipment slot брошен в пустоту. Обычно это UnequipSlot. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment")
    FNiosEquipmentWidgetUnequipRequested OnEquipmentSlotUnequipRequested;

    /** Local-only UI feedback for hovering an inventory item over the equipment panel body. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment|DragDrop")
    FNiosEquipmentAutoEquipAreaDragStateChanged OnAutoEquipAreaDragStateChanged;

    /** Fired after EquipmentWidget handled an area drop for auto-equip. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Equipment|DragDrop")
    FNiosEquipmentAutoEquipAreaDropHandled OnAutoEquipAreaDropHandled;

protected:
    virtual bool NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation
    ) override;

    virtual bool NativeOnDragOver(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation
    ) override;

    virtual void NativeOnDragEnter(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation
    ) override;

    virtual void NativeOnDragLeave(
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation
    ) override;

    /** Allow dropping an inventory item anywhere on this equipment panel to auto-equip it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Equipment|DragDrop")
    bool bEnableAutoEquipAreaDrop = true;

    /** Return true from NativeOnDragOver for valid inventory item drags so the panel can receive OnDrop. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Equipment|DragDrop")
    bool bConsumeAutoEquipAreaDragOver = true;

    /**
     * If an inventory item is dropped on any equipment slot and that exact slot rejects it,
     * try normal auto-equip. This makes dropping onto the equipment panel feel like
     * "put this item somewhere valid", even when child slot widgets cover the panel.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Equipment|DragDrop")
    bool bFallbackToAutoEquipWhenSpecificSlotRejects = true;

    UPROPERTY(BlueprintReadOnly, Category = "Equipment")
    TObjectPtr<UNiosCore_EquipmentComponent> EquipmentComponent = nullptr;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> HelmetSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> TorsoSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> LegsSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> HandsSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> FeetSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> BeltSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> CloakSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> AccessorySlot;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> RingSlot1;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> RingSlot2;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> ArtifactSlot1;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> ArtifactSlot2;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> ArtifactSlot3;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> ArtifactSlot4;

    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> LeftHandSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> RightHandSlot;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UNiosCore_EquipmentSlotWidget> RangedWeaponSlot;

    UPROPERTY()
    TArray<TObjectPtr<UNiosCore_EquipmentSlotWidget>> AllSlots;

    UPROPERTY()
    TMap<EEquipSlot, TObjectPtr<UNiosCore_EquipmentSlotWidget>> SlotWidgetsByType;

    UFUNCTION(BlueprintCallable)
    void RefreshEquipment();

    UFUNCTION()
    void HandleEquipmentSlotDropRequested(FNiosEquipmentSlotRequest Request);

    UFUNCTION()
    void HandleEquipmentSlotUnequipRequested(EEquipSlot InSlot);

    void BuildSlotMap();
    void BindSlotDropRequests();
    void UnbindSlotDropRequests();

    bool ExtractInventorySlotIndexFromDropOperation(UDragDropOperation* InOperation, int32& OutInventorySlotIndex) const;
};

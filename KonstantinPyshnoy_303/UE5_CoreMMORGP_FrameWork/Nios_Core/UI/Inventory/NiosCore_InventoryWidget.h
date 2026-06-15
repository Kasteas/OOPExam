#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Items/NiosCore_ItemDataTypes.h"
#include "Core/Nios_CoreStructers.h"
#include "NiosCore_InventoryWidget.generated.h"

class UUniformGridPanel;
class UNiosCore_ActionSlotWidget;
class UNiosCore_InventoryComponent;
class UActionSlotResolverSubsystem;
class UNiosCore_EquipmentComponent;


UENUM(BlueprintType)
enum class ENiosInventoryItemTypeFilter : uint8
{
    All UMETA(DisplayName = "All"),
    Equipment UMETA(DisplayName = "Equipment"),
    Weapon UMETA(DisplayName = "Weapon"),
    Armor UMETA(DisplayName = "Armor"),
    Usable UMETA(DisplayName = "Usable"),
    Misc UMETA(DisplayName = "Misc")
};

UENUM(BlueprintType)
enum class ENiosInventorySortMode : uint8
{
    Slot UMETA(DisplayName = "Slot"),
    ItemType UMETA(DisplayName = "Item Type"),
    Name UMETA(DisplayName = "Name")
};

UCLASS()
class NIOS_CORE_API UNiosCore_InventoryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ��������� ��������� ���������
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void BindInventoryComponent(UNiosCore_InventoryComponent* InInventoryComponent);

    // �������
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void UnbindInventoryComponent();

    // ������ ����������� ���� ������
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void RefreshFromInventory();

    /** Filter displayed backpack items by broad type. Does not mutate server inventory. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Filter")
    void SetItemTypeFilter(ENiosInventoryItemTypeFilter InFilter);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Filter")
    void ShowAllItems();

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Filter")
    void ShowEquipmentItems();

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Filter")
    void ShowUsableItems();

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Filter")
    void ShowMiscItems();

    /** Search by resolved item display name or raw ItemID. Empty text disables search. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Search")
    void SetInventorySearchText(const FText& InSearchText);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Search")
    void ClearInventorySearchText();

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Sort")
    void SetInventorySortMode(ENiosInventorySortMode InSortMode);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Sort")
    void SortBySlot();

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Sort")
    void SortByItemType();

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory|Sort")
    void SortByName();

    // ���������� ������ ����� �� �������
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void RefreshSlot(int32 InventoryIndex);

    // ������� �����
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void ClearSlot(int32 Index);

    // ��������� ������� ����� �� �������
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    UNiosCore_ActionSlotWidget* GetSlotByIndex(int32 InventoryIndex) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    void BindEquipmentComponent(UNiosCore_EquipmentComponent* InEquipmentComponent);

    /** ������������� ������ ����� */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Inventory")
    FVector2D SlotSize = FVector2D(100.f, 100.f);


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Inventory|Filter")
    ENiosInventoryItemTypeFilter ItemTypeFilter = ENiosInventoryItemTypeFilter::All;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Inventory|Sort")
    ENiosInventorySortMode SortMode = ENiosInventorySortMode::Slot;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Inventory|Search")
    FText SearchText;

    UPROPERTY(BlueprintReadOnly, Category = "Inventory")
    TObjectPtr<UNiosCore_EquipmentComponent> EquipmentComponent = nullptr;
protected:
    virtual void NativeDestruct() override;

    void RebuildCompactInventory();
    bool ShouldDisplayInventorySlot(const FNiosInventorySlot& InventorySlot, FText* OutResolvedName = nullptr, ENiosItemType* OutItemType = nullptr) const;
    FText ResolveItemDisplayName(FName ItemID) const;
    int32 GetItemTypeSortPriority(ENiosItemType ItemType) const;
    bool IsFilteredOrSortedViewActive() const;
    UNiosCore_ActionSlotWidget* CreateInventorySlotWidget(int32 VisualIndex, int32 InventoryIndex);
    void ApplyInventorySlotToWidget(UNiosCore_ActionSlotWidget* SlotWidget, const FNiosInventorySlot& InventorySlot, int32 ItemIndex = 0);

    UFUNCTION()
    void HandleInventoryChanged();

    UFUNCTION()
    void HandleInventorySlotChanged(int32 SlotIndex);

    UActionSlotResolverSubsystem* ResolveResolverSubsystem();
    FNiosActionSlotData MakeActionSlotDataFromInventorySlot(const FNiosInventorySlot& InventorySlot) const;

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> GridSlots = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Inventory")
    TSubclassOf<UNiosCore_ActionSlotWidget> SlotWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Inventory")
    int32 Columns = 5;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Inventory")
    TObjectPtr<UNiosCore_InventoryComponent> InventoryComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Inventory")
    TObjectPtr<UActionSlotResolverSubsystem> ResolverSubsystem = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Inventory")
    TArray<TObjectPtr<UNiosCore_ActionSlotWidget>> Slots;

    UPROPERTY()
    TMap<int32, TObjectPtr<UNiosCore_ActionSlotWidget>> SlotWidgetByInventoryIndex;

public:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Inventory")
    int32 SlotCount = 0;
};
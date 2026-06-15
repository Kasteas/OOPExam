#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Core/Nios_CoreStructers.h"
#include "NiosActionSlotDragDropOperation.generated.h"

class UNiosCore_ActionSlotWidget;

UCLASS()
class NIOS_CORE_API UNiosActionSlotDragDropOperation
    : public UDragDropOperation
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite)
    FNiosActionSlotData SlotData;

    UPROPERTY(BlueprintReadWrite)
    FNiosActionSlotVisualData VisualData;

    UPROPERTY(BlueprintReadWrite)
    TObjectPtr<UNiosCore_ActionSlotWidget> SourceSlot;

    UPROPERTY(BlueprintReadWrite)
    bool bSourceIsStatic = false;

    virtual void DragCancelled_Implementation(
        const FPointerEvent& PointerEvent
    ) override;

    virtual void Drop_Implementation(
        const FPointerEvent& PointerEvent
    ) override;

protected:
    void ReleaseRuntimeReferences();
};
#include "UI/ActionSlots/NiosCore_DragVisualWidget.h"

#include "Components/Image.h"

void UNiosCore_DragVisualWidget::SetVisualData(
    const FNiosActionSlotVisualData& InVisualData)
{
    if (!Image_Icon)
    {
        return;
    }

    UTexture2D* Texture =
        InVisualData.Icon.LoadSynchronous();

    if (!Texture)
    {
        return;
    }

    Image_Icon->SetBrushFromTexture(Texture);
}
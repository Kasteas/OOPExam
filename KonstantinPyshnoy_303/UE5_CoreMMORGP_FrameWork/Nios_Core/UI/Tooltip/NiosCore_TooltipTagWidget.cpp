#include "UI/Tooltip/NiosCore_TooltipTagWidget.h"

#include "Components/TextBlock.h"

void UNiosCore_TooltipTagWidget::SetTagText(const FText& InTagText)
{
    TagTextValue = InTagText;

    if (TagText)
    {
        TagText->SetText(TagTextValue);
        TagText->SetVisibility(TagTextValue.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    SetVisibility(TagTextValue.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    BP_OnTagTextChanged(TagTextValue);
}

FText UNiosCore_TooltipTagWidget::GetTagText() const
{
    return TagTextValue;
}

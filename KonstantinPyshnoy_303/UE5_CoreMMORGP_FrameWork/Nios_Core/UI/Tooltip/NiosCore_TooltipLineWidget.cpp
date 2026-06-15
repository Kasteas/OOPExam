#include "UI/Tooltip/NiosCore_TooltipLineWidget.h"

#include "Components/TextBlock.h"

void UNiosCore_TooltipLineWidget::SetLineData(const FNiosTooltipLine& InLineData)
{
    LineData = InLineData;
    ApplyLineDataToBoundWidgets();
    BP_OnLineDataChanged(LineData);
}

FNiosTooltipLine UNiosCore_TooltipLineWidget::GetLineData() const
{
    return LineData;
}

bool UNiosCore_TooltipLineWidget::ShouldShowLine() const
{
    if (LineData.bShowWhenZero)
    {
        return true;
    }

    return !LineData.Label.IsEmpty() || !LineData.Value.IsEmpty();
}

FSlateColor UNiosCore_TooltipLineWidget::GetColorForStyle(ENiosTooltipLineStyle Style) const
{
    switch (Style)
    {
    case ENiosTooltipLineStyle::Positive:
        return PositiveColor;
    case ENiosTooltipLineStyle::Negative:
        return NegativeColor;
    case ENiosTooltipLineStyle::Warning:
        return WarningColor;
    case ENiosTooltipLineStyle::Header:
        return HeaderColor;
    case ENiosTooltipLineStyle::Normal:
    default:
        return NormalColor;
    }
}

void UNiosCore_TooltipLineWidget::ApplyLineDataToBoundWidgets()
{
    const FSlateColor LineColor = GetColorForStyle(LineData.Style);

    if (LabelText)
    {
        LabelText->SetText(LineData.Label);
        LabelText->SetColorAndOpacity(LineColor);
        LabelText->SetVisibility(LineData.Label.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    if (ValueText)
    {
        ValueText->SetText(LineData.Value);
        ValueText->SetColorAndOpacity(LineColor);
        ValueText->SetVisibility(LineData.Value.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    SetVisibility(ShouldShowLine() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

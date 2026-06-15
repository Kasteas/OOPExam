#include "UI/Tooltip/NiosCore_TooltipSectionWidget.h"

#include "UI/Tooltip/NiosCore_TooltipLineWidget.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"

void UNiosCore_TooltipSectionWidget::SetSectionData(const FNiosTooltipSection& InSectionData)
{
    SectionData = InSectionData;

    if (HeaderText)
    {
        HeaderText->SetText(SectionData.Header);
        HeaderText->SetVisibility(SectionData.Header.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    }

    BuildLines();
    BP_OnSectionDataChanged(SectionData);
}

void UNiosCore_TooltipSectionWidget::ClearSection()
{
    SectionData = FNiosTooltipSection();

    if (HeaderText)
    {
        HeaderText->SetText(FText::GetEmpty());
        HeaderText->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (LinesBox)
    {
        LinesBox->ClearChildren();
    }

    SetVisibility(ESlateVisibility::Collapsed);
}

FNiosTooltipSection UNiosCore_TooltipSectionWidget::GetSectionData() const
{
    return SectionData;
}

void UNiosCore_TooltipSectionWidget::SetLineWidgetClass(TSubclassOf<UNiosCore_TooltipLineWidget> InLineWidgetClass)
{
    LineWidgetClass = InLineWidgetClass;

    if (SectionData.Lines.Num() > 0)
    {
        BuildLines();
    }
}

bool UNiosCore_TooltipSectionWidget::ShouldCreateLine_Implementation(const FNiosTooltipLine& Line) const
{
    return Line.bShowWhenZero || !Line.Label.IsEmpty() || !Line.Value.IsEmpty();
}

void UNiosCore_TooltipSectionWidget::BuildLines()
{
    if (LinesBox)
    {
        LinesBox->ClearChildren();
    }

    int32 AddedLines = 0;

    if (LinesBox && LineWidgetClass)
    {
        for (const FNiosTooltipLine& Line : SectionData.Lines)
        {
            if (!ShouldCreateLine(Line))
            {
                continue;
            }

            UNiosCore_TooltipLineWidget* LineWidget = CreateWidget<UNiosCore_TooltipLineWidget>(GetOwningPlayer(), LineWidgetClass);
            if (!LineWidget)
            {
                continue;
            }

            LineWidget->SetLineData(Line);
            LinesBox->AddChild(LineWidget);
            ++AddedLines;
        }
    }

    const bool bHasHeader = !SectionData.Header.IsEmpty();
    SetVisibility((bHasHeader || AddedLines > 0) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

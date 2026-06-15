#include "UI/Tooltip/NiosCore_TooltipWindowWidget.h"

#include "UI/Tooltip/NiosCore_TooltipLineWidget.h"
#include "UI/Tooltip/NiosCore_TooltipSectionWidget.h"
#include "UI/Tooltip/NiosCore_TooltipTagWidget.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UNiosCore_TooltipWindowWidget::SetTooltipData(const FNiosTooltipData& InTooltipData)
{
    TooltipData = InTooltipData;

    if (!TooltipData.bValid)
    {
        ClearTooltip();
        return;
    }

    ApplyHeader();
    BuildMainLines();
    BuildSections();
    BuildTags();

    SetVisibility(ESlateVisibility::HitTestInvisible);
    BP_OnTooltipDataChanged(TooltipData);
}

void UNiosCore_TooltipWindowWidget::ClearTooltip()
{
    TooltipData = FNiosTooltipData();

    if (IconImage)
    {
        IconImage->SetBrushFromTexture(nullptr);
        IconImage->SetVisibility(ESlateVisibility::Collapsed);
    }

    SetTextVisibilityByContent(CountText, FText::GetEmpty());
    SetTextVisibilityByContent(TitleText, FText::GetEmpty());
    SetTextVisibilityByContent(SubtitleText, FText::GetEmpty());
    SetTextVisibilityByContent(DescriptionText, FText::GetEmpty());

    if (MainLinesBox)
    {
        MainLinesBox->ClearChildren();
    }

    if (SectionsBox)
    {
        SectionsBox->ClearChildren();
    }

    if (TagsBox)
    {
        TagsBox->ClearChildren();
    }

    SetVisibility(ESlateVisibility::Collapsed);
    BP_OnTooltipCleared();
}

FNiosTooltipData UNiosCore_TooltipWindowWidget::GetTooltipData() const
{
    return TooltipData;
}

bool UNiosCore_TooltipWindowWidget::HasValidTooltip() const
{
    return TooltipData.bValid;
}

bool UNiosCore_TooltipWindowWidget::ShouldCreateLine_Implementation(const FNiosTooltipLine& Line) const
{
    return Line.bShowWhenZero || !Line.Label.IsEmpty() || !Line.Value.IsEmpty();
}

bool UNiosCore_TooltipWindowWidget::ShouldCreateSection_Implementation(const FNiosTooltipSection& Section) const
{
    if (!Section.Header.IsEmpty())
    {
        return true;
    }

    for (const FNiosTooltipLine& Line : Section.Lines)
    {
        if (ShouldCreateLine(Line))
        {
            return true;
        }
    }

    return false;
}

bool UNiosCore_TooltipWindowWidget::ShouldCreateTag_Implementation(const FText& Tag) const
{
    return !Tag.IsEmpty();
}

void UNiosCore_TooltipWindowWidget::ApplyHeader()
{
    if (IconImage)
    {
        UTexture2D* LoadedIcon = nullptr;
        if (!TooltipData.Icon.IsNull() && bLoadIconSynchronously)
        {
            LoadedIcon = TooltipData.Icon.LoadSynchronous();
        }

        IconImage->SetBrushFromTexture(LoadedIcon);
        IconImage->SetVisibility(LoadedIcon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (CountText)
    {
        const bool bShowCount = TooltipData.bShowCount && TooltipData.Count > 1;
        CountText->SetText(bShowCount ? FText::AsNumber(TooltipData.Count) : FText::GetEmpty());
        CountText->SetVisibility(bShowCount ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    SetTextVisibilityByContent(TitleText, TooltipData.Title);
    SetTextVisibilityByContent(SubtitleText, TooltipData.Subtitle);
    SetTextVisibilityByContent(DescriptionText, TooltipData.Description);
}

void UNiosCore_TooltipWindowWidget::BuildMainLines()
{
    if (!MainLinesBox)
    {
        return;
    }

    MainLinesBox->ClearChildren();

    if (!LineWidgetClass)
    {
        MainLinesBox->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    int32 AddedLines = 0;
    for (const FNiosTooltipLine& Line : TooltipData.MainLines)
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
        MainLinesBox->AddChild(LineWidget);
        ++AddedLines;
    }

    MainLinesBox->SetVisibility(AddedLines > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UNiosCore_TooltipWindowWidget::BuildSections()
{
    if (!SectionsBox)
    {
        return;
    }

    SectionsBox->ClearChildren();

    if (!SectionWidgetClass)
    {
        SectionsBox->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    int32 AddedSections = 0;
    for (const FNiosTooltipSection& Section : TooltipData.Sections)
    {
        if (!ShouldCreateSection(Section))
        {
            continue;
        }

        UNiosCore_TooltipSectionWidget* SectionWidget = CreateWidget<UNiosCore_TooltipSectionWidget>(GetOwningPlayer(), SectionWidgetClass);
        if (!SectionWidget)
        {
            continue;
        }

        SectionWidget->SetLineWidgetClass(LineWidgetClass);
        SectionWidget->SetSectionData(Section);
        SectionsBox->AddChild(SectionWidget);
        ++AddedSections;
    }

    SectionsBox->SetVisibility(AddedSections > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UNiosCore_TooltipWindowWidget::BuildTags()
{
    if (!TagsBox)
    {
        return;
    }

    TagsBox->ClearChildren();

    if (!TagWidgetClass)
    {
        TagsBox->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    int32 AddedTags = 0;
    for (const FText& Tag : TooltipData.Tags)
    {
        if (!ShouldCreateTag(Tag))
        {
            continue;
        }

        UNiosCore_TooltipTagWidget* TagWidget = CreateWidget<UNiosCore_TooltipTagWidget>(GetOwningPlayer(), TagWidgetClass);
        if (!TagWidget)
        {
            continue;
        }

        TagWidget->SetTagText(Tag);
        TagsBox->AddChild(TagWidget);
        ++AddedTags;
    }

    TagsBox->SetVisibility(AddedTags > 0 ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

void UNiosCore_TooltipWindowWidget::SetTextVisibilityByContent(UTextBlock* TextBlock, const FText& Text)
{
    if (!TextBlock)
    {
        return;
    }

    TextBlock->SetText(Text);
    TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
}

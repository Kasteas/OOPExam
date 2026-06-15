#include "UI/StatusEffects/NiosCore_StatusEffectIconWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void UNiosCore_StatusEffectIconWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshRemainingTime();
}

void UNiosCore_StatusEffectIconWidget::SetStatusEffectData(const FNiosReplicatedStatusEffect& InEffectData)
{
    EffectData = InEffectData;
    RefreshVisuals();
    BP_OnStatusEffectDataChanged(EffectData);
}

float UNiosCore_StatusEffectIconWidget::GetRemainingTime() const
{
    if (IsInfiniteDuration())
    {
        return 0.f;
    }

    const UWorld* World = GetWorld();
    const float Now = World ? World->GetTimeSeconds() : 0.f;
    return FMath::Max(0.f, EffectData.EndTime - Now);
}

float UNiosCore_StatusEffectIconWidget::GetDuration() const
{
    if (IsInfiniteDuration())
    {
        return 0.f;
    }

    return FMath::Max(0.f, EffectData.EndTime - EffectData.StartTime);
}

bool UNiosCore_StatusEffectIconWidget::IsInfiniteDuration() const
{
    return EffectData.EndTime <= 0.f;
}

bool UNiosCore_StatusEffectIconWidget::BuildTooltipData(FNiosTooltipData& OutTooltipData) const
{
    OutTooltipData.Reset();

    if (!HasValidStatusEffect())
    {
        return false;
    }

    OutTooltipData.bValid = true;
    OutTooltipData.Type = ENiosTooltipType::StatusEffect;
    OutTooltipData.SourceID = EffectData.EffectID;
    OutTooltipData.Title = EffectData.DisplayName;
    OutTooltipData.Description = EffectData.Description;
    OutTooltipData.Icon = EffectData.Icon;
    OutTooltipData.Count = EffectData.StackCount;
    OutTooltipData.bShowCount = EffectData.StackCount > 1;

    if (EffectData.StackCount > 1)
    {
        OutTooltipData.MainLines.Add({ NSLOCTEXT("NiosStatusEffect", "Stacks", "Stacks"), FText::AsNumber(EffectData.StackCount), ENiosTooltipLineStyle::Normal, true });
    }

    if (IsInfiniteDuration())
    {
        OutTooltipData.MainLines.Add({ NSLOCTEXT("NiosStatusEffect", "Duration", "Duration"), NSLOCTEXT("NiosStatusEffect", "Infinite", "Infinite"), ENiosTooltipLineStyle::Normal, true });
    }
    else
    {
        OutTooltipData.MainLines.Add({ NSLOCTEXT("NiosStatusEffect", "Remaining", "Remaining"), FormatRemainingTime(GetRemainingTime()), ENiosTooltipLineStyle::Normal, true });
        OutTooltipData.MainLines.Add({ NSLOCTEXT("NiosStatusEffect", "TotalDuration", "Duration"), FormatDuration(GetDuration()), ENiosTooltipLineStyle::Normal, true });
    }

    const UEnum* VisibilityEnum = StaticEnum<ENiosStatusEffectVisibility>();
    if (VisibilityEnum)
    {
        OutTooltipData.MainLines.Add({ NSLOCTEXT("NiosStatusEffect", "Visibility", "Visibility"), VisibilityEnum->GetDisplayNameTextByValue(static_cast<int64>(EffectData.Visibility)), ENiosTooltipLineStyle::Normal, true });
    }

    for (const FGameplayTag& Tag : EffectData.Tags)
    {
        OutTooltipData.Tags.Add(FText::FromString(Tag.ToString()));
    }

    return true;
}

FText UNiosCore_StatusEffectIconWidget::FormatRemainingTime_Implementation(float RemainingSeconds) const
{
    if (RemainingSeconds >= 60.f)
    {
        return FText::Format(NSLOCTEXT("NiosStatusEffect", "TimeMinutes", "{0}m"), FText::AsNumber(FMath::CeilToInt(RemainingSeconds / 60.f)));
    }

    return FText::Format(NSLOCTEXT("NiosStatusEffect", "TimeSeconds", "{0}s"), FText::AsNumber(FMath::CeilToInt(RemainingSeconds)));
}

FText UNiosCore_StatusEffectIconWidget::FormatDuration_Implementation(float DurationSeconds) const
{
    return FormatRemainingTime(DurationSeconds);
}

void UNiosCore_StatusEffectIconWidget::RefreshVisuals()
{
    if (IconImage)
    {
        UTexture2D* LoadedIcon = nullptr;
        if (!EffectData.Icon.IsNull() && bLoadIconSynchronously)
        {
            LoadedIcon = EffectData.Icon.LoadSynchronous();
        }

        IconImage->SetBrushFromTexture(LoadedIcon);
        IconImage->SetVisibility(LoadedIcon ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    if (StackText)
    {
        const bool bShouldShowStacks = bShowStacks && EffectData.StackCount > 1;
        StackText->SetText(bShouldShowStacks ? FText::AsNumber(EffectData.StackCount) : FText::GetEmpty());
        StackText->SetVisibility(bShouldShowStacks ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    }

    RefreshRemainingTime();
}

void UNiosCore_StatusEffectIconWidget::RefreshRemainingTime()
{
    UTextBlock* EffectiveRemainingText = RemainingText ? RemainingText : TimeText;
    if (!EffectiveRemainingText)
    {
        return;
    }

    if (!bShowRemainingTime || !HasValidStatusEffect() || IsInfiniteDuration())
    {
        EffectiveRemainingText->SetText(FText::GetEmpty());
        EffectiveRemainingText->SetVisibility(ESlateVisibility::Collapsed);
        return;
    }

    const float Remaining = GetRemainingTime();
    EffectiveRemainingText->SetText(FormatRemainingTime(Remaining));
    EffectiveRemainingText->SetVisibility(Remaining > 0.f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

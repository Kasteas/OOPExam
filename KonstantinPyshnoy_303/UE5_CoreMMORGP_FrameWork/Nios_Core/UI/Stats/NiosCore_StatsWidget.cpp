#include "UI/Stats/NiosCore_StatsWidget.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Character/NiosCore_PlayerState.h"
#include "GameFramework/PlayerController.h"



void UNiosCore_StatsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    ANiosCore_PlayerState* NiosPS =
        PC->GetPlayerState<ANiosCore_PlayerState>();

    if (!NiosPS || !NiosPS->StatsComponent)
    {
        return;
    }

    BindStatsComponent(NiosPS->StatsComponent);
}

void UNiosCore_StatsWidget::NativeDestruct()
{
    if (StatsComponent)
    {
        StatsComponent->OnCalculatedStatsChanged.RemoveDynamic(this, &UNiosCore_StatsWidget::HandleStatsChanged);
    }

    Super::NativeDestruct();
}

void UNiosCore_StatsWidget::BindStatsComponent(UNiosCore_StatsComponent* InStatsComponent)
{
    if (StatsComponent == InStatsComponent)
    {
        RefreshStats();
        return;
    }

    if (StatsComponent)
    {
        StatsComponent->OnCalculatedStatsChanged.RemoveDynamic(this, &UNiosCore_StatsWidget::HandleStatsChanged);
    }

    StatsComponent = InStatsComponent;

    if (StatsComponent)
    {
        StatsComponent->OnCalculatedStatsChanged.AddUniqueDynamic(this, &UNiosCore_StatsWidget::HandleStatsChanged);
    }

    RefreshStats();
}

void UNiosCore_StatsWidget::RefreshStats()
{
    CachedStats = StatsComponent ? StatsComponent->GetCalculatedStats() : TArray<FNiosCalculatedStat>();
    OnStatsWidgetDataChanged.Broadcast(CachedStats);
}

TArray<FNiosCalculatedStat> UNiosCore_StatsWidget::GetStats() const
{
    return CachedStats;
}

void UNiosCore_StatsWidget::HandleStatsChanged(const TArray<FNiosCalculatedStat>& NewStats)
{
    CachedStats = NewStats;
    OnStatsWidgetDataChanged.Broadcast(CachedStats);
}

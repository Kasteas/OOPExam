#include "UI/StatusEffects/NiosCore_StatusEffectGridWidget.h"

#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "UI/StatusEffects/NiosCore_StatusEffectIconWidget.h"

void UNiosCore_StatusEffectGridWidget::NativeConstruct()
{
    Super::NativeConstruct();
    BindToComponent();
    RefreshStatusEffects();
}

void UNiosCore_StatusEffectGridWidget::NativeDestruct()
{
    UnbindFromComponent();
    Super::NativeDestruct();
}

void UNiosCore_StatusEffectGridWidget::SetStatusEffectComponent(UNiosCore_StatusEffectComponent* InStatusEffectComponent)
{
    if (StatusEffectComponent == InStatusEffectComponent)
    {
        RefreshStatusEffects();
        return;
    }

    UnbindFromComponent();
    StatusEffectComponent = InStatusEffectComponent;
    BindToComponent();
    RefreshStatusEffects();
}

void UNiosCore_StatusEffectGridWidget::RefreshStatusEffects()
{
    TArray<FNiosReplicatedStatusEffect> Effects;

    if (UNiosCore_StatusEffectComponent* Component = StatusEffectComponent)
    {
        if (bShowPersonalEffects)
        {
            Effects.Append(Component->GetPersonalStatusEffects());
        }

        if (bShowPublicEffects)
        {
            Effects.Append(Component->GetPublicStatusEffects());
        }
    }

    BuildFromEffects(Effects);
    BP_OnStatusEffectsRefreshed();
}

void UNiosCore_StatusEffectGridWidget::ClearStatusEffects()
{
    if (EffectsGrid)
    {
        EffectsGrid->ClearChildren();
    }

    if (EffectsWrapBox)
    {
        EffectsWrapBox->ClearChildren();
    }

    LastInputEffectCount = 0;
    LastVisibleEffectCount = 0;
}

void UNiosCore_StatusEffectGridWidget::HandlePublicStatusEffectsChanged(const TArray<FNiosReplicatedStatusEffect>& Effects)
{
    RefreshStatusEffects();
}

void UNiosCore_StatusEffectGridWidget::HandlePersonalStatusEffectsChanged(const TArray<FNiosReplicatedStatusEffect>& Effects)
{
    RefreshStatusEffects();
}

void UNiosCore_StatusEffectGridWidget::BindToComponent()
{
    if (UNiosCore_StatusEffectComponent* Component = StatusEffectComponent)
    {
        Component->OnPublicStatusEffectsChanged.AddUniqueDynamic(this, &UNiosCore_StatusEffectGridWidget::HandlePublicStatusEffectsChanged);
        Component->OnPersonalStatusEffectsChanged.AddUniqueDynamic(this, &UNiosCore_StatusEffectGridWidget::HandlePersonalStatusEffectsChanged);
    }
}

void UNiosCore_StatusEffectGridWidget::UnbindFromComponent()
{
    if (UNiosCore_StatusEffectComponent* Component = StatusEffectComponent)
    {
        Component->OnPublicStatusEffectsChanged.RemoveDynamic(this, &UNiosCore_StatusEffectGridWidget::HandlePublicStatusEffectsChanged);
        Component->OnPersonalStatusEffectsChanged.RemoveDynamic(this, &UNiosCore_StatusEffectGridWidget::HandlePersonalStatusEffectsChanged);
    }
}

void UNiosCore_StatusEffectGridWidget::BuildFromEffects(const TArray<FNiosReplicatedStatusEffect>& Effects)
{
    LastInputEffectCount = Effects.Num();

    if (!EffectsGrid && !EffectsWrapBox)
    {
        LastVisibleEffectCount = 0;
        if (bLogRefreshDebug)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT UI REFRESH SKIPPED: No EffectsGrid or EffectsWrapBox bound Widget=%s Component=%s Input=%d"),
                *GetNameSafe(this),
                *GetNameSafe(StatusEffectComponent),
                Effects.Num());
        }
        return;
    }

    if (EffectsGrid)
    {
        EffectsGrid->ClearChildren();
    }

    if (EffectsWrapBox)
    {
        EffectsWrapBox->ClearChildren();
    }

    if (!StatusEffectIconWidgetClass)
    {
        LastVisibleEffectCount = 0;
        if (bLogRefreshDebug)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT UI REFRESH SKIPPED: No IconWidgetClass Widget=%s Component=%s Input=%d"),
                *GetNameSafe(this),
                *GetNameSafe(StatusEffectComponent),
                Effects.Num());
        }
        return;
    }

    TArray<FNiosReplicatedStatusEffect> VisibleEffects;
    for (const FNiosReplicatedStatusEffect& Effect : Effects)
    {
        if (ShouldShowEffect(Effect))
        {
            VisibleEffects.Add(Effect);
        }
    }

    if (bSortByRemainingTime)
    {
        const UWorld* World = GetWorld();
        const float Now = World ? World->GetTimeSeconds() : 0.f;
        VisibleEffects.Sort([Now](const FNiosReplicatedStatusEffect& A, const FNiosReplicatedStatusEffect& B)
        {
            const float RemainingA = A.EndTime > 0.f ? A.EndTime - Now : TNumericLimits<float>::Max();
            const float RemainingB = B.EndTime > 0.f ? B.EndTime - Now : TNumericLimits<float>::Max();
            return RemainingA < RemainingB;
        });
    }

    LastVisibleEffectCount = VisibleEffects.Num();

    if (bLogRefreshDebug)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT UI REFRESH: Widget=%s Component=%s Public=%d Personal=%d Input=%d Visible=%d UsingWrap=%d UsingGrid=%d"),
            *GetNameSafe(this),
            *GetNameSafe(StatusEffectComponent),
            StatusEffectComponent ? StatusEffectComponent->GetPublicStatusEffects().Num() : 0,
            StatusEffectComponent ? StatusEffectComponent->GetPersonalStatusEffects().Num() : 0,
            Effects.Num(),
            VisibleEffects.Num(),
            EffectsWrapBox ? 1 : 0,
            EffectsGrid ? 1 : 0);
    }

    const int32 SafeColumnCount = FMath::Max(1, ColumnCount);
    int32 AddedIndex = 0;
    for (const FNiosReplicatedStatusEffect& Effect : VisibleEffects)
    {
        UNiosCore_StatusEffectIconWidget* IconWidget = CreateWidget<UNiosCore_StatusEffectIconWidget>(GetOwningPlayer(), StatusEffectIconWidgetClass);
        if (!IconWidget)
        {
            continue;
        }

        IconWidget->SetStatusEffectData(Effect);

        if (EffectsWrapBox)
        {
            EffectsWrapBox->AddChildToWrapBox(IconWidget);
        }
        else if (EffectsGrid)
        {
            const int32 Row = AddedIndex / SafeColumnCount;
            const int32 Column = AddedIndex % SafeColumnCount;
            EffectsGrid->AddChildToUniformGrid(IconWidget, Row, Column);
        }

        ++AddedIndex;
    }
}

bool UNiosCore_StatusEffectGridWidget::ShouldShowEffect(const FNiosReplicatedStatusEffect& Effect) const
{
    if (Effect.EffectID.IsNone())
    {
        return false;
    }

    if (Effect.Visibility == ENiosStatusEffectVisibility::Personal && !bShowPersonalEffects)
    {
        return false;
    }

    if (Effect.Visibility == ENiosStatusEffectVisibility::Public && !bShowPublicEffects)
    {
        return false;
    }

    return true;
}

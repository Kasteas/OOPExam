#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/Stats/NiosCore_StatTypes.h"
#include "NiosCore_StatsWidget.generated.h"

class UNiosCore_StatsComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosStatsWidgetDataChanged, const TArray<FNiosCalculatedStat>&, Stats);

/**
 * Lightweight BP-friendly widget bridge.
 * Standalone BP-friendly stats panel. It auto-binds to the owning PlayerState StatsComponent,
 * but can still be manually bound via BindStatsComponent if needed.
 */
UCLASS()
class NIOS_CORE_API UNiosCore_StatsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeDestruct() override;
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void BindStatsComponent(UNiosCore_StatsComponent* InStatsComponent);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void RefreshStats();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats")
    TArray<FNiosCalculatedStat> GetStats() const;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Stats")
    FNiosStatsWidgetDataChanged OnStatsWidgetDataChanged;

protected:
    UFUNCTION()
    void HandleStatsChanged(const TArray<FNiosCalculatedStat>& NewStats);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Stats")
    TObjectPtr<UNiosCore_StatsComponent> StatsComponent = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Stats")
    TArray<FNiosCalculatedStat> CachedStats;
};

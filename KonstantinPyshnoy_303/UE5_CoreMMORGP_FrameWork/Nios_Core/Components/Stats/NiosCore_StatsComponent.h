#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Stats/NiosCore_StatTypes.h"
#include "NiosCore_StatsComponent.generated.h"

class UNiosCore_AbilitySystemComponent;
class UNiosCore_StatScalingDataAsset;
class UNiosCore_ClassStatsProfileDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNiosStatsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosCalculatedStatsChanged, const TArray<FNiosCalculatedStat>&, CalculatedStats);

UCLASS(ClassGroup = (Nios), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_StatsComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_StatsComponent();

protected:
    virtual void BeginPlay() override;

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Temporary class/base-stats profile. Later backend/class system can choose it by ClassID. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Class")
    TObjectPtr<UNiosCore_ClassStatsProfileDataAsset> ClassStatsProfile = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Class")
    int32 CurrentLevel = 1;

    /** Built-in conversion rates. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float HealthPerStamina = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float ManaPerIntelligence = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float EnergyPerAgility = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float RagePerStrength = 5.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float PhysicalCritPerAgility = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float MagicCritPerIntelligence = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float PhysicalDefensePerConstitution = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float MagicDefensePerSpirit = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float PhysicalModifierPerStrength = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float PhysicalModifierPerAgility = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float MagicModifierPerIntelligence = 1.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats|Derived")
    float MagicModifierPerSpirit = 0.5f;

    /** Optional extra formulas authored by designers. Applied after built-in formulas. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    TObjectPtr<UNiosCore_StatScalingDataAsset> ScalingData = nullptr;

    /** Replicated UI-friendly final/breakdown stats. */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CalculatedStats, Category = "Nios|Stats")
    TArray<FNiosCalculatedStat> CalculatedStats;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Stats")
    FNiosStatsChanged OnStatsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Stats")
    FNiosCalculatedStatsChanged OnCalculatedStatsChanged;

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void InitializeAbilitySystemComponent(UNiosCore_AbilitySystemComponent* InASC);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats|Class")
    void SetClassStatsProfile(UNiosCore_ClassStatsProfileDataAsset* InProfile);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats|Class")
    void SetCurrentLevel(int32 InLevel);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Class")
    TArray<FNiosStatModifier> GetDefaultStats() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats|Class")
    void ApplyClassBaseStats();

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void SetBaseStat(ENiosStatType StatType, float Value);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void AddOrReplaceModifierSource(const FNiosStatModifierSource& Source);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void RemoveModifierSource(FName SourceID);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void RemoveModifierSourcesByLayer(ENiosStatModifierLayer Layer);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void SetCurrentResource(ENiosStatType ResourceStat, float NewValue);

    /**
     * Updates only the runtime source for a current resource.
     * Used by ASC/effect pipeline after authoritative Health/Mana/Energy/Rage writes so later stat recalculations
     * preserve the real current value instead of refilling from stale runtime sources.
     */
    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void SetCurrentResourceRuntimeValue(ENiosStatType ResourceStat, float NewValue, bool bRecalculate = true);

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void RecalculateStats();

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void PushFinalStatsToASC();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats")
    bool GetCalculatedStat(ENiosStatType StatType, FNiosCalculatedStat& OutStat) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats")
    float GetFinalStat(ENiosStatType StatType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats")
    TArray<FNiosCalculatedStat> GetCalculatedStats() const;

    /** UI helpers based on the currently active class profile/resource flags. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasMana() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasEnergy() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasRage() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasHealth() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasResource(ENiosStatType ResourceStat) const;

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Stats")
    void BP_OnStatsInitialized();

    /** True after explicit InitializeStatsComponent() was called. Prevents double bootstrap on server/BP. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Stats")
    bool bStatsComponentInitialized = false;

    /** True after BP_OnStatsInitialized has been fired locally. Server fires after calculate, client fires after first OnRep. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Stats")
    bool bStatsReadyEventCalled = false;

    UFUNCTION(BlueprintCallable, Category = "Nios|Stats")
    void InitializeStatsComponent();
protected:
    UFUNCTION()
    void OnRep_CalculatedStats();

    UNiosCore_AbilitySystemComponent* ResolveASC() const;

    void AddValueToCalculatedStat(ENiosStatType StatType, ENiosStatModifierLayer Layer, float Value);
    FNiosCalculatedStat& FindOrAddCalculatedStat(ENiosStatType StatType);
    FNiosCalculatedStat* FindCalculatedStat(ENiosStatType StatType);

    void ApplyBuiltInDerivedStats();
    void ApplyScalingRules();
    void ApplyResourceGates();
    void InitializeRuntimeResourcesFromMax();
    void ClampCurrentResources();
    float GetLastKnownMaxForResource(ENiosStatType ResourceStat, float CurrentMaxValue) const;
    void StoreLastKnownMaxForResource(ENiosStatType ResourceStat, float CurrentMaxValue);
    void EnsureAllKnownStatsExist();
    void FinalizeCalculatedStats();
    void BroadcastStatsChanged();

    bool IsResourceEnabled(ENiosStatType ResourceStat) const;
    ENiosStatType GetMaxStatForResource(ENiosStatType ResourceStat) const;
    void ResetStatToRuntimeValue(ENiosStatType StatType, float RuntimeValue);
    void ZeroStat(ENiosStatType StatType);
    void AddRuntimeSource(ENiosStatType StatType, float Value);
    bool HasRuntimeSource(ENiosStatType StatType) const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_AbilitySystemComponent> AbilitySystemComponent = nullptr;

    UPROPERTY()
    FNiosResourceFlags ActiveResourceFlags;

    /** Server/runtime sources. Usually not directly used by UI. */
    UPROPERTY()
    TMap<FName, FNiosStatModifierSource> ModifierSources;

    /** Local runtime cache used to preserve current resource max values between recalculations. */
    UPROPERTY(Transient)
    TMap<ENiosStatType, float> LastKnownResourceMaxValues;

    /** First server-side resources bootstrap: fill Health/Mana/Energy to max, Rage to zero. */
    UPROPERTY(Transient)
    bool bRuntimeResourcesInitialized = false;
};

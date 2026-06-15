#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/StatusEffects/NiosCore_StatusEffectTypes.h"
#include "Engine/Texture2D.h"
#include "NiosCore_StatusEffectComponent.generated.h"

class UAbilitySystemComponent;
class UNiosCore_StatsComponent;
class UNiosCore_StatusEffectDataAsset;
class UNiosCore_StatusEffectComponent;
struct FNiosStatusEffectCatalogEntry;
struct FNiosStatusEffectOperation;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNiosStatusEffectsChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosReplicatedStatusEffectsChanged, const TArray<FNiosReplicatedStatusEffect>&, Effects);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosActiveCrowdControlChanged, const FNiosReplicatedStatusEffect&, CrowdControlEffect);

USTRUCT()
struct FNiosActiveAuraTarget
{
    GENERATED_BODY()

    UPROPERTY()
    TWeakObjectPtr<UNiosCore_StatusEffectComponent> TargetComponent;

    UPROPERTY()
    FName GrantedStatusEffectID = NAME_None;

    UPROPERTY()
    FName AuraSourceID = NAME_None;
};

USTRUCT()
struct FNiosActiveStatusEffect
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UNiosCore_StatusEffectDataAsset> Effect = nullptr;

    UPROPERTY()
    FName EffectID = NAME_None;

    UPROPERTY()
    FText DisplayName;

    UPROPERTY()
    FText Description;

    UPROPERTY()
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY()
    FGuid RuntimeID;

    UPROPERTY()
    ENiosStatusEffectSourceType SourceType = ENiosStatusEffectSourceType::Script;

    UPROPERTY()
    TWeakObjectPtr<AActor> SourceActor;

    UPROPERTY()
    FName SourceID = NAME_None;

    UPROPERTY()
    int32 StackCount = 1;

    UPROPERTY()
    float StartTime = 0.f;

    UPROPERTY()
    float EndTime = 0.f;

    UPROPERTY()
    float NextTickTime = 0.f;

    UPROPERTY()
    TArray<FName> AppliedStatSourceIDs;

    /** Snapshot operation values after source/caster scaling. Indices match Effect->Operations. */
    UPROPERTY()
    TArray<float> RuntimeOperationValues;

    /** Per-operation next scan/tick timestamps. Used by Aura operations so all effects run from one component tick. */
    UPROPERTY()
    TArray<float> RuntimeOperationNextTimes;

    /** Child effects currently applied by Aura operations. Removed on exit or when this parent effect ends. */
    UPROPERTY()
    TArray<FNiosActiveAuraTarget> ActiveAuraTargets;

    UPROPERTY()
    FGameplayTagContainer AppliedGrantedTags;

    UPROPERTY()
    FGameplayTagContainer AppliedRemovedTags;
};

UCLASS(ClassGroup = (Nios), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_StatusEffectComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_StatusEffectComponent();

    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Hidden/passive effects applied by ID through StatusEffectCatalog. */
    /** Visual/presentation profile used by client visual systems, e.g. Humanoid, Quadruped, Giant. Not a race. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|StatusEffects|Presentation")
    FName PresentationProfileID = FName(TEXT("Humanoid"));

    /** Hidden/passive effects applied by ID through StatusEffectCatalog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffects|Startup")
    TArray<FName> StartupStatusEffectIDs;

    /** Legacy transition fallback. Prefer StartupStatusEffectIDs + EffectCatalog. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffects|Startup", meta = (AdvancedDisplay))
    TArray<TObjectPtr<UNiosCore_StatusEffectDataAsset>> LegacyStartupStatusEffects;

    UPROPERTY(BlueprintAssignable, Category = "Nios|StatusEffects")
    FNiosStatusEffectsChanged OnStatusEffectsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Nios|StatusEffects")
    FNiosReplicatedStatusEffectsChanged OnPublicStatusEffectsChanged;

    UPROPERTY(BlueprintAssignable, Category = "Nios|StatusEffects")
    FNiosReplicatedStatusEffectsChanged OnPersonalStatusEffectsChanged;

    /** Fires on clients/server when the highest priority visible crowd-control effect changes. Visual layer can start/stop loop montages from this. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|StatusEffects|CrowdControl")
    FNiosActiveCrowdControlChanged OnActiveCrowdControlChanged;

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    void InitializeStatusEffectComponent(UAbilitySystemComponent* InASC, UNiosCore_StatsComponent* InStatsComponent);

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    FGuid ApplyStatusEffectByID(FName EffectID, AActor* SourceActor, ENiosStatusEffectSourceType SourceType, FName SourceID);

    /** Legacy transition fallback. Prefer ApplyStatusEffectByID. */
    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects", meta = (AdvancedDisplay))
    FGuid ApplyStatusEffect(UNiosCore_StatusEffectDataAsset* Effect, AActor* SourceActor, ENiosStatusEffectSourceType SourceType, FName SourceID);

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    bool RemoveStatusEffect(FGuid RuntimeID);

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    int32 RemoveStatusEffectsBySource(ENiosStatusEffectSourceType SourceType, FName SourceID);

    /**
     * Server-authoritative death/revive cleanup. Removes all runtime effects and rebuilds only
     * startup/passive effects. Equipment effects should be rebuilt by EquipmentComponent after this call.
     */
    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects|Lifecycle")
    void RefreshPersistentEffectsAfterRevive();

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects")
    const TArray<FNiosReplicatedStatusEffect>& GetPublicStatusEffects() const { return PublicStatusEffects; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects")
    const TArray<FNiosReplicatedStatusEffect>& GetPersonalStatusEffects() const { return PersonalStatusEffects; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects")
    void GetVisibleStatusEffects(bool bIncludePersonal, bool bIncludePublic, TArray<FNiosReplicatedStatusEffect>& OutEffects) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects")
    int32 GetVisibleStatusEffectCount(bool bIncludePersonal = true, bool bIncludePublic = true) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|Presentation")
    FName GetPresentationProfileID() const { return PresentationProfileID; }

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects|Presentation")
    void SetPresentationProfileID(FName InPresentationProfileID) { PresentationProfileID = InPresentationProfileID; }

    /** Returns highest-priority visible CC effect. Empty EffectID means no active CC. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|StatusEffects|CrowdControl")
    FNiosReplicatedStatusEffect GetHighestPriorityCrowdControlEffect(bool bIncludePersonal = true, bool bIncludePublic = true) const;

protected:
    UFUNCTION()
    void OnRep_PublicStatusEffects();

    UFUNCTION()
    void OnRep_PersonalStatusEffects();

    void ApplyStartupStatusEffects();
    void ApplyInstantOperations(const FNiosActiveStatusEffect& ActiveEffect);
    void ApplyPersistentOperations(FNiosActiveStatusEffect& ActiveEffect);
    void RemovePersistentOperations(FNiosActiveStatusEffect& ActiveEffect);
    void TickPeriodicOperations(FNiosActiveStatusEffect& ActiveEffect);
    void TickAuraOperations(FNiosActiveStatusEffect& ActiveEffect);
    void CleanupAuraTargets(FNiosActiveStatusEffect& ActiveEffect);
    void InitializeOperationRuntimeState(FNiosActiveStatusEffect& ActiveEffect, float Now);
    AActor* ResolveAuraOriginActor() const;
    UNiosCore_StatusEffectComponent* ResolveStatusEffectComponentFromActor(AActor* Actor) const;
    bool IsActorAllowedForAura(AActor* AuraOriginActor, AActor* CandidateActor, const FNiosStatusEffectOperation& Operation) const;
    FName MakeAuraSourceID(const FNiosActiveStatusEffect& ActiveEffect, const FNiosStatusEffectOperation& Operation, int32 OperationIndex) const;
    bool HasAuraAppliedToTarget(const FNiosActiveStatusEffect& ActiveEffect, UNiosCore_StatusEffectComponent* TargetComponent, FName AuraSourceID) const;
    bool ValidateAuraGrantedEffect(const FNiosActiveStatusEffect& ActiveEffect, const FNiosStatusEffectOperation& Operation, const FNiosStatusEffectCatalogEntry& GrantedEntry) const;
    bool DoesEffectContainAuraOperation(const UNiosCore_StatusEffectDataAsset* Effect) const;
    void RebuildReplicatedStatusEffects();
    void BroadcastActiveCrowdControlChanged();
    void RemoveExpiredEffects(float Now);
    void BuildRuntimeOperationValues(FNiosActiveStatusEffect& ActiveEffect) const;
    float GetRuntimeOperationValue(const FNiosActiveStatusEffect& ActiveEffect, int32 OperationIndex) const;
    UAbilitySystemComponent* ResolveSourceAbilitySystemComponent(const FNiosActiveStatusEffect& ActiveEffect) const;
    void RefreshDuration(FNiosActiveStatusEffect& ActiveEffect, float Now) const;
    FNiosReplicatedStatusEffect MakeReplicatedEffect(const FNiosActiveStatusEffect& ActiveEffect) const;
    bool ResolveCatalogEntry(FName EffectID, FNiosStatusEffectCatalogEntry& OutEntry) const;
    FGuid ApplyStatusEffectInternal(UNiosCore_StatusEffectDataAsset* Effect, FName EffectID, const FText& DisplayName, const FText& Description, TSoftObjectPtr<UTexture2D> Icon, AActor* SourceActor, ENiosStatusEffectSourceType SourceType, FName SourceID);
    FName MakeRuntimeSourceID(const FNiosActiveStatusEffect& ActiveEffect, const FNiosStatusEffectOperation& Operation) const;

    FNiosActiveStatusEffect* FindStackableEffect(FName EffectID, UNiosCore_StatusEffectDataAsset* Effect, ENiosStatusEffectSourceType SourceType, FName SourceID);

protected:
    UPROPERTY(Transient)
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_StatsComponent> StatsComponent = nullptr;

    UPROPERTY()
    TArray<FNiosActiveStatusEffect> ActiveEffects;

    UPROPERTY(ReplicatedUsing = OnRep_PublicStatusEffects)
    TArray<FNiosReplicatedStatusEffect> PublicStatusEffects;

    UPROPERTY(ReplicatedUsing = OnRep_PersonalStatusEffects)
    TArray<FNiosReplicatedStatusEffect> PersonalStatusEffects;

    UPROPERTY(Transient)
    bool bStartupEffectsApplied = false;
};

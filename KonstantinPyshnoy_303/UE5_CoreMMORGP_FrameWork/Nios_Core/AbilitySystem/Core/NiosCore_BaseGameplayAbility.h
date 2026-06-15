#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "TimerManager.h"
#include "AbilitySystem/Skills/NiosCore_SkillEffectTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillDeliveryTypes.h"
#include "NiosCore_BaseGameplayAbility.generated.h"

class UNiosCore_SkillDataAsset;
class UNiosCore_AbilitySystemComponent;
class UAbilityTask_WaitDelay;
class UAbilitySystemComponent;
class UAudioComponent;
class UNiagaraSystem;
struct FNiosCoreSkillPipeline;

UCLASS()
class NIOS_CORE_API UNiosCore_BaseGameplayAbility : public UGameplayAbility
{
    GENERATED_BODY()

    friend struct FNiosCoreSkillPipeline;

public:
    UNiosCore_BaseGameplayAbility();

    UFUNCTION(BlueprintCallable)
    FName GetSkillID() const;

    UFUNCTION(BlueprintCallable)
    UNiosCore_SkillDataAsset* GetSkillData() const;

    const FGameplayAbilityTargetDataHandle& GetCachedTargetData() const;

    /** First actor locked in CachedTargetData, useful for UI/debug/pipeline feedback. */
    AActor* GetPrimaryActorFromCachedTargetData() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    AActor* GetCastLockedTarget() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Target")
    AActor* GetEffectiveLookAtTarget() const;

protected:
    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData
    ) override;

    virtual void CancelAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateCancelAbility
    ) override;

    UNiosCore_AbilitySystemComponent* GetNiosASC() const;

    bool BuildTargetDataFromSelectedTarget();
    bool ValidateCachedTargetData() const;
    AActor* GetLockedOrSelectedTarget() const;

    bool CommitAbilityOnce();
    bool ValidateSkillTags() const;
    bool CanPaySkillCosts() const;
    void PaySkillCosts();
    void AddGrantedTagsWhileActive();
    void RemoveGrantedTagsWhileActive();

    void StartCast();

    UFUNCTION()
    void OnCastFinished();

    void BeginCastInterruptWatch();
    void StopCastInterruptWatch();
    void CheckCastInterruptConditions();
    void InterruptCast();
    void InterruptCastWithReason(FName Reason);

    void ExecuteAbility();
    void FinishAbilityAfterGameplay();

    void PlayCastLoopVisuals();
    void StopCastLoopVisuals();

    void ApplySkillGameplay();
    void ApplySkillEffects();
    bool ShouldUseCustomExecutions() const;
    void ExecuteCustomSkillExecutions();
    void ExecuteSkillDeliveries();
    void BeginAsyncDeliveryImpact();
    void FinishAsyncDeliveryImpact();
    void ExecuteSkillDelivery(const FNiosSkillDeliveryDefinition& Delivery);
    void ExecuteSkillDeliveryImpact(FNiosSkillDeliveryDefinition Delivery);
    void ExecuteSkillEffectsByIndices(const TArray<int32>& EffectIndices);
    void ExecuteSkillEffectsByIndicesWithTargetOverride(const TArray<int32>& EffectIndices, const TArray<UAbilitySystemComponent*>& OverrideTargetASCs);
    FVector ResolveSkillPoint(ENiosSkillPointSource PointSource) const;
    void ResolveAreaTargetASCs(const FVector& Point, float Radius, int32 MaxTargets, TArray<UAbilitySystemComponent*>& OutTargetASCs) const;
    bool TeleportAvatarToPointSafe(const FVector& GroundPoint, const FNiosSkillDeliveryDefinition& Delivery);
    void SpawnDeliveryNiagaraSoft(const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem, const FVector& Location, const FRotator& Rotation, FName DebugName) const;
    void ExecuteDeliveryGameplayCue(FGameplayTag CueTag, const FVector& CueLocation);
    FGameplayTag ResolveDefaultCueTag(FName CueEvent) const;
    FGameplayTag ResolveCastLoopCueTag() const;
    FGameplayTag ResolveExecuteCueTag() const;
    FGameplayTag ResolveImpactCueTag() const;
    FGameplayTag ResolveFailCueTag() const;
    void ExecuteImpactGameplayCue(UAbilitySystemComponent* TargetASC, const FNiosSkillEffectDefinition& Effect);

    float CalculateSkillEffectValue(const FNiosSkillEffectDefinition& Effect) const;
    void ResolveSkillEffectTargetASCs(const FNiosSkillEffectDefinition& Effect, TArray<UAbilitySystemComponent*>& OutTargetASCs) const;
    void ApplySkillEffectToASC(const FNiosSkillEffectDefinition& Effect, UAbilitySystemComponent* TargetASC);

    void AddCastLoopGameplayCue();
    void RemoveCastLoopGameplayCue();
    void ExecuteSkillGameplayCue();

    void EndAbilityInternal();

    void PlayExecuteVisuals();

    UFUNCTION()
    void OnTargetDataReady(const FGameplayAbilityTargetDataHandle& Data);

    UFUNCTION()
    void OnTargetDataCancelled(const FGameplayAbilityTargetDataHandle& Data);

protected:
    FGameplayAbilitySpecHandle CachedSpecHandle;
    FGameplayAbilityActivationInfo CachedActivationInfo;
    FGameplayAbilityTargetDataHandle CachedTargetData;

    UPROPERTY()
    TObjectPtr<UAbilityTask_WaitDelay> CastDelayTask;

    UPROPERTY()
    TObjectPtr<UAudioComponent> ActiveCastLoopAudioComponent;

    FTimerHandle CastInterruptPollTimerHandle;
    FVector CastStartLocation = FVector::ZeroVector;
    FName CastInterruptReason = NAME_None;

    bool bWasCancelled = false;
    bool bCastInterruptWatchActive = false;
    bool bGameplayCueExecuted = false;
    bool bCastLoopCueAdded = false;
    bool bAbilityCommitted = false;
    bool bGrantedActiveTagsAdded = false;
    bool bGameplayExecutionStarted = false;
    bool bSkillFeedbackFinalized = false;
    int32 PendingDeliveryImpacts = 0;
};
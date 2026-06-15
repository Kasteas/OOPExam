#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "NiosCore_SkillVisualDispatcher.generated.h"

class UAbilitySystemComponent;
class UAudioComponent;
class UNiosCore_SkillDataAsset;
class UNiagaraSystem;
struct FNiosSkillEffectDefinition;

/**
 * Visual-only helper for skill feedback.
 *
 * GameplayAbility owns the lifecycle, but this dispatcher owns how cast/execute/impact
 * visuals are resolved and sent to clients. This keeps BaseGameplayAbility from becoming
 * a montage/sound/cue god object and gives us a clean place for future prediction/cue payloads.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosSkillVisualContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> SourceActor = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UNiosCore_SkillDataAsset> Skill = nullptr;

    UPROPERTY(BlueprintReadOnly)
    bool bHasAuthority = false;
};

class NIOS_CORE_API FNiosCoreSkillVisualDispatcher
{
public:
    static void PlayCastLoopVisuals(const FNiosSkillVisualContext& Context, bool bStart, TObjectPtr<UAudioComponent>& ActiveCastLoopAudioComponent);
    static void PlayExecuteVisuals(const FNiosSkillVisualContext& Context);

    static FGameplayTag ResolveDefaultCueTag(const UNiosCore_SkillDataAsset* Skill, FName CueEvent);
    static FGameplayTag ResolveCastLoopCueTag(const UNiosCore_SkillDataAsset* Skill);
    static FGameplayTag ResolveExecuteCueTag(const UNiosCore_SkillDataAsset* Skill);
    static FGameplayTag ResolveImpactCueTag(const UNiosCore_SkillDataAsset* Skill);
    static FGameplayTag ResolveFailCueTag(const UNiosCore_SkillDataAsset* Skill);

    static void AddCastLoopGameplayCue(const FNiosSkillVisualContext& Context, bool& bCastLoopCueAdded);
    static void RemoveCastLoopGameplayCue(const FNiosSkillVisualContext& Context, bool& bCastLoopCueAdded);
    static void ExecuteSkillGameplayCue(const FNiosSkillVisualContext& Context, bool& bGameplayCueExecuted);
    static void ExecuteDeliveryGameplayCue(const FNiosSkillVisualContext& Context, FGameplayTag CueTag, const FVector& CueLocation);
    static void DispatchDeliveryNiagara(const FNiosSkillVisualContext& Context, const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem, const FVector& Location, const FRotator& Rotation, FName DebugName);
    static void ExecuteImpactGameplayCue(const FNiosSkillVisualContext& Context, UAbilitySystemComponent* TargetASC, const FNiosSkillEffectDefinition& Effect, float RawMagnitude);
};

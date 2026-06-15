#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillEffectTypes.h"
#include "NiosCore_SkillPipeline.generated.h"

class UNiosCore_BaseGameplayAbility;
class UNiosCore_SkillDataAsset;
class UNiosCore_AbilitySystemComponent;
class UAbilitySystemComponent;
class AActor;
class UNiosCore_CooldownComponent;

UENUM(BlueprintType)
enum class ENiosSkillPipelineFailure : uint8
{
    None,
    MissingSkillData,
    MissingASC,
    RequiredTagsMissing,
    BlockedTagsPresent,
    NotEnoughResources,
    GlobalCooldownActive,
    SkillCooldownActive,
    MissingTarget,
    InvalidTarget,
    TargetOutOfRange,
    SourceDead,
    TargetDead,
    CommitFailed,
    Cancelled,
    Interrupted,
    RequirementFailed,
    RequirementHandledByAbility
};

/**
 * Runtime snapshot for a skill activation.
 * This is intentionally lightweight and does not own UObject references.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosSkillPipelineContext
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UNiosCore_SkillDataAsset> Skill = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UAbilitySystemComponent> SourceASC = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UNiosCore_AbilitySystemComponent> SourceNiosASC = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> SourceActor = nullptr;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<AActor> SelectedActor = nullptr;

    UPROPERTY(BlueprintReadOnly)
    bool bHasTargetLocation = false;

    UPROPERTY(BlueprintReadOnly)
    FVector TargetLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    bool bHasAuthority = false;

    UPROPERTY(BlueprintReadOnly)
    ENiosSkillPipelineFailure FailureReason = ENiosSkillPipelineFailure::None;

    bool IsValid() const
    {
        return Skill != nullptr && SourceASC != nullptr && SourceActor != nullptr;
    }
};

/**
 * Orchestrates the generic skill lifecycle.
 *
 * BaseGameplayAbility remains the GAS entry point; this struct owns the order of stages:
 * Activate -> Validate -> Cast/Execute -> Commit -> Visuals -> Gameplay -> Finish.
 * Target resolving, effect processing and visual dispatching stay in their own dedicated helpers.
 */
struct NIOS_CORE_API FNiosCoreSkillPipeline
{
    static FNiosSkillPipelineContext BuildContext(const UNiosCore_BaseGameplayAbility& Ability);

    static void Activate(
        UNiosCore_BaseGameplayAbility& Ability,
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo);

    static void OnCastFinished(UNiosCore_BaseGameplayAbility& Ability);
    static void Execute(UNiosCore_BaseGameplayAbility& Ability);
    static void FinishAfterGameplay(UNiosCore_BaseGameplayAbility& Ability);
    static void HandleTargetDataReady(UNiosCore_BaseGameplayAbility& Ability, const FGameplayAbilityTargetDataHandle& Data);

private:
    static bool ValidateActivation(UNiosCore_BaseGameplayAbility& Ability, FNiosSkillPipelineContext& Context);
    static bool PrepareTargetData(UNiosCore_BaseGameplayAbility& Ability, FNiosSkillPipelineContext& Context);
    static void FailActivation(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context, bool bReplicateEnd = true);
    static void FailExecution(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context, bool bReplicateEnd = true);
    static void InterruptActivation(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context, bool bReplicateEnd = true);
    static UNiosCore_CooldownComponent* ResolveCooldownComponent(const FNiosSkillPipelineContext& Context);
    static FName ResolveSkillCooldownID(const UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context);
    static bool CheckCooldowns(UNiosCore_BaseGameplayAbility& Ability, FNiosSkillPipelineContext& Context);
    static void StartCooldownsAfterSuccessfulExecute(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context);
    static void ExecuteGroundPointFallbackVisualIfNeeded(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context);
    static bool ExtractGroundTargetLocation(const FGameplayAbilityTargetDataHandle& TargetData, FVector& OutLocation);
    static const TCHAR* FailureToString(ENiosSkillPipelineFailure Failure);
};

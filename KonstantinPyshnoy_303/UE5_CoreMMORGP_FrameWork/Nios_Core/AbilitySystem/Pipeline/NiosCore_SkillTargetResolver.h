#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillEffectTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillDeliveryTypes.h"

class UAbilitySystemComponent;
class UNiosCore_SkillDataAsset;

/**
 * Runtime target context for a single skill activation.
 * This is intentionally small and code-facing: it is not long-lived gameplay state.
 */
struct NIOS_CORE_API FNiosCoreSkillTargetContext
{
    AActor* SourceActor = nullptr;
    UAbilitySystemComponent* SourceASC = nullptr;

    AActor* SelectedActor = nullptr;
    AActor* PrimaryTargetActor = nullptr;
    UAbilitySystemComponent* PrimaryTargetASC = nullptr;

    FVector TargetLocation = FVector::ZeroVector;
    ENiosCoreTargetRelation RelationToSource = ENiosCoreTargetRelation::Neutral;

    bool bIsSelf = false;
    bool bIsValid = false;
    FName FailureReason = NAME_None;
};

/**
 * Single place for skill target interpretation.
 * Ability owns activation; this resolver owns "who/where is the target?".
 */
class NIOS_CORE_API FNiosCoreSkillTargetResolver
{
public:
    static bool BuildActorTargetData(AActor* TargetActor, FGameplayAbilityTargetDataHandle& OutTargetData);

    static FNiosCoreSkillTargetContext ResolveContext(
        UWorld* World,
        UAbilitySystemComponent* SourceASC,
        AActor* SourceActor,
        AActor* SelectedActor,
        const UNiosCore_SkillDataAsset* Skill,
        const FGameplayAbilityTargetDataHandle& TargetData
    );

    static bool ValidateAbilityTarget(
        UWorld* World,
        UAbilitySystemComponent* SourceASC,
        AActor* SourceActor,
        AActor* SelectedActor,
        const UNiosCore_SkillDataAsset* Skill,
        const FGameplayAbilityTargetDataHandle& TargetData,
        FName& OutFailureReason
    );

    /** Runtime distance used by authoritative actor-target range checks. Returns 0 for invalid inputs. */
    static float CalculateActorRangeDistance(
        const AActor* SourceActor,
        const AActor* TargetActor,
        const UNiosCore_SkillDataAsset* Skill
    );

    /** Radius subtracted from origin distance for edge-to-edge range checks. */
    static float ResolveActorRangeRadius(
        const AActor* Actor,
        const UNiosCore_SkillDataAsset* Skill
    );

    static ENiosSkillEffectTarget ResolveEffectiveEffectTarget(
        const UNiosCore_SkillDataAsset* Skill,
        const FNiosSkillEffectDefinition& Effect
    );

    static void ResolveEffectTargetASCs(
        UWorld* World,
        UAbilitySystemComponent* SourceASC,
        AActor* SourceActor,
        AActor* SelectedActor,
        const UNiosCore_SkillDataAsset* Skill,
        const FNiosSkillEffectDefinition& Effect,
        const FGameplayAbilityTargetDataHandle& TargetData,
        TArray<UAbilitySystemComponent*>& OutTargetASCs
    );

    static FVector ResolvePoint(
        AActor* SourceActor,
        AActor* SelectedActor,
        const FGameplayAbilityTargetDataHandle& TargetData,
        ENiosSkillPointSource PointSource
    );
};

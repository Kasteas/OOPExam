#pragma once

#include "CoreMinimal.h"
#include "NiosCore_SkillRequirementResolver.generated.h"

class UNiosCore_SkillDataAsset;
class UAbilitySystemComponent;
class UNiosCore_EquipmentComponent;
class AActor;

UENUM(BlueprintType)
enum class ENiosSkillRequirementResult : uint8
{
    Passed,
    Failed,
    HandledByAbility
};

UENUM(BlueprintType)
enum class ENiosSkillRequirementFailure : uint8
{
    None,
    MissingSourceActor,
    MissingEquipmentComponent,
    WeaponNotEquipped,
    WeaponNotInHands,
    AutoToggleAbilityMissing,
    AutoToggleAbilityFailed
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosSkillRequirementResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ENiosSkillRequirementResult Result = ENiosSkillRequirementResult::Passed;

    UPROPERTY(BlueprintReadOnly)
    ENiosSkillRequirementFailure Failure = ENiosSkillRequirementFailure::None;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<UNiosCore_EquipmentComponent> EquipmentComponent = nullptr;

    UPROPERTY(BlueprintReadOnly)
    FName DebugReason = NAME_None;

    FORCEINLINE bool IsPassed() const { return Result == ENiosSkillRequirementResult::Passed; }
    FORCEINLINE bool IsHandledByAbility() const { return Result == ENiosSkillRequirementResult::HandledByAbility; }
};

/**
 * Type-safe gameplay requirements for SkillDataAsset.
 * Keep hard gameplay requirements as enums/structs here, not as a large gameplay-tag soup.
 *
 * Important MMO ownership rule:
 * - Core gameplay components may live on PlayerState/ASC owner.
 * - Character/Pawn is treated as avatar/visual shell.
 * Therefore equipment lookup must search OwnerActor/PlayerState first and only then avatar.
 */
struct NIOS_CORE_API FNiosCoreSkillRequirementResolver
{
    static FNiosSkillRequirementResult ResolveRequirements(
        const UNiosCore_SkillDataAsset* Skill,
        UAbilitySystemComponent* SourceASC,
        AActor* SourceActor);

    static const TCHAR* FailureToString(ENiosSkillRequirementFailure Failure);

private:
    static UNiosCore_EquipmentComponent* ResolveEquipmentComponent(
        UAbilitySystemComponent* SourceASC,
        AActor* SourceActor);
};

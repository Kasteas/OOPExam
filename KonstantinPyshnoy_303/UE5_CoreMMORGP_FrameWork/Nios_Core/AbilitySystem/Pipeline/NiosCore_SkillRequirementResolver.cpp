#include "AbilitySystem/Pipeline/NiosCore_SkillRequirementResolver.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

namespace
{
    UNiosCore_EquipmentComponent* FindEquipmentOnActor(AActor* Actor)
    {
        return Actor ? Actor->FindComponentByClass<UNiosCore_EquipmentComponent>() : nullptr;
    }
}

UNiosCore_EquipmentComponent* FNiosCoreSkillRequirementResolver::ResolveEquipmentComponent(
    UAbilitySystemComponent* SourceASC,
    AActor* SourceActor)
{
    // 1) ASC owner first. In the Nios MMO architecture this is usually PlayerState.
    if (SourceASC)
    {
        if (UNiosCore_EquipmentComponent* Equipment = FindEquipmentOnActor(SourceASC->GetOwnerActor()))
        {
            return Equipment;
        }
    }

    // 2) Explicit source actor, if it is already the PlayerState/owner actor.
    if (UNiosCore_EquipmentComponent* Equipment = FindEquipmentOnActor(SourceActor))
    {
        return Equipment;
    }

    // 3) Pawn -> PlayerState. Character is an avatar shell, core components can live on PlayerState.
    if (const APawn* Pawn = Cast<APawn>(SourceActor))
    {
        if (UNiosCore_EquipmentComponent* Equipment = FindEquipmentOnActor(Pawn->GetPlayerState()))
        {
            return Equipment;
        }

        if (const AController* Controller = Pawn->GetController())
        {
            if (UNiosCore_EquipmentComponent* Equipment = FindEquipmentOnActor(Controller->PlayerState))
            {
                return Equipment;
            }
        }
    }

    // 4) Controller -> PlayerState fallback.
    if (const AController* Controller = Cast<AController>(SourceActor))
    {
        if (UNiosCore_EquipmentComponent* Equipment = FindEquipmentOnActor(Controller->PlayerState))
        {
            return Equipment;
        }
    }

    // 5) Actor owner fallback.
    if (SourceActor)
    {
        if (UNiosCore_EquipmentComponent* Equipment = FindEquipmentOnActor(SourceActor->GetOwner()))
        {
            return Equipment;
        }
    }

    // 6) ASC avatar fallback, for AI/world actors where the component may live directly on the actor.
    if (SourceASC)
    {
        if (UNiosCore_EquipmentComponent* Equipment = FindEquipmentOnActor(SourceASC->GetAvatarActor()))
        {
            return Equipment;
        }
    }

    return nullptr;
}

FNiosSkillRequirementResult FNiosCoreSkillRequirementResolver::ResolveRequirements(
    const UNiosCore_SkillDataAsset* Skill,
    UAbilitySystemComponent* SourceASC,
    AActor* SourceActor)
{
    FNiosSkillRequirementResult Result;

    if (!Skill || !Skill->bRequireWeaponInHands)
    {
        return Result;
    }

    if (!SourceActor && !SourceASC)
    {
        Result.Result = ENiosSkillRequirementResult::Failed;
        Result.Failure = ENiosSkillRequirementFailure::MissingSourceActor;
        Result.DebugReason = TEXT("MissingSourceActor");
        return Result;
    }

    UNiosCore_EquipmentComponent* Equipment = ResolveEquipmentComponent(SourceASC, SourceActor);
    Result.EquipmentComponent = Equipment;

    if (!Equipment)
    {
        Result.Result = ENiosSkillRequirementResult::Failed;
        Result.Failure = ENiosSkillRequirementFailure::MissingEquipmentComponent;
        Result.DebugReason = TEXT("MissingEquipmentComponent");

        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL REQUIREMENT FAILED: MissingEquipmentComponent SourceActor=%s ASCOwner=%s ASCAvatar=%s"),
            *GetNameSafe(SourceActor),
            SourceASC ? *GetNameSafe(SourceASC->GetOwnerActor()) : TEXT("None"),
            SourceASC ? *GetNameSafe(SourceASC->GetAvatarActor()) : TEXT("None"));

        return Result;
    }

    EEquipSlot RequiredSlot = EEquipSlot::None;
    FReplicatedEquipmentSlot RequiredSlotData;
    if (!Equipment->ResolveWeaponSlotMatchingTypes(Skill->RequiredWeaponTypes, RequiredSlot, RequiredSlotData))
    {
        Result.Result = ENiosSkillRequirementResult::Failed;
        Result.Failure = ENiosSkillRequirementFailure::WeaponNotEquipped;
        Result.DebugReason = TEXT("WeaponNotEquipped");
        return Result;
    }

    if (Equipment->IsWeaponInHandsMatchingTypes(Skill->RequiredWeaponTypes))
    {
        return Result;
    }

    if (!Skill->bAutoToggleWeaponReadyIfEquipped)
    {
        Result.Result = ENiosSkillRequirementResult::Failed;
        Result.Failure = ENiosSkillRequirementFailure::WeaponNotInHands;
        Result.DebugReason = TEXT("WeaponNotInHands");
        return Result;
    }

    if (!SourceASC || !Skill->AutoToggleWeaponReadyAbilityClass)
    {
        Result.Result = ENiosSkillRequirementResult::Failed;
        Result.Failure = ENiosSkillRequirementFailure::AutoToggleAbilityMissing;
        Result.DebugReason = TEXT("AutoToggleAbilityMissing");
        return Result;
    }

    const bool bActivatedToggle = SourceASC->TryActivateAbilityByClass(Skill->AutoToggleWeaponReadyAbilityClass);
    if (!bActivatedToggle)
    {
        Result.Result = ENiosSkillRequirementResult::Failed;
        Result.Failure = ENiosSkillRequirementFailure::AutoToggleAbilityFailed;
        Result.DebugReason = TEXT("AutoToggleAbilityFailed");
        return Result;
    }

    Result.Result = ENiosSkillRequirementResult::HandledByAbility;
    Result.Failure = ENiosSkillRequirementFailure::WeaponNotInHands;
    Result.DebugReason = TEXT("WeaponReadyingByToggleAbility");

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL REQUIREMENT HANDLED: weapon equipped but not in hands. Activated=%s Slot=%d Item=%s EquipmentOwner=%s"),
        *GetNameSafe(Skill->AutoToggleWeaponReadyAbilityClass.Get()),
        static_cast<int32>(RequiredSlot),
        *RequiredSlotData.ItemID.ToString(),
        *GetNameSafe(Equipment->GetOwner()));

    return Result;
}

const TCHAR* FNiosCoreSkillRequirementResolver::FailureToString(ENiosSkillRequirementFailure Failure)
{
    switch (Failure)
    {
    case ENiosSkillRequirementFailure::None: return TEXT("None");
    case ENiosSkillRequirementFailure::MissingSourceActor: return TEXT("MissingSourceActor");
    case ENiosSkillRequirementFailure::MissingEquipmentComponent: return TEXT("MissingEquipmentComponent");
    case ENiosSkillRequirementFailure::WeaponNotEquipped: return TEXT("WeaponNotEquipped");
    case ENiosSkillRequirementFailure::WeaponNotInHands: return TEXT("WeaponNotInHands");
    case ENiosSkillRequirementFailure::AutoToggleAbilityMissing: return TEXT("AutoToggleAbilityMissing");
    case ENiosSkillRequirementFailure::AutoToggleAbilityFailed: return TEXT("AutoToggleAbilityFailed");
    default: return TEXT("Unknown");
    }
}

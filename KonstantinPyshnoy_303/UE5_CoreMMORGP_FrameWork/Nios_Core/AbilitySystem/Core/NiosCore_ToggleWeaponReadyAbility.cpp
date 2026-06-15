#include "AbilitySystem/Core/NiosCore_ToggleWeaponReadyAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

UNiosCore_ToggleWeaponReadyAbility::UNiosCore_ToggleWeaponReadyAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UNiosCore_ToggleWeaponReadyAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    LastFailureReason = ENiosWeaponToggleFailure::None;
    bDrawing = false;
    bSheathing = false;

    if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
    {
        FailToggle(ENiosWeaponToggleFailure::MissingActorInfo, TEXT("MissingActorInfo"));
        return;
    }

    CachedEquipmentComponent = ResolveEquipmentComponent();
    if (!CachedEquipmentComponent)
    {
        FailToggle(ENiosWeaponToggleFailure::MissingEquipmentComponent, TEXT("MissingEquipmentComponent"));
        return;
    }

    EEquipSlot ActiveSlot = EEquipSlot::None;
    FReplicatedEquipmentSlot ActiveSlotData;
    if (!CachedEquipmentComponent->ResolveWeaponSlotMatchingTypes(TArray<ENiosWeaponType>(), ActiveSlot, ActiveSlotData))
    {
        FailToggle(ENiosWeaponToggleFailure::NoEquippedWeapon, TEXT("NoEquippedWeapon"));
        return;
    }

    const FNiosWeaponVisualState CurrentState = CachedEquipmentComponent->GetWeaponVisualState();
    if (bIgnoreWhileTransitioning &&
        (CurrentState.ReadyState == ENiosWeaponReadyState::Drawing || CurrentState.ReadyState == ENiosWeaponReadyState::Sheathing))
    {
        FailToggle(ENiosWeaponToggleFailure::AlreadyTransitioning, TEXT("AlreadyTransitioning"));
        return;
    }

    if (CurrentState.ReadyState == ENiosWeaponReadyState::InHands)
    {
        bSheathing = true;
        CachedEquipmentComponent->SetWeaponVisualState(ENiosWeaponReadyState::Sheathing, ENiosWeaponVisualSocketState::Hand, ActiveSlot);
        UE_LOG(LogTemp, Warning, TEXT(">>> TOGGLE WEAPON START: Sheathe Slot=%d Item=%s Duration=%.2f"),
            static_cast<int32>(ActiveSlot), *ActiveSlotData.ItemID.ToString(), SheatheDuration);
        BP_OnWeaponToggleStarted(false, ActiveSlot, ActiveSlotData);

        if (SheatheDuration <= KINDA_SMALL_NUMBER)
        {
            FinishToggleNow();
            return;
        }

        ToggleDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, SheatheDuration);
    }
    else
    {
        bDrawing = true;
        const ENiosWeaponVisualSocketState SheathedSocket = CurrentState.SocketState != ENiosWeaponVisualSocketState::None
            ? CurrentState.SocketState
            : CachedEquipmentComponent->GetDefaultSheathedSocketForSlot(ActiveSlot);

        CachedEquipmentComponent->SetWeaponVisualState(ENiosWeaponReadyState::Drawing, SheathedSocket, ActiveSlot);
        UE_LOG(LogTemp, Warning, TEXT(">>> TOGGLE WEAPON START: Draw Slot=%d Item=%s Duration=%.2f"),
            static_cast<int32>(ActiveSlot), *ActiveSlotData.ItemID.ToString(), DrawDuration);
        BP_OnWeaponToggleStarted(true, ActiveSlot, ActiveSlotData);

        if (DrawDuration <= KINDA_SMALL_NUMBER)
        {
            FinishToggleNow();
            return;
        }

        ToggleDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, DrawDuration);
    }

    if (!ToggleDelayTask)
    {
        FinishToggleNow();
        return;
    }

    ToggleDelayTask->OnFinish.AddDynamic(this, &UNiosCore_ToggleWeaponReadyAbility::OnToggleDelayFinished);
    ToggleDelayTask->ReadyForActivation();
}

void UNiosCore_ToggleWeaponReadyAbility::OnToggleDelayFinished()
{
    FinishToggleNow();
}

void UNiosCore_ToggleWeaponReadyAbility::FinishToggleNow()
{
    if (!CachedEquipmentComponent)
    {
        EndToggleAbility(true);
        return;
    }

    EEquipSlot ActiveSlot = EEquipSlot::None;
    FReplicatedEquipmentSlot ActiveSlotData;
    if (!CachedEquipmentComponent->ResolveWeaponSlotMatchingTypes(TArray<ENiosWeaponType>(), ActiveSlot, ActiveSlotData))
    {
        FailToggle(ENiosWeaponToggleFailure::LostWeaponDuringToggle, TEXT("LostWeaponDuringToggle"));
        return;
    }

    if (bDrawing)
    {
        CachedEquipmentComponent->SetWeaponVisualState(ENiosWeaponReadyState::InHands, ENiosWeaponVisualSocketState::Hand, ActiveSlot);
        UE_LOG(LogTemp, Warning, TEXT(">>> TOGGLE WEAPON FINISH: InHands Slot=%d Item=%s"),
            static_cast<int32>(ActiveSlot), *ActiveSlotData.ItemID.ToString());
        BP_OnWeaponToggleFinished(true, ActiveSlot, ActiveSlotData);
    }
    else if (bSheathing)
    {
        CachedEquipmentComponent->SetWeaponVisualState(
            ENiosWeaponReadyState::Sheathed,
            CachedEquipmentComponent->GetDefaultSheathedSocketForSlot(ActiveSlot),
            ActiveSlot);
        UE_LOG(LogTemp, Warning, TEXT(">>> TOGGLE WEAPON FINISH: Sheathed Slot=%d Item=%s"),
            static_cast<int32>(ActiveSlot), *ActiveSlotData.ItemID.ToString());
        BP_OnWeaponToggleFinished(false, ActiveSlot, ActiveSlotData);
    }

    EndToggleAbility(false);
}

void UNiosCore_ToggleWeaponReadyAbility::FailToggle(ENiosWeaponToggleFailure FailureReason, const TCHAR* DebugReason)
{
    LastFailureReason = FailureReason;

    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    AActor* AvatarActor = ActorInfo && ActorInfo->AvatarActor.IsValid() ? ActorInfo->AvatarActor.Get() : nullptr;

    UE_LOG(LogTemp, Warning, TEXT(">>> TOGGLE WEAPON FAILED: Reason=%s Avatar=%s"),
        DebugReason ? DebugReason : TEXT("Unknown"),
        *GetNameSafe(AvatarActor));

    BP_OnWeaponToggleFailed(FailureReason);
    EndToggleAbility(true);
}

void UNiosCore_ToggleWeaponReadyAbility::EndToggleAbility(bool bWasCancelled)
{
    if (ToggleDelayTask)
    {
        ToggleDelayTask->EndTask();
        ToggleDelayTask = nullptr;
    }

    bDrawing = false;
    bSheathing = false;

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

UNiosCore_EquipmentComponent* UNiosCore_ToggleWeaponReadyAbility::ResolveEquipmentComponent() const
{
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
    {
        return nullptr;
    }

    // MMO rule for Nios Core:
    // core gameplay components usually live on PlayerState because pawn/avatar can be swapped.
    // Avatar/Character is mostly a visual and movement shell.
    // Keep avatar lookup as fallback for test actors and future NPC/world entities.

    if (ActorInfo->OwnerActor.IsValid())
    {
        if (UNiosCore_EquipmentComponent* Equipment = ActorInfo->OwnerActor->FindComponentByClass<UNiosCore_EquipmentComponent>())
        {
            return Equipment;
        }
    }

    if (ActorInfo->PlayerController.IsValid())
    {
        if (APlayerState* PlayerState = ActorInfo->PlayerController->PlayerState)
        {
            if (UNiosCore_EquipmentComponent* Equipment = PlayerState->FindComponentByClass<UNiosCore_EquipmentComponent>())
            {
                return Equipment;
            }
        }
    }

    if (ActorInfo->AvatarActor.IsValid())
    {
        if (UNiosCore_EquipmentComponent* Equipment = ActorInfo->AvatarActor->FindComponentByClass<UNiosCore_EquipmentComponent>())
        {
            return Equipment;
        }
    }

    return nullptr;
}

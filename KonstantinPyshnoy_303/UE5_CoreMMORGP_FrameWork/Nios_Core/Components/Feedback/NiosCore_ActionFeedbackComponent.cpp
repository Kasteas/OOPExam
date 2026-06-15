#include "Components/Feedback/NiosCore_ActionFeedbackComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

UNiosCore_ActionFeedbackComponent::UNiosCore_ActionFeedbackComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

UNiosCore_ActionFeedbackComponent* UNiosCore_ActionFeedbackComponent::EnsureOnPlayerController(APlayerController* PlayerController)
{
    if (!PlayerController)
    {
        return nullptr;
    }

    if (UNiosCore_ActionFeedbackComponent* Existing = PlayerController->FindComponentByClass<UNiosCore_ActionFeedbackComponent>())
    {
        return Existing;
    }

    UNiosCore_ActionFeedbackComponent* NewComponent = NewObject<UNiosCore_ActionFeedbackComponent>(PlayerController, TEXT("NiosActionFeedbackComponent"));
    if (!NewComponent)
    {
        return nullptr;
    }

    NewComponent->SetIsReplicated(true);
    PlayerController->AddInstanceComponent(NewComponent);
    NewComponent->RegisterComponent();

    UE_LOG(LogTemp, Warning, TEXT(">>> ACTION FEEDBACK: Added component to PlayerController=%s"), *GetNameSafe(PlayerController));
    return NewComponent;
}

static APlayerController* NiosResolveOwningPlayerController(AActor* SourceActor)
{
    if (!SourceActor)
    {
        return nullptr;
    }

    if (APlayerController* DirectPC = Cast<APlayerController>(SourceActor))
    {
        return DirectPC;
    }

    if (APawn* Pawn = Cast<APawn>(SourceActor))
    {
        return Cast<APlayerController>(Pawn->GetController());
    }

    if (APlayerState* PlayerState = Cast<APlayerState>(SourceActor))
    {
        return PlayerState->GetPlayerController();
    }

    for (AActor* Owner = SourceActor->GetOwner(); Owner; Owner = Owner->GetOwner())
    {
        if (APlayerController* OwnerPC = Cast<APlayerController>(Owner))
        {
            return OwnerPC;
        }

        if (APawn* OwnerPawn = Cast<APawn>(Owner))
        {
            if (APlayerController* OwnerPawnPC = Cast<APlayerController>(OwnerPawn->GetController()))
            {
                return OwnerPawnPC;
            }
        }
    }

    return nullptr;
}

bool UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(AActor* SourceActor, FNiosCoreActionFailResult Result)
{
    if (!Result.IsValidFailure())
    {
        return false;
    }

    if (!Result.SourceActor)
    {
        Result.SourceActor = SourceActor;
    }

    APlayerController* PlayerController = NiosResolveOwningPlayerController(SourceActor);
    if (!PlayerController)
    {
        // AI/server-only actors can fail actions too; that is not a player-facing feedback error.
        UE_LOG(LogTemp, Verbose, TEXT(">>> ACTION FEEDBACK SKIPPED: No owning PC Source=%s Reason=%d Debug=%s"),
            *GetNameSafe(SourceActor), static_cast<int32>(Result.Reason), *Result.DebugReason.ToString());
        return false;
    }

    UNiosCore_ActionFeedbackComponent* FeedbackComponent = EnsureOnPlayerController(PlayerController);
    if (!FeedbackComponent)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> ACTION FEEDBACK FAILED: No component PC=%s Reason=%d Debug=%s"),
            *GetNameSafe(PlayerController), static_cast<int32>(Result.Reason), *Result.DebugReason.ToString());
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> ACTION FEEDBACK SEND: PC=%s Reason=%d Context=%s Debug=%s"),
        *GetNameSafe(PlayerController),
        static_cast<int32>(Result.Reason),
        *Result.ContextId.ToString(),
        *Result.DebugReason.ToString());

    if (PlayerController->IsLocalController())
    {
        FeedbackComponent->HandleActionFailed(Result);
    }
    else
    {
        FeedbackComponent->Client_ReceiveActionFailed(Result);
    }

    return true;
}

void UNiosCore_ActionFeedbackComponent::Client_ReceiveActionFailed_Implementation(FNiosCoreActionFailResult Result)
{
    HandleActionFailed(Result);
}

void UNiosCore_ActionFeedbackComponent::HandleActionFailed(FNiosCoreActionFailResult Result)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> ACTION FEEDBACK LOCAL: Owner=%s Reason=%d Context=%s Required=%.2f Current=%.2f Debug=%s"),
        *GetNameSafe(GetOwner()),
        static_cast<int32>(Result.Reason),
        *Result.ContextId.ToString(),
        Result.RequiredValue,
        Result.CurrentValue,
        *Result.DebugReason.ToString());

    OnActionFailed.Broadcast(Result);
    BP_OnActionFailed(Result);
}

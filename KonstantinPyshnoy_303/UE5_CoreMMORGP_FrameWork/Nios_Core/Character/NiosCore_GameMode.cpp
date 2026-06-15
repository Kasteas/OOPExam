#include "Character/NiosCore_GameMode.h"

#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "Engine/World.h"
#include "Components/Feedback/NiosCore_ActionFeedbackComponent.h"

void ANiosCore_GameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    UE_LOG(LogTemp, Warning, TEXT(">>> GM: PostLogin Player=%s"),
        *GetNameSafe(NewPlayer)
    );

    UNiosCore_ActionFeedbackComponent::EnsureOnPlayerController(NewPlayer);

    // :
    //    pawn .
    // UE   RestartPlayer().
}

void ANiosCore_GameMode::RestartPlayer(AController* NewPlayer)
{
    if (!NewPlayer)
        return;

    UE_LOG(LogTemp, Warning, TEXT(">>> GM: RestartPlayer Controller=%s"),
        *GetNameSafe(NewPlayer)
    );

    if (NewPlayer->GetPawn())
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> GM: Controller already has pawn=%s"),
            *GetNameSafe(NewPlayer->GetPawn())
        );
        return;
    }

    APawn* ReconnectPawn = FindDisconnectedPawnFor(NewPlayer);

    if (ReconnectPawn && !ReconnectPawn->IsPendingKillPending())
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> GM: Reconnect possess old pawn=%s"),
            *GetNameSafe(ReconnectPawn)
        );

        NewPlayer->Possess(ReconnectPawn);
        ReconnectPawn->SetOwner(NewPlayer);

        return;
    }

    AActor* StartSpot = FindPlayerStart(NewPlayer);
    if (!StartSpot)
    {
        UE_LOG(LogTemp, Error, TEXT(">>> GM: No PlayerStart found"));
        return;
    }

    APawn* NewPawn = SpawnDefaultPawnFor(NewPlayer, StartSpot);
    if (!NewPawn)
    {
        UE_LOG(LogTemp, Error, TEXT(">>> GM: SpawnDefaultPawnFor failed"));
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> GM: Spawn new pawn=%s StartSpot=%s"),
        *GetNameSafe(NewPawn),
        *GetNameSafe(StartSpot)
    );

    NewPlayer->Possess(NewPawn);
    NewPawn->SetOwner(NewPlayer);
}

void ANiosCore_GameMode::Logout(AController* Exiting)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> GM: Logout Controller=%s"),
        *GetNameSafe(Exiting)
    );

    if (Exiting)
    {
        APawn* Pawn = Exiting->GetPawn();

        if (Pawn)
        {
            StoreDisconnectedPawn(Exiting, Pawn);

            UE_LOG(LogTemp, Warning, TEXT(">>> GM: Keep pawn alive on logout Pawn=%s"),
                *GetNameSafe(Pawn)
            );

            Exiting->UnPossess();

            Pawn->SetOwner(nullptr);
            Pawn->SetInstigator(nullptr);

            //  Destroy().
            //  SetLifeSpan().
            //      reconnect  -logout abuse.
        }
    }

    Super::Logout(Exiting);
}

FString ANiosCore_GameMode::GetReconnectKey(AController* Controller) const
{
    if (!Controller)
        return FString();

    APlayerState* PS = Controller->PlayerState;
    if (!PS)
        return Controller->GetName();

    const FUniqueNetIdRepl& UniqueId = PS->GetUniqueId();

    if (UniqueId.IsValid())
    {
        return UniqueId->ToString();
    }

    if (!PS->GetPlayerName().IsEmpty())
    {
        return PS->GetPlayerName();
    }

    return Controller->GetName();
}

APawn* ANiosCore_GameMode::FindDisconnectedPawnFor(AController* Controller)
{
    const FString Key = GetReconnectKey(Controller);

    if (Key.IsEmpty())
        return nullptr;

    TObjectPtr<APawn>* FoundPawnPtr = DisconnectedPawns.Find(Key);
    if (!FoundPawnPtr)
        return nullptr;

    APawn* Pawn = FoundPawnPtr->Get();

    if (!IsValid(Pawn))
    {
        DisconnectedPawns.Remove(Key);
        return nullptr;
    }

    DisconnectedPawns.Remove(Key);
    return Pawn;
}

void ANiosCore_GameMode::StoreDisconnectedPawn(AController* Controller, APawn* Pawn)
{
    if (!Controller || !Pawn)
        return;

    const FString Key = GetReconnectKey(Controller);

    if (Key.IsEmpty())
        return;

    DisconnectedPawns.Add(Key, Pawn);

    UE_LOG(LogTemp, Warning, TEXT(">>> GM: Stored disconnected pawn Key=%s Pawn=%s"),
        *Key,
        *GetNameSafe(Pawn)
    );
}
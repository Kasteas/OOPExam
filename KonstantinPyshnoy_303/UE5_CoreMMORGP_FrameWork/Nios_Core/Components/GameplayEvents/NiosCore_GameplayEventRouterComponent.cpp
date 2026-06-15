#include "Components/GameplayEvents/NiosCore_GameplayEventRouterComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/NiosMissionManagerInterface.h"
#include "MOMissionsManager.h"
#include "Types/MissionEventNames.h"
#include "Types/MissionTypes.h"


namespace
{
    UMOMissionsManager* NiosResolveMissionManagerFromActor(AActor* Actor, int32 Depth = 0)
    {
        if (!Actor || Depth > 4)
        {
            return nullptr;
        }

        if (Actor->GetClass()->ImplementsInterface(UNiosMissionManagerInterface::StaticClass()))
        {
            if (UMOMissionsManager* Manager = INiosMissionManagerInterface::Execute_GetNiosMissionManager(Actor))
            {
                return Manager;
            }
        }

        if (UMOMissionsManager* Manager = Actor->FindComponentByClass<UMOMissionsManager>())
        {
            return Manager;
        }

        if (APawn* Pawn = Cast<APawn>(Actor))
        {
            if (APlayerState* PlayerState = Pawn->GetPlayerState())
            {
                if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(PlayerState, Depth + 1))
                {
                    return Manager;
                }
            }

            if (AController* Controller = Pawn->GetController())
            {
                if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(Controller, Depth + 1))
                {
                    return Manager;
                }
            }
        }

        if (AController* Controller = Cast<AController>(Actor))
        {
            if (APlayerState* PlayerState = Controller->PlayerState)
            {
                if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(PlayerState, Depth + 1))
                {
                    return Manager;
                }
            }
        }

        if (AActor* Owner = Actor->GetOwner())
        {
            if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(Owner, Depth + 1))
            {
                return Manager;
            }
        }

        return nullptr;
    }

    UMOMissionsManager* NiosResolveMissionManagerForGameplayEvent(
        const UNiosCore_GameplayEventRouterComponent* Router,
        const FNiosCoreGameplayEvent& Event)
    {
        if (!Router)
        {
            return nullptr;
        }

        if (APlayerState* OwnerPlayerState = Event.OwnerPlayerState.Get())
        {
            if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(OwnerPlayerState))
            {
                return Manager;
            }
        }

        if (APlayerState* OwnerPlayerState = Router->GetOwningPlayerState())
        {
            if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(OwnerPlayerState))
            {
                return Manager;
            }
        }

        // Kill credit, dialogue and interact events often originate from a pawn/avatar while the
        // router may live on PlayerState, Character or Controller depending on the project setup.
        // Resolve through runtime actor refs before giving up so quests keep working after pawn swaps
        // and in projects that keep MissionManager on Character rather than PlayerState.
        if (AActor* InstigatorActor = Event.InstigatorActor.Get())
        {
            if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(InstigatorActor))
            {
                return Manager;
            }
        }

        if (AActor* SourceActor = Event.SourceActor.Get())
        {
            if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(SourceActor))
            {
                return Manager;
            }
        }

        if (AActor* Owner = Router->GetOwner())
        {
            if (UMOMissionsManager* Manager = NiosResolveMissionManagerFromActor(Owner))
            {
                return Manager;
            }
        }

        return nullptr;
    }
}

UNiosCore_GameplayEventRouterComponent::UNiosCore_GameplayEventRouterComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

APlayerState* UNiosCore_GameplayEventRouterComponent::GetOwningPlayerState() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    if (APlayerState* PlayerState = Cast<APlayerState>(Owner))
    {
        return PlayerState;
    }

    if (APawn* Pawn = Cast<APawn>(Owner))
    {
        return Pawn->GetPlayerState();
    }

    if (AController* Controller = Cast<AController>(Owner))
    {
        return Controller->PlayerState;
    }

    return nullptr;
}

FNiosCoreGameplayEvent UNiosCore_GameplayEventRouterComponent::MakeEvent(FName EventType, FName TargetId, int32 Amount, AActor* SourceActor, AActor* InstigatorActor, FName ContextId) const
{
    FNiosCoreGameplayEvent Event;
    Event.EventType = EventType;
    Event.TargetId = TargetId;
    Event.Amount = Amount;
    Event.SourceActor = SourceActor;
    Event.InstigatorActor = InstigatorActor;
    Event.ContextId = ContextId;
    Event.OwnerPlayerState = GetOwningPlayerState();
    return Event;
}

void UNiosCore_GameplayEventRouterComponent::EmitGameplayEvent(const FNiosCoreGameplayEvent& Event)
{
    if (!Event.IsValidEvent())
    {
        return;
    }

    FNiosCoreGameplayEvent RoutedEvent = Event;
    if (!RoutedEvent.OwnerPlayerState)
    {
        RoutedEvent.OwnerPlayerState = GetOwningPlayerState();
    }

    OnGameplayEvent.Broadcast(RoutedEvent);

    UMOMissionsManager* MissionsManager = NiosResolveMissionManagerForGameplayEvent(this, RoutedEvent);
    if (!MissionsManager)
    {
        UE_LOG(LogTemp, Warning, TEXT("NiosGameplayEventRouter: event %s/%s was emitted on %s but no MissionManager could be resolved. Instigator=%s Source=%s OwnerPS=%s"),
            *RoutedEvent.EventType.ToString(),
            *RoutedEvent.TargetId.ToString(),
            *GetNameSafe(GetOwner()),
            *GetNameSafe(RoutedEvent.InstigatorActor.Get()),
            *GetNameSafe(RoutedEvent.SourceActor.Get()),
            *GetNameSafe(RoutedEvent.OwnerPlayerState.Get()));
        return;
    }

    if (RoutedEvent.EventType == NiosCoreGameplayEventNames::ItemCountChanged)
    {
        MissionsManager->UpdateItemCountForObjectives(RoutedEvent.TargetId, RoutedEvent.Amount);
        return;
    }

    FMissionQuestEvent MissionEvent;
    MissionEvent.EventType = RoutedEvent.EventType;
    MissionEvent.TargetId = RoutedEvent.TargetId;
    MissionEvent.Amount = RoutedEvent.Amount;
    MissionEvent.SourceId = RoutedEvent.SourceId;
    MissionEvent.ContextId = RoutedEvent.ContextId;
    MissionEvent.MissionID = RoutedEvent.MissionID;
    MissionEvent.ObjectiveID = RoutedEvent.ObjectiveID;
    MissionsManager->SubmitQuestEvent(MissionEvent);
}

void UNiosCore_GameplayEventRouterComponent::ReportItemCountChanged(FName ItemId, int32 CurrentCount, FName ContextId)
{
    if (ItemId.IsNone())
    {
        return;
    }

    EmitGameplayEvent(MakeEvent(NiosCoreGameplayEventNames::ItemCountChanged, ItemId, FMath::Max(0, CurrentCount), nullptr, nullptr, ContextId));
}

void UNiosCore_GameplayEventRouterComponent::ReportNpcTalked(FName NpcId, AActor* NpcActor, FName ContextId)
{
    if (NpcId.IsNone())
    {
        return;
    }

    EmitGameplayEvent(MakeEvent(NiosCoreGameplayEventNames::NPCTalked, NpcId, 1, NpcActor, nullptr, ContextId));
}

void UNiosCore_GameplayEventRouterComponent::ReportItemTurnedIn(FName NpcId, FName MissionID, FName ObjectiveID, AActor* NpcActor, FName ContextId)
{
    if (NpcId.IsNone())
    {
        return;
    }

    FNiosCoreGameplayEvent Event = MakeEvent(NiosCoreGameplayEventNames::ItemTurnedIn, NpcId, 1, NpcActor, nullptr, ContextId);
    Event.MissionID = MissionID;
    Event.ObjectiveID = ObjectiveID;
    EmitGameplayEvent(Event);
}

void UNiosCore_GameplayEventRouterComponent::ReportActorKilled(FName ActorId, AActor* VictimActor, AActor* KillerActor, int32 Count)
{
    if (ActorId.IsNone())
    {
        return;
    }

    EmitGameplayEvent(MakeEvent(NiosCoreGameplayEventNames::ActorKilled, ActorId, FMath::Max(1, Count), VictimActor, KillerActor));
}

void UNiosCore_GameplayEventRouterComponent::ReportLocationReached(FName LocationId, AActor* SourceActor, FName ContextId)
{
    if (LocationId.IsNone())
    {
        return;
    }

    EmitGameplayEvent(MakeEvent(NiosCoreGameplayEventNames::LocationReached, LocationId, 1, SourceActor, nullptr, ContextId));
}

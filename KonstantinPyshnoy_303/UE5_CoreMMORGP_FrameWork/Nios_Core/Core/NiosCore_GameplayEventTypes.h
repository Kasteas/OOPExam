#pragma once

#include "CoreMinimal.h"
class AActor;
class APlayerState;

#include "NiosCore_GameplayEventTypes.generated.h"

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreGameplayEvent
{
    GENERATED_BODY()

    // Core-level event id. Keep it generic; do not encode quest names into EventType.
    // Examples: Nios.Event.ItemCountChanged, Nios.Event.ActorKilled, Nios.Event.NPCTalked.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    FName EventType;

    // ItemId, NpcId, ActorId, LocationId, AbilityId etc.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    FName TargetId;

    // For ItemCountChanged this is absolute current inventory count.
    // For kills/talk/craft/use events this is usually delta amount, normally 1.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    int32 Amount = 1;

    // Optional stable ids for audit/validation.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    FName SourceId;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    FName ContextId;

    // Optional explicit routing for UI-selected turn-ins.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    FName MissionID;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    FName ObjectiveID;

    // Runtime-only actor refs for validation. Do not persist these.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    TObjectPtr<AActor> SourceActor = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    TObjectPtr<AActor> InstigatorActor = nullptr;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Nios|Gameplay Event")
    TObjectPtr<APlayerState> OwnerPlayerState = nullptr;

    bool IsValidEvent() const
    {
        return !EventType.IsNone() && !TargetId.IsNone();
    }
};

namespace NiosCoreGameplayEventNames
{
    static const FName ItemCountChanged(TEXT("Nios.Event.ItemCountChanged"));
    static const FName ItemCollected(TEXT("Nios.Event.ItemCollected"));
    static const FName ItemRemoved(TEXT("Nios.Event.ItemRemoved"));
    static const FName ItemTurnedIn(TEXT("Nios.Event.ItemTurnedIn"));
    static const FName ActorKilled(TEXT("Nios.Event.ActorKilled"));
    static const FName NPCTalked(TEXT("Nios.Event.NPCTalked"));
    static const FName LocationReached(TEXT("Nios.Event.LocationReached"));
    static const FName ItemCrafted(TEXT("Nios.Event.ItemCrafted"));
    static const FName AbilityUsed(TEXT("Nios.Event.AbilityUsed"));
}

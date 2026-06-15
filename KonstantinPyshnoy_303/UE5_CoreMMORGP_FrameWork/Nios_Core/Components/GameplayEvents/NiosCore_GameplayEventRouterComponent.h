#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Core/NiosCore_GameplayEventTypes.h"
#include "NiosCore_GameplayEventRouterComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosCoreGameplayEventReceived, const FNiosCoreGameplayEvent&, Event);

/**
 * Per-player gameplay event router.
 *
 * This is intentionally not a world-global bus. Combat/inventory/dialog systems should resolve
 * the affected player(s) first, then emit on that player's router. MissionObjectives can listen
 * to this component without Nios_Core depending on the quest plugin.
 */
UCLASS(ClassGroup = (Nios), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_GameplayEventRouterComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_GameplayEventRouterComponent();

    UPROPERTY(BlueprintAssignable, Category = "Nios|Gameplay Events")
    FNiosCoreGameplayEventReceived OnGameplayEvent;

    UFUNCTION(BlueprintCallable, Category = "Nios|Gameplay Events")
    void EmitGameplayEvent(const FNiosCoreGameplayEvent& Event);

    UFUNCTION(BlueprintCallable, Category = "Nios|Gameplay Events|Inventory")
    void ReportItemCountChanged(FName ItemId, int32 CurrentCount, FName ContextId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Nios|Gameplay Events|Dialogue")
    void ReportNpcTalked(FName NpcId, AActor* NpcActor = nullptr, FName ContextId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Nios|Gameplay Events|Dialogue")
    void ReportItemTurnedIn(FName NpcId, FName MissionID = NAME_None, FName ObjectiveID = NAME_None, AActor* NpcActor = nullptr, FName ContextId = NAME_None);

    UFUNCTION(BlueprintCallable, Category = "Nios|Gameplay Events|Combat")
    void ReportActorKilled(FName ActorId, AActor* VictimActor = nullptr, AActor* KillerActor = nullptr, int32 Count = 1);

    UFUNCTION(BlueprintCallable, Category = "Nios|Gameplay Events")
    void ReportLocationReached(FName LocationId, AActor* SourceActor = nullptr, FName ContextId = NAME_None);

    UFUNCTION(BlueprintPure, Category = "Nios|Gameplay Events")
    APlayerState* GetOwningPlayerState() const;

private:
    FNiosCoreGameplayEvent MakeEvent(FName EventType, FName TargetId, int32 Amount, AActor* SourceActor = nullptr, AActor* InstigatorActor = nullptr, FName ContextId = NAME_None) const;
};

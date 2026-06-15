#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "NiosCore_GameMode.generated.h"

class UNiosCore_CombatRulesDataAsset;

UCLASS()
class NIOS_CORE_API ANiosCore_GameMode : public AGameMode
{
    GENERATED_BODY()

public:
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void RestartPlayer(AController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    UFUNCTION(BlueprintPure, Category = "Nios|Combat|Rules")
    UNiosCore_CombatRulesDataAsset* GetCombatRulesDataAsset() const { return CombatRulesDataAsset; }

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Combat|Rules")
    TObjectPtr<UNiosCore_CombatRulesDataAsset> CombatRulesDataAsset = nullptr;

protected:
    FString GetReconnectKey(AController* Controller) const;

    APawn* FindDisconnectedPawnFor(AController* Controller);
    void StoreDisconnectedPawn(AController* Controller, APawn* Pawn);

protected:
    UPROPERTY()
    TMap<FString, TObjectPtr<APawn>> DisconnectedPawns;
};
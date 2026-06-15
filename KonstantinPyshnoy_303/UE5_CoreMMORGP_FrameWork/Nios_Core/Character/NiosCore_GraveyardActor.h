#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Respawn/NiosCore_GraveyardRegistryDataAsset.h"
#include "NiosCore_GraveyardActor.generated.h"

class UArrowComponent;
class USceneComponent;
class APawn;

/**
 * World marker used by server-authoritative resurrection.
 * Client asks its PlayerController to respawn, server selects the nearest enabled graveyard.
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API ANiosCore_GraveyardActor : public AActor
{
    GENERATED_BODY()

public:
    ANiosCore_GraveyardActor();

    UFUNCTION(BlueprintPure, Category = "Nios|Respawn")
    bool IsRespawnEnabled() const { return bRespawnEnabled; }

    UFUNCTION(BlueprintPure, Category = "Nios|Respawn")
    FName GetGraveyardID() const { return GraveyardID; }

    /** Returns the authoritative spawn transform used by respawn flow. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Respawn")
    FTransform GetRespawnTransform(const APawn* RespawningPawn) const;

    /** Helper for designers/editor utilities: copy marker values into registry data. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Respawn")
    FNiosCoreGraveyardEntry MakeRegistryEntry() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    TObjectPtr<USceneComponent> Root;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    TObjectPtr<UArrowComponent> RespawnArrow;

    /** Disable without deleting the marker from the map. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    bool bRespawnEnabled = true;

    /** Optional designer/debug identifier. If None, actor name is used in feedback/logs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FName GraveyardID = NAME_None;

    /** Local-space offset from RespawnArrow/actor location. Useful when the mesh marker should not be the exact spawn point. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FVector LocalRespawnOffset = FVector::ZeroVector;

    /** Random horizontal scatter radius. 0 means exact marker location. Server-only in normal use. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn", meta = (ClampMin = "0", UIMin = "0"))
    float SpawnRadius = 0.f;
};

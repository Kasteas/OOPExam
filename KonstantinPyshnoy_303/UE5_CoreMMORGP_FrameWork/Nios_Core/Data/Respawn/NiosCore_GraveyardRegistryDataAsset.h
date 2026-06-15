#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "NiosCore_GraveyardRegistryDataAsset.generated.h"

class APawn;

UENUM(BlueprintType)
enum class ENiosCoreGraveyardAccessMode : uint8
{
    /** Empty/lightweight rule: any dead player can use this graveyard. */
    Everyone UMETA(DisplayName = "Everyone"),

    /** Pawn or PlayerState must own at least one tag from WhoCanReviveHere. */
    RequiresAnyGameplayTag UMETA(DisplayName = "Requires Any Gameplay Tag"),

    /** Pawn or PlayerState must own every tag from WhoCanReviveHere. */
    RequiresAllGameplayTags UMETA(DisplayName = "Requires All Gameplay Tags")
};

/**
 * Data-driven resurrection point.
 *
 * This intentionally stores coordinates rather than relying on a placed actor being loaded.
 * That makes graveyard respawn compatible with World Partition/unloaded cells.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreGraveyardEntry
{
    GENERATED_BODY()

    /** Designer-facing stable ID returned in respawn result/logs. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FName GraveyardID = NAME_None;

    /** Short level/map name where this graveyard is valid. None means any level. Example: L_StarterIsland. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FName LevelName = NAME_None;

    /** World-space respawn coordinate. This can point into an unloaded World Partition cell. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FVector Location = FVector::ZeroVector;

    /** Spawn facing direction. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FRotator Rotation = FRotator::ZeroRotator;

    /** Random horizontal scatter radius. 0 means exact coordinate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn", meta = (ClampMin = "0", UIMin = "0"))
    float SpawnRadius = 0.f;

    /** Disable without deleting data from the registry. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    bool bEnabled = true;

    /** Who may use this graveyard. For simple vertical slice use Everyone. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    ENiosCoreGraveyardAccessMode AccessMode = ENiosCoreGraveyardAccessMode::Everyone;

    /** Tags checked on Pawn and PlayerState when AccessMode requires tags. Empty tag set passes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FGameplayTagContainer WhoCanReviveHere;

    bool IsValidForLevel(FName CurrentLevelName) const;
    bool IsAllowedForPawn(const APawn* RespawningPawn) const;
    FTransform BuildRespawnTransform(APawn* RespawningPawn) const;
};

/**
 * Lightweight DataAsset registry for graveyard respawn.
 *
 * Server should query this first, then optionally fall back to loaded graveyard actors for test maps/debug.
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API UNiosCore_GraveyardRegistryDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    TArray<FNiosCoreGraveyardEntry> Graveyards;

    UFUNCTION(BlueprintCallable, Category = "Nios|Respawn", meta = (WorldContext = "WorldContextObject"))
    bool FindNearestGraveyard(
        UObject* WorldContextObject,
        FVector Origin,
        APawn* RespawningPawn,
        float SearchRadius,
        FTransform& OutTransform,
        FName& OutGraveyardID) const;
};

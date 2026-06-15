#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "NiosCore_PlayerController.generated.h"

class UNiosCore_GroundTargetingComponent;
class UNiosCore_GraveyardRegistryDataAsset;
class APawn;

UENUM(BlueprintType)
enum class ENiosCoreRespawnRequestResult : uint8
{
    Success UMETA(DisplayName = "Success"),
    MissingPawn UMETA(DisplayName = "Missing Pawn"),
    NotDead UMETA(DisplayName = "Not Dead"),
    MissingAbilitySystem UMETA(DisplayName = "Missing Ability System"),
    InvalidHealthState UMETA(DisplayName = "Invalid Health State"),
    NoRespawnPoint UMETA(DisplayName = "No Respawn Point"),
    Disabled UMETA(DisplayName = "Disabled")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNiosRespawnAtGraveyardResultSignature, bool, bSuccess, ENiosCoreRespawnRequestResult, Result, FName, RespawnPointName);

/**
 * C++ base PlayerController for local player presentation/input systems.
 *
 * PlayerController owns client-facing components:
 * - Ground targeting / cursor targeting.
 * - Client-to-server resurrection requests.
 * - Future local input routing helpers.
 * - Future local presentation helpers.
 *
 * Persistent gameplay components stay on PlayerState.
 */
UCLASS(BlueprintType, Blueprintable)
class NIOS_CORE_API ANiosCore_PlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    ANiosCore_PlayerController();

    virtual void PlayerTick(float DeltaTime) override;

    UFUNCTION(BlueprintPure, Category="Nios|Targeting")
    UNiosCore_GroundTargetingComponent* GetGroundTargetingComponent() const { return GroundTargetingComponent; }

    /** Called by GroundTargetingComponent after local confirm. Sends authoritative activation request. */
    UFUNCTION(BlueprintCallable, Category="Nios|Skills")
    void TryActivateGroundSkill(FName SkillID, FVector TargetLocation);

    UFUNCTION(Server, Reliable)
    void Server_TryActivateGroundSkill(FName SkillID, FVector TargetLocation);

    /** Client entry point: asks the server to revive the current dead pawn at the nearest enabled graveyard. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Respawn")
    void RequestRespawnAtNearestGraveyard();

    UPROPERTY(BlueprintAssignable, Category = "Nios|Respawn")
    FNiosRespawnAtGraveyardResultSignature OnRespawnAtGraveyardResult;

    UFUNCTION(BlueprintCallable, Category = "Nios|PvP")
    void SetPvPEnabled(bool bNewPvPEnabled);

    UFUNCTION(BlueprintCallable, Category = "Nios|PvP")
    void TogglePvPEnabled();

    UFUNCTION(BlueprintPure, Category = "Nios|PvP")
    bool IsPvPEnabled() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Nios|Targeting")
    TObjectPtr<UNiosCore_GroundTargetingComponent> GroundTargetingComponent;

    /** Local/client safety lock: Health <= 0 must block movement even if pawn death replication path is delayed. */
    UPROPERTY(Transient)
    bool bDeathMoveInputLocked = false;

    /** Master switch for corpse-run/graveyard respawn. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    bool bAllowRespawnAtNearestGraveyard = true;

    /** World Partition-safe source of truth. Stores level name + coordinates + access rules. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    TObjectPtr<UNiosCore_GraveyardRegistryDataAsset> GraveyardRegistry = nullptr;

    /** Prefer registry coordinates over loaded actors. Keep enabled for World Partition/MMO maps. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    bool bPreferGraveyardRegistry = true;

    /** Allow actor scan fallback for tiny test maps/debug. Not World Partition safe by itself. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    bool bAllowLoadedGraveyardActorFallback = true;

    /** Graveyards can be native ANiosCore_GraveyardActor or any actor with this tag when actor fallback is enabled. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    FName GraveyardActorTag = FName(TEXT("Nios.Graveyard"));

    /** 0 means search the whole current level/world. Registry entries still filter by LevelName first. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn", meta = (ClampMin = "0", UIMin = "0"))
    float GraveyardSearchRadius = 0.f;

    /** If no graveyard exists, fall back to GameMode FindPlayerStart. Useful for early test maps. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn")
    bool bFallbackToPlayerStartWhenNoGraveyard = true;

    /** Health restored by server graveyard respawn. Clamp: [Minimum, MaxHealth]. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn", meta = (ClampMin = "0", UIMin = "0", UIMax = "1"))
    float GraveyardRespawnHealthPercent = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Respawn", meta = (ClampMin = "0", UIMin = "0"))
    float GraveyardRespawnMinimumHealth = 1.f;

    UFUNCTION(Server, Reliable)
    void Server_RequestRespawnAtNearestGraveyard();

    UFUNCTION(Client, Reliable)
    void Client_ReceiveRespawnAtGraveyardResult(bool bSuccess, ENiosCoreRespawnRequestResult Result, FName RespawnPointName);

    void BroadcastRespawnAtGraveyardResult(bool bSuccess, ENiosCoreRespawnRequestResult Result, FName RespawnPointName);
    bool FindNearestRespawnTransform(APawn* DeadPawn, FTransform& OutTransform, FName& OutRespawnPointName);
    void RefreshDeathMovementInputLock();
};

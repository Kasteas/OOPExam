#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/Loot/NiosCore_LootDataTypes.h"
#include "NiosCore_LootSourceComponent.generated.h"

class APlayerState;
class UNiosCore_InventoryComponent;
class UNiosCore_LootRulesDataAsset;
class UNiosCore_LootTableDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNiosCoreLootChanged);

/**
 * Server-authoritative loot source. Put it on chests, corpses, mobs and world objects.
 *
 * The component has two independent rule layers:
 * 1) Ownership: public, first damage unit, top damage unit, first interactor unit.
 * 2) Distribution inside the owner unit: free-for-all, round-robin, vote threshold, leader-only.
 *
 * Party/raid support is intentionally provider-based. Until a party system exists, every player is treated as a solo unit.
 * Later, override the BP_GetLootUnit* events in a subclass/BP or route them to a party subsystem.
 */
UCLASS(ClassGroup = (Nios), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_LootSourceComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_LootSourceComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Low-level static helper used by combat pipeline. Damaged actor owns this component. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Contribution")
    static void ReportDamageForLoot(AActor* DamagedActor, AActor* DamageSourceActor, float DamageAmount);

    /** Low-level static helper used by combat/death pipeline. Locks ownership at death time when possible. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Contribution")
    static void NotifyActorDiedForLoot(AActor* DeadActor);

    /** Server-side contribution record. Usually called by ReportDamageForLoot. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Contribution")
    void RecordDamageContribution(AActor* DamageSourceActor, float DamageAmount);

    /** Resolve ownership now. For mobs/bosses this should happen on death. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Ownership")
    bool ResolveLootOwnership(AActor* OptionalInteractor = nullptr, bool bForceReResolve = false);

    /** Death-time helper: resolves ownership and optionally generates loot. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Ownership")
    void FinalizeLootOnDeath();

    UFUNCTION(BlueprintCallable, Category = "Nios|Loot")
    void GenerateLoot(bool bForceRegenerate = false);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot")
    bool HasLoot() const;

    /** Returns true if the actor can loot at least one current slot. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot")
    bool CanLoot(AActor* LootingActor);

    /**
     * Lightweight interaction helper for traces/UI.
     * Returns true when the actor may try to open this loot source, even if loot has not been generated yet.
     * Server still validates actual transfers through LootSlotToInventory/LootAllToInventory.
     */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Interaction")
    bool CanAttemptLoot(AActor* LootingActor, FText& OutReasonText) const;

    /** Trace/UI convenience helper: finds a LootSourceComponent on LootSourceActor and runs CanAttemptLoot. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Interaction")
    static bool CanAttemptLootActor(AActor* LootSourceActor, AActor* LootingActor, FText& OutReasonText);


    /** Read-only UI helper. Server still validates the real loot request. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|UI")
    bool GetLootSlotAccessInfo(AActor* LootingActor, int32 LootSlotIndex, FNiosCoreLootSlotAccessInfo& OutAccessInfo) const;

    /** Server-side transfer of one slot into inventory. Supports partial transfer if inventory has limited space. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot")
    bool LootSlotToInventory(int32 LootSlotIndex, UNiosCore_InventoryComponent* InventoryComponent, int32 Count, int32& OutAddedCount);

    /** Server-side transfer of every currently allowed slot into inventory. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot")
    bool LootAllToInventory(UNiosCore_InventoryComponent* InventoryComponent, int32& OutAddedCount);

    /** Future Need/Greed/MasterLoot hook: explicitly awards a slot to a player. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Loot|Vote")
    bool AwardLootSlotToPlayer(int32 LootSlotIndex, APlayerState* WinnerPlayerState);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot")
    const TArray<FNiosCoreLootSlot>& GetLootSlots() const { return LootSlots; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot")
    bool IsGenerated() const { return bLootGenerated; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|Ownership")
    bool IsOwnershipResolved() const { return bLootOwnershipResolved; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|Ownership")
    FName GetResolvedLootOwnerUnitID() const { return ResolvedLootOwnerUnit.UnitID; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Loot|Rules")
    FNiosCoreLootRules GetEffectiveLootRules() const;

    /** Party provider extension point. Default returns Solo_<PlayerId>. */
    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Loot|Groups")
    FName BP_GetLootUnitID(APlayerState* PlayerState) const;

    /** Party provider extension point. Default returns Solo. */
    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Loot|Groups")
    ENiosCoreLootUnitType BP_GetLootUnitType(APlayerState* PlayerState) const;

    /** Party provider extension point. Default returns the same player as the solo leader. */
    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Loot|Groups")
    APlayerState* BP_GetLootUnitLeader(APlayerState* PlayerState) const;

    /** Party provider extension point. Default returns one-member solo unit. */
    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Loot|Groups")
    TArray<APlayerState*> BP_GetLootUnitMembers(APlayerState* PlayerState) const;

protected:
    void BuildEntries(TArray<FNiosCoreLootEntry>& OutEntries) const;
    void MarkChanged();
    void AssignDistributionReservations();
    void EnsureLootReadyForLooting(AActor* OptionalInteractor);

    bool CanLootSlot(AActor* LootingActor, FNiosCoreLootSlot& LootSlot);
    bool CanReachLootSource(AActor* LootingActor) const;
    bool IsOwnerAliveForLootInteraction() const;
    bool IsPlayerStateInResolvedOwnerUnit(APlayerState* PlayerState) const;
    bool IsPlayerStateCurrentlyValidForOwnership(APlayerState* PlayerState, const FNiosCoreLootRules& Rules) const;
    bool IsLootUnitCurrentlyEligible(const FNiosCoreLootUnitSnapshot& Unit, const FNiosCoreLootRules& Rules) const;
    bool TryResolvePlayerState(AActor* Actor, APlayerState*& OutPlayerState) const;
    bool ResolveLootUnitForPlayerState(APlayerState* PlayerState, FNiosCoreLootUnitSnapshot& OutUnit) const;
    AActor* ResolveLootingPhysicalActor(AActor* InventoryOwnerActor) const;
    AActor* ResolvePlayerPhysicalActor(APlayerState* PlayerState) const;
    APlayerState* GetNextEligibleRoundRobinMember();
    APlayerState* ResolveSlotReservation(FNiosCoreLootSlot& LootSlot);

    UFUNCTION()
    void OnRep_LootSlots();

    UFUNCTION()
    void OnRep_LootOwnership();

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Loot|Vote")
    void BP_OnLootVoteRequested(int32 LootSlotIndex, FName ItemID, ENiosCoreLootItemRarity Rarity);

public:
    /**
     * Select where the item list comes from.
     * CustomLoot is quickest for one-off chests/mobs. DataSettingsLoot is for shared bandit/wolf/boss tables.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Items|Setup")
    ENiosCoreLootItemsMode LootItemsMode = ENiosCoreLootItemsMode::CustomLoot;

    /** Reusable shared loot table, for example one table used by a whole bandit gang archetype. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Items|Data Settings Loot", meta = (DisplayName = "Data Settings Loot Table", EditCondition = "LootItemsMode != ENiosCoreLootItemsMode::CustomLoot", EditConditionHides))
    TObjectPtr<UNiosCore_LootTableDataAsset> LootTable = nullptr;

    /** Local per-actor loot list. Good for unique chests, testing, guaranteed quest items, or one-off overrides. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Items|Custom Loot", meta = (DisplayName = "Custom Loot Entries", EditCondition = "LootItemsMode != ENiosCoreLootItemsMode::DataSettingsLoot", EditConditionHides))
    TArray<FNiosCoreLootEntry> InlineLootEntries;

    /** Optional reusable rules preset. InlineLootRules are used when this is not set. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Rules")
    TObjectPtr<UNiosCore_LootRulesDataAsset> LootRulesAsset = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Rules")
    FNiosCoreLootRules InlineLootRules;

    /** If true, loot rolls once on BeginPlay. Good for chests. Corpses usually generate on death/request. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Items")
    bool bGenerateLootOnBeginPlay = false;

    /** If true, repeated GenerateLoot() calls do nothing unless bForceRegenerate is true. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Items")
    bool bGenerateOnlyOnce = true;

    /** 0 disables click distance check. Default 300uu = roughly 3m. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Interaction", meta = (ClampMin = "0"))
    float LootInteractionRange = 300.f;

    /**
     * If true, owners implementing NiosGASActorInterface cannot be looted while IsAlive() is true.
     * This blocks opening living mobs/players, while plain chests/world actors can still be opened.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Interaction")
    bool bBlockLootWhileOwnerAlive = true;

    /** If true, first loot request generates loot when it was not generated earlier. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Items")
    bool bGenerateOnFirstLootRequest = true;

    /** Optional cleanup for simple chests/corpses after all loot was taken. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Loot|Interaction")
    bool bDestroyOwnerWhenEmpty = false;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Loot")
    FNiosCoreLootChanged OnLootChanged;

protected:
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LootSlots, Category = "Nios|Loot")
    TArray<FNiosCoreLootSlot> LootSlots;

    UPROPERTY(BlueprintReadOnly, Replicated, Category = "Nios|Loot")
    bool bLootGenerated = false;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LootOwnership, Category = "Nios|Loot|Ownership")
    bool bLootOwnershipResolved = false;

    /** True when non-public ownership rules deliberately fell back to public access. */
    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LootOwnership, Category = "Nios|Loot|Ownership")
    bool bLootResolvedAsPublic = false;

    UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_LootOwnership, Category = "Nios|Loot|Ownership")
    FNiosCoreLootUnitSnapshot ResolvedLootOwnerUnit;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Contribution")
    TArray<FNiosCoreLootContributionRecord> ContributionRecords;

    UPROPERTY(Transient)
    int32 RoundRobinCursor = 0;
};

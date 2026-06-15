#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "NiosCore_LootDataTypes.generated.h"

class APlayerState;

UENUM(BlueprintType)
enum class ENiosCoreLootOwnershipRule : uint8
{
    /** Anyone who can reach the source may loot it. Good for simple chests: whoever clicks first takes the item. */
    Public UMETA(DisplayName = "Public / Whoever Gets It"),

    /** The first valid player/party/raid unit that damaged the source owns the loot. */
    FirstDamageUnit UMETA(DisplayName = "First Damage Unit"),

    /** The valid player/party/raid unit with the highest total contribution owns the loot. */
    TopDamageUnit UMETA(DisplayName = "Top Damage Unit"),

    /** The first valid player/party/raid unit that opens the source locks ownership. Useful for private chests. */
    FirstInteractorUnit UMETA(DisplayName = "First Interactor Unit")
};

UENUM(BlueprintType)
enum class ENiosCoreLootDistributionRule : uint8
{
    /** Any member of the owner unit may take any slot. Public sources also use this as free-for-all. */
    FreeForAllInsideOwnerUnit UMETA(DisplayName = "Free For All Inside Owner Unit"),

    /** Generated slots are reserved for owner-unit members in sequence. */
    RoundRobin UMETA(DisplayName = "Round Robin"),

    /** Common loot is round-robin. Items at/above VoteRarityThreshold are routed to future vote flow. */
    RoundRobinWithVoteThreshold UMETA(DisplayName = "Round Robin + Vote Threshold"),

    /** Only party/raid leader, or solo owner, may take loot. */
    LeaderOnly UMETA(DisplayName = "Leader Only")
};

UENUM(BlueprintType)
enum class ENiosCoreLootUnitType : uint8
{
    Solo UMETA(DisplayName = "Solo"),
    Party UMETA(DisplayName = "Party"),
    Raid UMETA(DisplayName = "Raid")
};

UENUM(BlueprintType)
enum class ENiosCoreLootItemRarity : uint8
{
    Common UMETA(DisplayName = "Common"),
    Uncommon UMETA(DisplayName = "Uncommon"),
    Rare UMETA(DisplayName = "Rare"),
    Epic UMETA(DisplayName = "Epic"),
    Legendary UMETA(DisplayName = "Legendary")
};

UENUM(BlueprintType)
enum class ENiosCoreLootVoteState : uint8
{
    None UMETA(DisplayName = "None"),
    PendingVote UMETA(DisplayName = "Pending Vote"),
    Awarded UMETA(DisplayName = "Awarded")
};

/**
 * How this source builds its item list.
 * CustomLoot is fastest for one-off chests/mobs. DataSettingsLoot is for reusable shared tables.
 */
UENUM(BlueprintType)
enum class ENiosCoreLootItemsMode : uint8
{
    /** Use only the CustomLootEntries array on the LootSourceComponent. */
    CustomLoot UMETA(DisplayName = "Custom Loot"),

    /** Use only the reusable LootTable data asset. */
    DataSettingsLoot UMETA(DisplayName = "Data Settings Loot"),

    /** Use the reusable LootTable and append CustomLootEntries as local extras/overrides. */
    DataSettingsAndCustomLoot UMETA(DisplayName = "Data Settings + Custom Loot")
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLootSlotAccessInfo
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Access")
    bool bCanLoot = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Access")
    bool bWithinRange = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Access")
    bool bInOwnerUnit = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Access")
    bool bReservedForRequester = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Access")
    bool bReservedForOther = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Access")
    bool bPendingVote = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot|Access")
    FText ReasonText;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLootRules
{
    GENERATED_BODY()

    /** Who owns the source after death/open. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Ownership")
    ENiosCoreLootOwnershipRule OwnershipRule = ENiosCoreLootOwnershipRule::Public;

    /** How loot is distributed inside the owner unit. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Distribution")
    ENiosCoreLootDistributionRule DistributionRule = ENiosCoreLootDistributionRule::FreeForAllInsideOwnerUnit;

    /** 0 disables owner presence distance check. 5000uu is roughly 50m. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Ownership", meta = (ClampMin = "0"))
    float OwnershipPresenceRadius = 5000.f;

    /** Skip candidate units that have no living member when ownership is resolved. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Ownership")
    bool bSkipDeadOwnershipUnits = true;

    /** Skip candidate units that have no member near the source when ownership is resolved. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Ownership")
    bool bSkipOutOfRangeOwnershipUnits = true;

    /** If no valid damage/interactor owner exists, make the source public instead of locking it. Mostly useful for test maps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Ownership")
    bool bFallbackToPublicWhenNoValidOwner = false;

    /** If true, first loot click after death/open freezes ownership. Recommended for bosses/corpses. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Ownership")
    bool bLockOwnershipWhenResolved = true;

    /** Items at or above this rarity enter the future vote path for RoundRobinWithVoteThreshold. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Vote")
    ENiosCoreLootItemRarity VoteRarityThreshold = ENiosCoreLootItemRarity::Uncommon;

    /** Until real Need/Greed UI exists, allow the owner leader to take vote-threshold items. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot|Vote")
    bool bFallbackVoteItemsToLeaderUntilVoteSystem = true;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLootUnitSnapshot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    FName UnitID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    ENiosCoreLootUnitType UnitType = ENiosCoreLootUnitType::Solo;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    TObjectPtr<APlayerState> Leader = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    TArray<TObjectPtr<APlayerState>> Members;

    FORCEINLINE bool IsValidUnit() const
    {
        return !UnitID.IsNone() && Members.Num() > 0;
    }
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLootContributionRecord
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    TObjectPtr<APlayerState> PlayerState = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    FNiosCoreLootUnitSnapshot LootUnit;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    float DamageAmount = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    float FirstDamageTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    float LastDamageTime = 0.f;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLootEntry
{
    GENERATED_BODY()

    /** ItemID from DT_Items / resolver database. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot")
    FName ItemID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot", meta = (ClampMin = "1"))
    int32 MinCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot", meta = (ClampMin = "1"))
    int32 MaxCount = 1;

    /** 0..1 chance per roll. 1 = always. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot", meta = (ClampMin = "0", ClampMax = "1"))
    float DropChance = 1.f;

    /** Number of independent rolls for this entry. Useful for coin/material stacks. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot", meta = (ClampMin = "1"))
    int32 Rolls = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Loot")
    ENiosCoreLootItemRarity Rarity = ENiosCoreLootItemRarity::Common;

    FORCEINLINE bool IsValidEntry() const
    {
        return !ItemID.IsNone() && MaxCount > 0 && Rolls > 0 && DropChance > 0.f;
    }
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreLootSlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    int32 LootSlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    FName ItemID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    int32 Count = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    ENiosCoreLootItemRarity Rarity = ENiosCoreLootItemRarity::Common;

    /** Round-robin or vote winner reservation. Null means free inside the owner unit. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    TObjectPtr<APlayerState> ReservedForPlayerState = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    ENiosCoreLootVoteState VoteState = ENiosCoreLootVoteState::None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Loot")
    bool bLooted = false;

    FORCEINLINE bool IsEmpty() const
    {
        return bLooted || ItemID.IsNone() || Count <= 0;
    }

    FORCEINLINE void Clear()
    {
        ItemID = NAME_None;
        Count = 0;
        Rarity = ENiosCoreLootItemRarity::Common;
        ReservedForPlayerState = nullptr;
        VoteState = ENiosCoreLootVoteState::None;
        bLooted = true;
    }
};

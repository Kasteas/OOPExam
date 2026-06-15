#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiosCore_StatTypes.generated.h"

UENUM(BlueprintType)
enum class ENiosStatType : uint8
{
    None UMETA(DisplayName = "None"),

    // Core attributes. These are the main class/level/equipment driven stats.
    Strength UMETA(DisplayName = "Strength"),
    Agility UMETA(DisplayName = "Agility"),
    Intelligence UMETA(DisplayName = "Intelligence"),
    Stamina UMETA(DisplayName = "Stamina"),
    Spirit UMETA(DisplayName = "Spirit"),
    Constitution UMETA(DisplayName = "Constitution"),

    // Runtime resources. Current values are clamped by their Max* stat.
    Health UMETA(DisplayName = "Health"),
    MaxHealth UMETA(DisplayName = "Max Health"),

    Mana UMETA(DisplayName = "Mana"),
    MaxMana UMETA(DisplayName = "Max Mana"),

    Energy UMETA(DisplayName = "Energy"),
    MaxEnergy UMETA(DisplayName = "Max Energy"),

    Rage UMETA(DisplayName = "Rage"),
    MaxRage UMETA(DisplayName = "Max Rage"),

    // Derived combat stats.
    PhysicalCrit UMETA(DisplayName = "Physical Crit"),
    MagicCrit UMETA(DisplayName = "Magic Crit"),

    PhysicalDefense UMETA(DisplayName = "Physical Defense"),
    MagicDefense UMETA(DisplayName = "Magic Defense"),

    PhysicalModifier UMETA(DisplayName = "Physical Modifier"),
    MagicModifier UMETA(DisplayName = "Magic Modifier"),

    PlayerLevel UMETA(DisplayName = "Player Level"),
    PlayerXP UMETA(DisplayName = "Player XP"),
};

UENUM(BlueprintType)
enum class ENiosStatModifierLayer : uint8
{
    Base UMETA(DisplayName = "Base"),
    Equipment UMETA(DisplayName = "Equipment"),
    Passive UMETA(DisplayName = "Passive"),
    Buff UMETA(DisplayName = "Buff"),
    Debuff UMETA(DisplayName = "Debuff"),
    Runtime UMETA(DisplayName = "Runtime")
};

USTRUCT(BlueprintType)
struct FNiosStatModifier
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ENiosStatType StatType = ENiosStatType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value = 0.f;
};

/** One source of modifiers: item instance, buff id, passive id, talent id, class base stats, runtime resource state, etc. */
USTRUCT(BlueprintType)
struct FNiosStatModifierSource
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SourceID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ENiosStatModifierLayer Layer = ENiosStatModifierLayer::Equipment;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNiosStatModifier> Modifiers;

    FORCEINLINE bool IsValid() const
    {
        return !SourceID.IsNone() && Modifiers.Num() > 0;
    }
};

USTRUCT(BlueprintType)
struct FNiosResourceFlags
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasMana = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasEnergy = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bHasRage = false;
};

USTRUCT(BlueprintType)
struct FNiosClassLevelStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 Level = 1;

    /** Usually core stats only: Strength, Agility, Intelligence, Stamina, Spirit, Constitution. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FNiosStatModifier> BaseStats;
};

USTRUCT(BlueprintType)
struct FNiosStatScalingRule
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ENiosStatType SourceStat = ENiosStatType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ENiosStatType TargetStat = ENiosStatType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Scale = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    ENiosStatModifierLayer OutputLayer = ENiosStatModifierLayer::Passive;
};

USTRUCT(BlueprintType)
struct FNiosCalculatedStat
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    ENiosStatType StatType = ENiosStatType::None;

    UPROPERTY(BlueprintReadOnly)
    float BaseValue = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float EquipmentValue = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float PassiveValue = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float BuffValue = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float DebuffValue = 0.f;

    /** Current resources like Health/Mana/Energy/Rage live here. */
    UPROPERTY(BlueprintReadOnly)
    float RuntimeValue = 0.f;

    UPROPERTY(BlueprintReadOnly)
    float FinalValue = 0.f;

    FORCEINLINE float GetTemporaryDelta() const
    {
        return BuffValue + DebuffValue;
    }
};

/** Optional designer-authored formulas. Built-in formulas live in StatsComponent and can be supplemented by these rules. */
UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_StatScalingDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    TArray<FNiosStatScalingRule> ScalingRules;
};

/** Temporary class/base-stats profile. Later backend/class system can select these by ClassID/level. */
UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_ClassStatsProfileDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    FName ClassID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    FNiosResourceFlags ResourceFlags;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    TArray<FNiosClassLevelStats> LevelStats;

    /** UI helpers: use these to decide which resource bars should be shown for this class. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasMana() const
    {
        return ResourceFlags.bHasMana;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasEnergy() const
    {
        return ResourceFlags.bHasEnergy;
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasRage() const
    {
        return ResourceFlags.bHasRage;
    }

    /** Health is always available for gameplay entities that use this profile. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasHealth() const
    {
        return true;
    }

    /** Generic UI helper for stat bars. Max* aliases are accepted too. */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats|Resources")
    bool HasResource(ENiosStatType ResourceStat) const
    {
        switch (ResourceStat)
        {
        case ENiosStatType::Health:
        case ENiosStatType::MaxHealth:
            return true;
        case ENiosStatType::Mana:
        case ENiosStatType::MaxMana:
            return ResourceFlags.bHasMana;
        case ENiosStatType::Energy:
        case ENiosStatType::MaxEnergy:
            return ResourceFlags.bHasEnergy;
        case ENiosStatType::Rage:
        case ENiosStatType::MaxRage:
            return ResourceFlags.bHasRage;
        default:
            return false;
        }
    }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|Stats")
    bool GetStatsForLevel(int32 Level, FNiosClassLevelStats& OutStats) const
    {
        OutStats = FNiosClassLevelStats();

        const FNiosClassLevelStats* BestMatch = nullptr;
        for (const FNiosClassLevelStats& Entry : LevelStats)
        {
            if (Entry.Level == Level)
            {
                OutStats = Entry;
                return true;
            }

            if (Entry.Level <= Level && (!BestMatch || Entry.Level > BestMatch->Level))
            {
                BestMatch = &Entry;
            }
        }

        if (BestMatch)
        {
            OutStats = *BestMatch;
            return true;
        }

        return false;
    }
};

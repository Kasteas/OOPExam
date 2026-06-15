#include "Components/Stats/NiosCore_StatsComponent.h"

#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

namespace
{
    static const FName NiosClassBaseSourceID(TEXT("ClassBaseStats"));

    FName MakeBaseSourceID(ENiosStatType StatType)
    {
        return FName(*FString::Printf(TEXT("Base_%d"), static_cast<int32>(StatType)));
    }

    FName MakeRuntimeSourceID(ENiosStatType StatType)
    {
        return FName(*FString::Printf(TEXT("Runtime_%d"), static_cast<int32>(StatType)));
    }
}

UNiosCore_StatsComponent::UNiosCore_StatsComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UNiosCore_StatsComponent::BeginPlay()
{
    Super::BeginPlay();

    if (!AbilitySystemComponent)
    {
        AbilitySystemComponent = ResolveASC();
    }


}

void UNiosCore_StatsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UNiosCore_StatsComponent, CalculatedStats);
}

void UNiosCore_StatsComponent::InitializeAbilitySystemComponent(UNiosCore_AbilitySystemComponent* InASC)
{
    if (AbilitySystemComponent == InASC)
    {
        return;
    }

    AbilitySystemComponent = InASC;

    // If the component was initialized before ASC/AttributeSet became available, push the already calculated stats now.
    if (bStatsComponentInitialized)
    {
        RecalculateStats();
    }
}

UNiosCore_AbilitySystemComponent* UNiosCore_StatsComponent::ResolveASC() const
{
    if (AbilitySystemComponent)
    {
        return AbilitySystemComponent;
    }

    AActor* Owner = GetOwner();
    return Owner ? Owner->FindComponentByClass<UNiosCore_AbilitySystemComponent>() : nullptr;
}

void UNiosCore_StatsComponent::SetClassStatsProfile(UNiosCore_ClassStatsProfileDataAsset* InProfile)
{
    if (ClassStatsProfile != InProfile)
    {
        ClassStatsProfile = InProfile;

        // Class/profile changes are authoring/bootstrap events. Refill runtime resources from the new Max* values.
        bRuntimeResourcesInitialized = false;
        LastKnownResourceMaxValues.Empty();
        ModifierSources.Remove(MakeRuntimeSourceID(ENiosStatType::Health));
        ModifierSources.Remove(MakeRuntimeSourceID(ENiosStatType::Mana));
        ModifierSources.Remove(MakeRuntimeSourceID(ENiosStatType::Energy));
        ModifierSources.Remove(MakeRuntimeSourceID(ENiosStatType::Rage));
    }

    ApplyClassBaseStats();
    RecalculateStats();
}

void UNiosCore_StatsComponent::SetCurrentLevel(int32 InLevel)
{
    CurrentLevel = FMath::Max(1, InLevel);
    ApplyClassBaseStats();
    RecalculateStats();
}

TArray<FNiosStatModifier> UNiosCore_StatsComponent::GetDefaultStats() const
{
    TArray<FNiosStatModifier> Result;

    if (!ClassStatsProfile)
    {
        return Result;
    }

    FNiosClassLevelStats LevelStats;
    if (ClassStatsProfile->GetStatsForLevel(CurrentLevel, LevelStats))
    {
        Result = LevelStats.BaseStats;
    }

    return Result;
}

void UNiosCore_StatsComponent::ApplyClassBaseStats()
{
    if (!ClassStatsProfile)
    {
        return;
    }

    ActiveResourceFlags = ClassStatsProfile->ResourceFlags;

    FNiosClassLevelStats LevelStats;
    if (!ClassStatsProfile->GetStatsForLevel(CurrentLevel, LevelStats))
    {
        return;
    }

    FNiosStatModifierSource Source;
    Source.SourceID = NiosClassBaseSourceID;
    Source.Layer = ENiosStatModifierLayer::Base;
    Source.Modifiers = LevelStats.BaseStats;
    Source.Modifiers.Add({ ENiosStatType::PlayerLevel, static_cast<float>(CurrentLevel) });

    if (Source.IsValid())
    {
        ModifierSources.Add(Source.SourceID, Source);
    }
}

void UNiosCore_StatsComponent::SetBaseStat(ENiosStatType StatType, float Value)
{
    if (StatType == ENiosStatType::None)
    {
        return;
    }

    FNiosStatModifierSource BaseSource;
    BaseSource.SourceID = MakeBaseSourceID(StatType);
    BaseSource.Layer = ENiosStatModifierLayer::Base;
    BaseSource.Modifiers.Add({ StatType, Value });

    AddOrReplaceModifierSource(BaseSource);
}

void UNiosCore_StatsComponent::AddOrReplaceModifierSource(const FNiosStatModifierSource& Source)
{
    if (!Source.IsValid())
    {
        return;
    }

    ModifierSources.Add(Source.SourceID, Source);
    RecalculateStats();
}

void UNiosCore_StatsComponent::RemoveModifierSource(FName SourceID)
{
    if (SourceID.IsNone())
    {
        return;
    }

    if (ModifierSources.Remove(SourceID) > 0)
    {
        RecalculateStats();
    }
}

void UNiosCore_StatsComponent::RemoveModifierSourcesByLayer(ENiosStatModifierLayer Layer)
{
    bool bChanged = false;

    for (auto It = ModifierSources.CreateIterator(); It; ++It)
    {
        if (It.Value().Layer == Layer)
        {
            It.RemoveCurrent();
            bChanged = true;
        }
    }

    if (bChanged)
    {
        RecalculateStats();
    }
}

void UNiosCore_StatsComponent::SetCurrentResource(ENiosStatType ResourceStat, float NewValue)
{
    SetCurrentResourceRuntimeValue(ResourceStat, NewValue, true);
}

void UNiosCore_StatsComponent::SetCurrentResourceRuntimeValue(ENiosStatType ResourceStat, float NewValue, bool bRecalculate)
{
    if (ResourceStat != ENiosStatType::Health &&
        ResourceStat != ENiosStatType::Mana &&
        ResourceStat != ENiosStatType::Energy &&
        ResourceStat != ENiosStatType::Rage)
    {
        return;
    }

    const float RuntimeValue = FMath::Max(0.f, NewValue);

    FNiosStatModifierSource Source;
    Source.SourceID = MakeRuntimeSourceID(ResourceStat);
    Source.Layer = ENiosStatModifierLayer::Runtime;
    Source.Modifiers.Add({ ResourceStat, RuntimeValue });
    ModifierSources.Add(Source.SourceID, Source);

    if (FNiosCalculatedStat* Existing = FindCalculatedStat(ResourceStat))
    {
        Existing->BaseValue = 0.f;
        Existing->EquipmentValue = 0.f;
        Existing->PassiveValue = 0.f;
        Existing->BuffValue = 0.f;
        Existing->DebuffValue = 0.f;
        Existing->RuntimeValue = RuntimeValue;
        Existing->FinalValue = RuntimeValue;
    }

    if (bRecalculate)
    {
        RecalculateStats();
    }
}

void UNiosCore_StatsComponent::AddRuntimeSource(ENiosStatType StatType, float Value)
{
    FNiosStatModifierSource Source;
    Source.SourceID = MakeRuntimeSourceID(StatType);
    Source.Layer = ENiosStatModifierLayer::Runtime;
    Source.Modifiers.Add({ StatType, Value });
    AddOrReplaceModifierSource(Source);
}


bool UNiosCore_StatsComponent::HasRuntimeSource(ENiosStatType StatType) const
{
    return ModifierSources.Contains(MakeRuntimeSourceID(StatType));
}

FNiosCalculatedStat* UNiosCore_StatsComponent::FindCalculatedStat(ENiosStatType StatType)
{
    for (FNiosCalculatedStat& Stat : CalculatedStats)
    {
        if (Stat.StatType == StatType)
        {
            return &Stat;
        }
    }

    return nullptr;
}

FNiosCalculatedStat& UNiosCore_StatsComponent::FindOrAddCalculatedStat(ENiosStatType StatType)
{
    if (FNiosCalculatedStat* Existing = FindCalculatedStat(StatType))
    {
        return *Existing;
    }

    FNiosCalculatedStat NewStat;
    NewStat.StatType = StatType;
    return CalculatedStats.Add_GetRef(NewStat);
}

void UNiosCore_StatsComponent::AddValueToCalculatedStat(ENiosStatType StatType, ENiosStatModifierLayer Layer, float Value)
{
    if (StatType == ENiosStatType::None || FMath::IsNearlyZero(Value))
    {
        return;
    }

    FNiosCalculatedStat& Stat = FindOrAddCalculatedStat(StatType);

    switch (Layer)
    {
    case ENiosStatModifierLayer::Base:
        Stat.BaseValue += Value;
        break;
    case ENiosStatModifierLayer::Equipment:
        Stat.EquipmentValue += Value;
        break;
    case ENiosStatModifierLayer::Passive:
        Stat.PassiveValue += Value;
        break;
    case ENiosStatModifierLayer::Buff:
        Stat.BuffValue += Value;
        break;
    case ENiosStatModifierLayer::Debuff:
        Stat.DebuffValue += Value;
        break;
    case ENiosStatModifierLayer::Runtime:
        Stat.RuntimeValue += Value;
        break;
    default:
        break;
    }
}

void UNiosCore_StatsComponent::RecalculateStats()
{
    CalculatedStats.Empty();

    for (const TPair<FName, FNiosStatModifierSource>& Pair : ModifierSources)
    {
        const FNiosStatModifierSource& Source = Pair.Value;

        for (const FNiosStatModifier& Modifier : Source.Modifiers)
        {
            AddValueToCalculatedStat(Modifier.StatType, Source.Layer, Modifier.Value);
        }
    }

    EnsureAllKnownStatsExist();
    FinalizeCalculatedStats();

    ApplyBuiltInDerivedStats();
    FinalizeCalculatedStats();

    ApplyScalingRules();
    FinalizeCalculatedStats();

    ApplyResourceGates();
    FinalizeCalculatedStats();

    if (!bRuntimeResourcesInitialized)
    {
        InitializeRuntimeResourcesFromMax();
    }
    else
    {
        ClampCurrentResources();
    }
    FinalizeCalculatedStats();

    PushFinalStatsToASC();
    BroadcastStatsChanged();

    if (!bStatsReadyEventCalled && CalculatedStats.Num() > 0)
    {
        bStatsReadyEventCalled = true;
        BP_OnStatsInitialized();
    }
}

void UNiosCore_StatsComponent::EnsureAllKnownStatsExist()
{
    const ENiosStatType KnownStats[] =
    {
        ENiosStatType::Strength,
        ENiosStatType::Agility,
        ENiosStatType::Intelligence,
        ENiosStatType::Stamina,
        ENiosStatType::Spirit,
        ENiosStatType::Constitution,
        ENiosStatType::Health,
        ENiosStatType::MaxHealth,
        ENiosStatType::Mana,
        ENiosStatType::MaxMana,
        ENiosStatType::Energy,
        ENiosStatType::MaxEnergy,
        ENiosStatType::Rage,
        ENiosStatType::MaxRage,
        ENiosStatType::PhysicalCrit,
        ENiosStatType::MagicCrit,
        ENiosStatType::PhysicalDefense,
        ENiosStatType::MagicDefense,
        ENiosStatType::PhysicalModifier,
        ENiosStatType::MagicModifier,
        ENiosStatType::PlayerLevel,
        ENiosStatType::PlayerXP
    };

    for (ENiosStatType StatType : KnownStats)
    {
        FindOrAddCalculatedStat(StatType);
    }
}

void UNiosCore_StatsComponent::FinalizeCalculatedStats()
{
    for (FNiosCalculatedStat& Stat : CalculatedStats)
    {
        Stat.FinalValue =
            Stat.BaseValue +
            Stat.EquipmentValue +
            Stat.PassiveValue +
            Stat.BuffValue +
            Stat.DebuffValue +
            Stat.RuntimeValue;
    }
}

void UNiosCore_StatsComponent::ApplyBuiltInDerivedStats()
{
    const float Strength = GetFinalStat(ENiosStatType::Strength);
    const float Agility = GetFinalStat(ENiosStatType::Agility);
    const float Intelligence = GetFinalStat(ENiosStatType::Intelligence);
    const float Stamina = GetFinalStat(ENiosStatType::Stamina);
    const float Spirit = GetFinalStat(ENiosStatType::Spirit);
    const float Constitution = GetFinalStat(ENiosStatType::Constitution);

    AddValueToCalculatedStat(ENiosStatType::MaxHealth, ENiosStatModifierLayer::Passive, Stamina * HealthPerStamina);

    if (ActiveResourceFlags.bHasMana)
    {
        AddValueToCalculatedStat(ENiosStatType::MaxMana, ENiosStatModifierLayer::Passive, Intelligence * ManaPerIntelligence);
    }

    if (ActiveResourceFlags.bHasEnergy)
    {
        AddValueToCalculatedStat(ENiosStatType::MaxEnergy, ENiosStatModifierLayer::Passive, Agility * EnergyPerAgility);
    }

    if (ActiveResourceFlags.bHasRage)
    {
        AddValueToCalculatedStat(ENiosStatType::MaxRage, ENiosStatModifierLayer::Passive, Strength * RagePerStrength);
    }

    AddValueToCalculatedStat(ENiosStatType::PhysicalCrit, ENiosStatModifierLayer::Passive, Agility * PhysicalCritPerAgility);
    AddValueToCalculatedStat(ENiosStatType::MagicCrit, ENiosStatModifierLayer::Passive, Intelligence * MagicCritPerIntelligence);

    AddValueToCalculatedStat(ENiosStatType::PhysicalDefense, ENiosStatModifierLayer::Passive, Constitution * PhysicalDefensePerConstitution);
    AddValueToCalculatedStat(ENiosStatType::MagicDefense, ENiosStatModifierLayer::Passive, Spirit * MagicDefensePerSpirit);

    AddValueToCalculatedStat(
        ENiosStatType::PhysicalModifier,
        ENiosStatModifierLayer::Passive,
        Strength * PhysicalModifierPerStrength + Agility * PhysicalModifierPerAgility
    );

    AddValueToCalculatedStat(
        ENiosStatType::MagicModifier,
        ENiosStatModifierLayer::Passive,
        Intelligence * MagicModifierPerIntelligence + Spirit * MagicModifierPerSpirit
    );
}

void UNiosCore_StatsComponent::ApplyScalingRules()
{
    if (!ScalingData)
    {
        return;
    }

    for (const FNiosStatScalingRule& Rule : ScalingData->ScalingRules)
    {
        if (Rule.SourceStat == ENiosStatType::None || Rule.TargetStat == ENiosStatType::None || FMath::IsNearlyZero(Rule.Scale))
        {
            continue;
        }

        const float SourceValue = GetFinalStat(Rule.SourceStat);
        const float Bonus = SourceValue * Rule.Scale;
        AddValueToCalculatedStat(Rule.TargetStat, Rule.OutputLayer, Bonus);
    }
}

bool UNiosCore_StatsComponent::IsResourceEnabled(ENiosStatType ResourceStat) const
{
    switch (ResourceStat)
    {
    case ENiosStatType::Mana:
    case ENiosStatType::MaxMana:
        return ActiveResourceFlags.bHasMana;
    case ENiosStatType::Energy:
    case ENiosStatType::MaxEnergy:
        return ActiveResourceFlags.bHasEnergy;
    case ENiosStatType::Rage:
    case ENiosStatType::MaxRage:
        return ActiveResourceFlags.bHasRage;
    default:
        return true;
    }
}

ENiosStatType UNiosCore_StatsComponent::GetMaxStatForResource(ENiosStatType ResourceStat) const
{
    switch (ResourceStat)
    {
    case ENiosStatType::Health: return ENiosStatType::MaxHealth;
    case ENiosStatType::Mana: return ENiosStatType::MaxMana;
    case ENiosStatType::Energy: return ENiosStatType::MaxEnergy;
    case ENiosStatType::Rage: return ENiosStatType::MaxRage;
    default: return ENiosStatType::None;
    }
}

void UNiosCore_StatsComponent::ResetStatToRuntimeValue(ENiosStatType StatType, float RuntimeValue)
{
    RuntimeValue = FMath::Max(0.f, RuntimeValue);

    FNiosCalculatedStat& Stat = FindOrAddCalculatedStat(StatType);
    Stat.BaseValue = 0.f;
    Stat.EquipmentValue = 0.f;
    Stat.PassiveValue = 0.f;
    Stat.BuffValue = 0.f;
    Stat.DebuffValue = 0.f;
    Stat.RuntimeValue = RuntimeValue;
    Stat.FinalValue = RuntimeValue;

    // Persist runtime resources as sources, so the next RecalculateStats() starts
    // from the previous current value instead of refilling from Max every time.
    FNiosStatModifierSource RuntimeSource;
    RuntimeSource.SourceID = MakeRuntimeSourceID(StatType);
    RuntimeSource.Layer = ENiosStatModifierLayer::Runtime;
    RuntimeSource.Modifiers.Add({ StatType, RuntimeValue });
    ModifierSources.Add(RuntimeSource.SourceID, RuntimeSource);
}

void UNiosCore_StatsComponent::ZeroStat(ENiosStatType StatType)
{
    ResetStatToRuntimeValue(StatType, 0.f);
}

void UNiosCore_StatsComponent::ApplyResourceGates()
{
    if (!ActiveResourceFlags.bHasMana)
    {
        ZeroStat(ENiosStatType::Mana);
        ZeroStat(ENiosStatType::MaxMana);
    }

    if (!ActiveResourceFlags.bHasEnergy)
    {
        ZeroStat(ENiosStatType::Energy);
        ZeroStat(ENiosStatType::MaxEnergy);
    }

    if (!ActiveResourceFlags.bHasRage)
    {
        ZeroStat(ENiosStatType::Rage);
        ZeroStat(ENiosStatType::MaxRage);
    }
}

float UNiosCore_StatsComponent::GetLastKnownMaxForResource(ENiosStatType ResourceStat, float CurrentMaxValue) const
{
    if (const float* StoredMax = LastKnownResourceMaxValues.Find(ResourceStat))
    {
        return FMath::Max(0.f, *StoredMax);
    }

    return FMath::Max(0.f, CurrentMaxValue);
}

void UNiosCore_StatsComponent::StoreLastKnownMaxForResource(ENiosStatType ResourceStat, float CurrentMaxValue)
{
    LastKnownResourceMaxValues.Add(ResourceStat, FMath::Max(0.f, CurrentMaxValue));
}

void UNiosCore_StatsComponent::InitializeRuntimeResourcesFromMax()
{
    const ENiosStatType Resources[] =
    {
        ENiosStatType::Health,
        ENiosStatType::Mana,
        ENiosStatType::Energy,
        ENiosStatType::Rage
    };

    for (ENiosStatType Resource : Resources)
    {
        if (!IsResourceEnabled(Resource))
        {
            ZeroStat(Resource);
            StoreLastKnownMaxForResource(Resource, 0.f);
            continue;
        }

        const ENiosStatType MaxStat = GetMaxStatForResource(Resource);
        const float MaxValue = FMath::Max(0.f, GetFinalStat(MaxStat));

        // First initialization policy:
        // Health/Mana/Energy start full. Rage starts empty by design.
        const float InitialValue = Resource == ENiosStatType::Rage
            ? 0.f
            : MaxValue;

        ResetStatToRuntimeValue(Resource, InitialValue);
        StoreLastKnownMaxForResource(Resource, MaxValue);
    }    bRuntimeResourcesInitialized = true;
}

void UNiosCore_StatsComponent::ClampCurrentResources()
{
    const ENiosStatType Resources[] =
    {
        ENiosStatType::Health,
        ENiosStatType::Mana,
        ENiosStatType::Energy,
        ENiosStatType::Rage
    };

    UNiosCore_AbilitySystemComponent* ASC = ResolveASC();

    auto ReadCurrentFromASC = [ASC](ENiosStatType Resource, float& OutValue) -> bool
    {
        OutValue = 0.f;

        if (!ASC)
        {
            return false;
        }

        switch (Resource)
        {
        case ENiosStatType::Health:
            return ASC->GetAttributeValue(UNiosCore_AttributeSet::GetHealthAttribute(), OutValue);
        case ENiosStatType::Mana:
            return ASC->GetAttributeValue(UNiosCore_AttributeSet::GetManaAttribute(), OutValue);
        case ENiosStatType::Energy:
            return ASC->GetAttributeValue(UNiosCore_AttributeSet::GetEnergyAttribute(), OutValue);
        case ENiosStatType::Rage:
            return ASC->GetAttributeValue(UNiosCore_AttributeSet::GetRageAttribute(), OutValue);
        default:
            return false;
        }
    };

    for (ENiosStatType Resource : Resources)
    {
        if (!IsResourceEnabled(Resource))
        {
            ZeroStat(Resource);
            StoreLastKnownMaxForResource(Resource, 0.f);
            continue;
        }

        const ENiosStatType MaxStat = GetMaxStatForResource(Resource);
        const float MaxValue = FMath::Max(0.f, GetFinalStat(MaxStat));

        float CurrentValue = 0.f;

        if (HasRuntimeSource(Resource))
        {
            FNiosCalculatedStat CurrentStat;
            CurrentValue = GetCalculatedStat(Resource, CurrentStat)
                ? CurrentStat.FinalValue
                : 0.f;
        }
        else if (ReadCurrentFromASC(Resource, CurrentValue))
        {
            // Existing GAS value is treated as current runtime state.
        }
        else
        {
            CurrentValue = Resource == ENiosStatType::Rage
                ? 0.f
                : MaxValue;
        }

        // Clamp-only policy after first initialization:
        // 300 / 300 -> Max becomes 200 => Current becomes 200.
        // 300 / 300 -> Max becomes 700 => Current stays 300.
        const float NewCurrentValue = MaxValue > 0.f
            ? FMath::Clamp(CurrentValue, 0.f, MaxValue)
            : 0.f;

        ResetStatToRuntimeValue(Resource, NewCurrentValue);
        StoreLastKnownMaxForResource(Resource, MaxValue);
    }

}

void UNiosCore_StatsComponent::PushFinalStatsToASC()
{
    UNiosCore_AbilitySystemComponent* ASC = ResolveASC();
    if (!ASC || ASC->GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    auto Push = [ASC](ENiosStatType StatType, float Value)
    {
        switch (StatType)
        {
        case ENiosStatType::Strength:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetStrengthAttribute(), Value);
            break;
        case ENiosStatType::Agility:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetAgilityAttribute(), Value);
            break;
        case ENiosStatType::Intelligence:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetIntelligenceAttribute(), Value);
            break;
        case ENiosStatType::Stamina:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetStaminaAttribute(), Value);
            break;
        case ENiosStatType::Spirit:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetSpiritAttribute(), Value);
            break;
        case ENiosStatType::Constitution:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetConstitutionAttribute(), Value);
            break;
        case ENiosStatType::PhysicalDefense:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetPhysicalDefenseAttribute(), Value);
            break;
        case ENiosStatType::MagicDefense:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetMagicDefenseAttribute(), Value);
            break;
        case ENiosStatType::PhysicalCrit:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetPhysicalCritAttribute(), Value);
            break;
        case ENiosStatType::MagicCrit:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetMagicCritAttribute(), Value);
            break;
        case ENiosStatType::PhysicalModifier:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetPhysicalModifierAttribute(), Value);
            break;
        case ENiosStatType::MagicModifier:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetMagicModifierAttribute(), Value);
            break;
        case ENiosStatType::Health:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetHealthAttribute(), Value);
            break;
        case ENiosStatType::MaxHealth:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetMaxHealthAttribute(), Value);
            break;
        case ENiosStatType::Mana:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetManaAttribute(), Value);
            break;
        case ENiosStatType::MaxMana:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetMaxManaAttribute(), Value);
            break;
        case ENiosStatType::Energy:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetEnergyAttribute(), Value);
            break;
        case ENiosStatType::MaxEnergy:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetMaxEnergyAttribute(), Value);
            break;
        case ENiosStatType::Rage:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetRageAttribute(), Value);
            break;
        case ENiosStatType::MaxRage:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetMaxRageAttribute(), Value);
            break;
        case ENiosStatType::PlayerLevel:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetPlayerLevelAttribute(), Value);
            break;
        case ENiosStatType::PlayerXP:
            ASC->SetAttributeValue(UNiosCore_AttributeSet::GetPlayerXPAttribute(), Value);
            break;
        default:
            break;
        }
    };

    for (const FNiosCalculatedStat& Stat : CalculatedStats)
    {        Push(Stat.StatType, Stat.FinalValue);
    }
}

bool UNiosCore_StatsComponent::GetCalculatedStat(ENiosStatType StatType, FNiosCalculatedStat& OutStat) const
{
    OutStat = FNiosCalculatedStat();

    for (const FNiosCalculatedStat& Stat : CalculatedStats)
    {
        if (Stat.StatType == StatType)
        {
            OutStat = Stat;
            return true;
        }
    }

    return false;
}

float UNiosCore_StatsComponent::GetFinalStat(ENiosStatType StatType) const
{
    FNiosCalculatedStat Stat;
    return GetCalculatedStat(StatType, Stat) ? Stat.FinalValue : 0.f;
}

TArray<FNiosCalculatedStat> UNiosCore_StatsComponent::GetCalculatedStats() const
{
    return CalculatedStats;
}

bool UNiosCore_StatsComponent::HasMana() const
{
    return ActiveResourceFlags.bHasMana;
}

bool UNiosCore_StatsComponent::HasEnergy() const
{
    return ActiveResourceFlags.bHasEnergy;
}

bool UNiosCore_StatsComponent::HasRage() const
{
    return ActiveResourceFlags.bHasRage;
}

bool UNiosCore_StatsComponent::HasHealth() const
{
    return true;
}

bool UNiosCore_StatsComponent::HasResource(ENiosStatType ResourceStat) const
{
    switch (ResourceStat)
    {
    case ENiosStatType::Health:
    case ENiosStatType::MaxHealth:
        return true;
    case ENiosStatType::Mana:
    case ENiosStatType::MaxMana:
        return ActiveResourceFlags.bHasMana;
    case ENiosStatType::Energy:
    case ENiosStatType::MaxEnergy:
        return ActiveResourceFlags.bHasEnergy;
    case ENiosStatType::Rage:
    case ENiosStatType::MaxRage:
        return ActiveResourceFlags.bHasRage;
    default:
        return false;
    }
}

void UNiosCore_StatsComponent::BroadcastStatsChanged()
{
    OnStatsChanged.Broadcast();
    OnCalculatedStatsChanged.Broadcast(CalculatedStats);
}

void UNiosCore_StatsComponent::InitializeStatsComponent()
{
    if (!AbilitySystemComponent)
    {
        AbilitySystemComponent = ResolveASC();
    }

    if (bStatsComponentInitialized)
    {
        // Idempotent but not a no-op: late ASC/AttributeSet initialization must receive current calculated stats.
        RecalculateStats();
        return;
    }

    bStatsComponentInitialized = true;

    ApplyClassBaseStats();
    RecalculateStats();
}

void UNiosCore_StatsComponent::OnRep_CalculatedStats()
{
    BroadcastStatsChanged();

    if (!bStatsReadyEventCalled && CalculatedStats.Num() > 0)
    {
        bStatsReadyEventCalled = true;
        BP_OnStatsInitialized();
    }
}

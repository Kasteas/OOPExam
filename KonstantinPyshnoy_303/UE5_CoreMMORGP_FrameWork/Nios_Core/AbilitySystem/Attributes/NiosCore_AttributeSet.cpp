// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"

#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

UNiosCore_AttributeSet::UNiosCore_AttributeSet()
{
    InitStrength(10.f);
    InitAgility(10.f);
    InitIntelligence(10.f);
    InitStamina(10.f);
    InitSpirit(10.f);
    InitConstitution(10.f);

    // Non-zero defaults keep test mannequins usable immediately and prevent UI divide-by-zero.
    InitMaxHealth(100.f);
    InitHealth(100.f);
    InitMaxMana(100.f);
    InitMana(100.f);
    InitMaxEnergy(100.f);
    InitEnergy(100.f);
    InitMaxRage(100.f);
    InitRage(0.f);

    InitPhysicalCrit(0.f);
    InitMagicCrit(0.f);
    InitPhysicalDefense(0.f);
    InitMagicDefense(0.f);
    InitPhysicalModifier(0.f);
    InitMagicModifier(0.f);

    InitPlayerLevel(1.f);
    InitPlayerXP(0.f);
}

void UNiosCore_AttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UNiosCore_AttributeSet, Strength);
    DOREPLIFETIME(UNiosCore_AttributeSet, Agility);
    DOREPLIFETIME(UNiosCore_AttributeSet, Intelligence);
    DOREPLIFETIME(UNiosCore_AttributeSet, Stamina);
    DOREPLIFETIME(UNiosCore_AttributeSet, Spirit);
    DOREPLIFETIME(UNiosCore_AttributeSet, Constitution);

    DOREPLIFETIME(UNiosCore_AttributeSet, Health);
    DOREPLIFETIME(UNiosCore_AttributeSet, MaxHealth);
    DOREPLIFETIME(UNiosCore_AttributeSet, Mana);
    DOREPLIFETIME(UNiosCore_AttributeSet, MaxMana);
    DOREPLIFETIME(UNiosCore_AttributeSet, Energy);
    DOREPLIFETIME(UNiosCore_AttributeSet, MaxEnergy);
    DOREPLIFETIME(UNiosCore_AttributeSet, Rage);
    DOREPLIFETIME(UNiosCore_AttributeSet, MaxRage);

    DOREPLIFETIME(UNiosCore_AttributeSet, PhysicalCrit);
    DOREPLIFETIME(UNiosCore_AttributeSet, MagicCrit);
    DOREPLIFETIME(UNiosCore_AttributeSet, PhysicalDefense);
    DOREPLIFETIME(UNiosCore_AttributeSet, MagicDefense);
    DOREPLIFETIME(UNiosCore_AttributeSet, PhysicalModifier);
    DOREPLIFETIME(UNiosCore_AttributeSet, MagicModifier);

    DOREPLIFETIME(UNiosCore_AttributeSet, PlayerLevel);
    DOREPLIFETIME(UNiosCore_AttributeSet, PlayerXP);
}

void UNiosCore_AttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UNiosCore_AttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
    Super::PostGameplayEffectExecute(Data);
    ClampCurrentResourceAttributes();
}

void UNiosCore_AttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
    if (Attribute == GetHealthAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, FMath::Max(0.f, GetMaxHealth()));
    }
    else if (Attribute == GetManaAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, FMath::Max(0.f, GetMaxMana()));
    }
    else if (Attribute == GetEnergyAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, FMath::Max(0.f, GetMaxEnergy()));
    }
    else if (Attribute == GetRageAttribute())
    {
        NewValue = FMath::Clamp(NewValue, 0.f, FMath::Max(0.f, GetMaxRage()));
    }
    else if (Attribute == GetMaxHealthAttribute() ||
             Attribute == GetMaxManaAttribute() ||
             Attribute == GetMaxEnergyAttribute() ||
             Attribute == GetMaxRageAttribute())
    {
        NewValue = FMath::Max(0.f, NewValue);
    }
}

void UNiosCore_AttributeSet::ClampCurrentResourceAttributes()
{
    SetHealth(FMath::Clamp(GetHealth(), 0.f, FMath::Max(0.f, GetMaxHealth())));
    SetMana(FMath::Clamp(GetMana(), 0.f, FMath::Max(0.f, GetMaxMana())));
    SetEnergy(FMath::Clamp(GetEnergy(), 0.f, FMath::Max(0.f, GetMaxEnergy())));
    SetRage(FMath::Clamp(GetRage(), 0.f, FMath::Max(0.f, GetMaxRage())));
}

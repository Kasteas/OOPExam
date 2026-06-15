// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeDelta.h"
#include "NiosCore_AttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class NIOS_CORE_API UNiosCore_AttributeSet : public UAttributeSet
{
    GENERATED_BODY()

public:
    UNiosCore_AttributeSet();

    // Core attributes
    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Strength;
    ATTRIBUTE_ACCESSORS(ThisClass, Strength);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Agility;
    ATTRIBUTE_ACCESSORS(ThisClass, Agility);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Intelligence;
    ATTRIBUTE_ACCESSORS(ThisClass, Intelligence);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Stamina;
    ATTRIBUTE_ACCESSORS(ThisClass, Stamina);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Spirit;
    ATTRIBUTE_ACCESSORS(ThisClass, Spirit);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Constitution;
    ATTRIBUTE_ACCESSORS(ThisClass, Constitution);

    // Runtime resources
    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Health;
    ATTRIBUTE_ACCESSORS(ThisClass, Health);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData MaxHealth;
    ATTRIBUTE_ACCESSORS(ThisClass, MaxHealth);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Mana;
    ATTRIBUTE_ACCESSORS(ThisClass, Mana);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData MaxMana;
    ATTRIBUTE_ACCESSORS(ThisClass, MaxMana);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Energy;
    ATTRIBUTE_ACCESSORS(ThisClass, Energy);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData MaxEnergy;
    ATTRIBUTE_ACCESSORS(ThisClass, MaxEnergy);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData Rage;
    ATTRIBUTE_ACCESSORS(ThisClass, Rage);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData MaxRage;
    ATTRIBUTE_ACCESSORS(ThisClass, MaxRage);

    // Derived combat stats
    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData PhysicalCrit;
    ATTRIBUTE_ACCESSORS(ThisClass, PhysicalCrit);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData MagicCrit;
    ATTRIBUTE_ACCESSORS(ThisClass, MagicCrit);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData PhysicalDefense;
    ATTRIBUTE_ACCESSORS(ThisClass, PhysicalDefense);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData MagicDefense;
    ATTRIBUTE_ACCESSORS(ThisClass, MagicDefense);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData PhysicalModifier;
    ATTRIBUTE_ACCESSORS(ThisClass, PhysicalModifier);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData MagicModifier;
    ATTRIBUTE_ACCESSORS(ThisClass, MagicModifier);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData PlayerLevel;
    ATTRIBUTE_ACCESSORS(ThisClass, PlayerLevel);

    UPROPERTY(BlueprintReadOnly, Replicated)
    FGameplayAttributeData PlayerXP;
    ATTRIBUTE_ACCESSORS(ThisClass, PlayerXP);

protected:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
    void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
    void ClampCurrentResourceAttributes();
};

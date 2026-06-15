// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Attributes/NiosCore_AssynTaskAttributeSet.h"

UNiosCore_AssynTaskAttributeSet* UNiosCore_AssynTaskAttributeSet::ListenForAttributeChange(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAttribute Attribute)
{
	UNiosCore_AssynTaskAttributeSet* WaitForAttributeChangedTask = NewObject<UNiosCore_AssynTaskAttributeSet>();
	WaitForAttributeChangedTask->ASC = AbilitySystemComponent;
	WaitForAttributeChangedTask->AttributeToListenFor = Attribute;

	if (!IsValid(AbilitySystemComponent) || !Attribute.IsValid())
	{
		WaitForAttributeChangedTask->RemoveFromRoot();
		return nullptr;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(WaitForAttributeChangedTask, &UNiosCore_AssynTaskAttributeSet::AttributeChanged);

	return WaitForAttributeChangedTask;
}

UNiosCore_AssynTaskAttributeSet* UNiosCore_AssynTaskAttributeSet::ListenForAttributesChange(UAbilitySystemComponent* AbilitySystemComponent, TArray<FGameplayAttribute> Attributes)
{
	UNiosCore_AssynTaskAttributeSet* WaitForAttributeChangedTask = NewObject<UNiosCore_AssynTaskAttributeSet>();
	WaitForAttributeChangedTask->ASC = AbilitySystemComponent;
	WaitForAttributeChangedTask->AttributesToListenFor = Attributes;

	if (!IsValid(AbilitySystemComponent) || Attributes.Num() < 1)
	{
		WaitForAttributeChangedTask->RemoveFromRoot();
		return nullptr;
	}

	for (FGameplayAttribute Attribute : Attributes)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Attribute).AddUObject(WaitForAttributeChangedTask, &UNiosCore_AssynTaskAttributeSet::AttributeChanged);
	}

	return WaitForAttributeChangedTask;
}

void UNiosCore_AssynTaskAttributeSet::EndTask()
{
	if (IsValid(ASC))
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeToListenFor).RemoveAll(this);

		for (FGameplayAttribute Attribute : AttributesToListenFor)
		{
			ASC->GetGameplayAttributeValueChangeDelegate(Attribute).RemoveAll(this);
		}
	}

	SetReadyToDestroy();
	MarkAsGarbage();
}

void UNiosCore_AssynTaskAttributeSet::AttributeChanged(const FOnAttributeChangeData& Data)
{
	OnAttributeChanged.Broadcast(Data.Attribute, Data.NewValue, Data.OldValue);
}
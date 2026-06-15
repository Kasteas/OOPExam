#include "AbilitySystem/Executions/NiosAbilityExecution.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"

void UNiosAbilityExecution::Execute_Implementation(UGameplayAbility* Ability)
{
}

void UNiosAbilityExecution::ApplyEffect(
    UGameplayAbility* Ability,
    const TArray<FNiosCoreAttributeDelta>& Deltas)
{
    if (!Ability)
        return;

    UAbilitySystemComponent* ASC = Ability->GetAbilitySystemComponentFromActorInfo();
    if (!ASC)
        return;

    for (const FNiosCoreAttributeDelta& Delta : Deltas)
    {
        if (!Delta.Attribute.IsValid())
            continue;

        if (FMath::IsNearlyZero(Delta.Value))
            continue;

        ASC->ApplyModToAttribute(
            Delta.Attribute,
            EGameplayModOp::Additive,
            Delta.Value
        );
    }
}
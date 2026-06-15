#include "AbilitySystem/Executions/NiosExec_ApplyToSelf.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"

void UNiosExec_ApplyToSelf::Execute_Implementation(UGameplayAbility* Ability)
{
    if (!Ability)
        return;

    UAbilitySystemComponent* ASC =
        Ability->GetAbilitySystemComponentFromActorInfo();

    if (!ASC)
        return;

    for (TSubclassOf<UGameplayEffect> GEClass : Effects)
    {
        if (!GEClass)
            continue;

        FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
        Context.AddSourceObject(Ability);

        FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(
            GEClass,
            Ability->GetAbilityLevel(),
            Context
        );

        if (!SpecHandle.IsValid())
            continue;

        ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
    }
}
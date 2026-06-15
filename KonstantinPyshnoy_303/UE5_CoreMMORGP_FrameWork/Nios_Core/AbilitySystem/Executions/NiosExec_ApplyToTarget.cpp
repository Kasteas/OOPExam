#include "AbilitySystem/Executions/NiosExec_ApplyToTarget.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffect.h"

#include "AbilitySystem/Core/NiosCore_BaseGameplayAbility.h"

void UNiosExec_ApplyToTarget::Execute_Implementation(UGameplayAbility* Ability)
{
    UNiosCore_BaseGameplayAbility* NiosAbility =
        Cast<UNiosCore_BaseGameplayAbility>(Ability);

    if (!NiosAbility)
        return;

    UAbilitySystemComponent* SourceASC =
        NiosAbility->GetAbilitySystemComponentFromActorInfo();

    if (!SourceASC)
        return;

    const FGameplayAbilityTargetDataHandle& TargetData =
        NiosAbility->GetCachedTargetData();

    for (int32 i = 0; i < TargetData.Num(); ++i)
    {
        const FGameplayAbilityTargetData* Data = TargetData.Get(i);
        if (!Data)
            continue;

        TArray<TWeakObjectPtr<AActor>> Actors = Data->GetActors();

        for (TWeakObjectPtr<AActor> ActorPtr : Actors)
        {
            AActor* TargetActor = ActorPtr.Get();
            if (!TargetActor)
                continue;

            UAbilitySystemComponent* TargetASC =
                UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

            if (!TargetASC)
                continue;

            for (TSubclassOf<UGameplayEffect> GEClass : Effects)
            {
                if (!GEClass)
                    continue;

                FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
                Context.AddSourceObject(Ability);

                FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
                    GEClass,
                    Ability->GetAbilityLevel(),
                    Context
                );

                if (!SpecHandle.IsValid())
                    continue;

                SourceASC->ApplyGameplayEffectSpecToTarget(
                    *SpecHandle.Data.Get(),
                    TargetASC
                );
            }
        }
    }
}
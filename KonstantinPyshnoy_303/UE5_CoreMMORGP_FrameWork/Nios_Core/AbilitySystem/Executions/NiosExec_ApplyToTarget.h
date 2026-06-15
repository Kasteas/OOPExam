#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Executions/NiosAbilityExecution.h"
#include "NiosExec_ApplyToTarget.generated.h"

class UGameplayAbility;
class UGameplayEffect;

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class NIOS_CORE_API UNiosExec_ApplyToTarget : public UNiosAbilityExecution
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, Category = "Effects")
    TArray<TSubclassOf<UGameplayEffect>> Effects;

    virtual void Execute_Implementation(UGameplayAbility* Ability) override;
};
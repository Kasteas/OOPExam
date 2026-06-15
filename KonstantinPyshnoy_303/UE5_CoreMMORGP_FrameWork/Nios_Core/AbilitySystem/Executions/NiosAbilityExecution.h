#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeDelta.h"
#include "NiosAbilityExecution.generated.h"

class UGameplayAbility;

UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class NIOS_CORE_API UNiosAbilityExecution : public UObject
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, Category = "Nios|Ability")
    void Execute(UGameplayAbility* Ability);

    virtual void Execute_Implementation(UGameplayAbility* Ability);

protected:
    void ApplyEffect(
        UGameplayAbility* Ability,
        const TArray<FNiosCoreAttributeDelta>& Deltas
    );
};
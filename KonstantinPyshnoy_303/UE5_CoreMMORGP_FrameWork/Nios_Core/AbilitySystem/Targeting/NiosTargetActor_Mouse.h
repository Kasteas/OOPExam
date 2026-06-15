#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "NiosTargetActor_Mouse.generated.h"

class APlayerController;

UCLASS()
class NIOS_CORE_API ANiosTargetActor_Mouse : public AGameplayAbilityTargetActor
{
    GENERATED_BODY()

public:
    ANiosTargetActor_Mouse();

    virtual void StartTargeting(UGameplayAbility* Ability) override;
    virtual void ConfirmTargetingAndContinue() override;

private:
    UPROPERTY()
    TObjectPtr<APlayerController> SourcePC;
};
#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Actor.h"
#include "Animation/AnimMontage.h"
#include "Components/AudioComponent.h"
#include "NiosGCN_SkillCastLoop.generated.h"

UCLASS()
class NIOS_CORE_API ANiosGCN_SkillCastLoop : public AGameplayCueNotify_Actor
{
    GENERATED_BODY()

public:
    ANiosGCN_SkillCastLoop();

protected:
    virtual bool OnActive_Implementation(
        AActor* MyTarget,
        const FGameplayCueParameters& Parameters
    ) override;

    virtual bool OnRemove_Implementation(
        AActor* MyTarget,
        const FGameplayCueParameters& Parameters
    ) override;

private:
    UPROPERTY()
    TObjectPtr<UAnimMontage> ActiveCastLoopMontage;

    UPROPERTY()
    TObjectPtr<UAudioComponent> ActiveCastLoopAudioComponent;
};
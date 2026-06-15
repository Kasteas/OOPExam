#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Static.h"
#include "NiosGCN_SkillExecute.generated.h"

UCLASS()
class NIOS_CORE_API UNiosGCN_SkillExecute : public UGameplayCueNotify_Static
{
    GENERATED_BODY()

public:
    UNiosGCN_SkillExecute();

    virtual bool OnExecute_Implementation(
        AActor* MyTarget,
        const FGameplayCueParameters& Parameters
    ) const override;
};
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "NiosCore_AttributeDelta.generated.h"

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreAttributeDelta
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Value = 0.f;
};
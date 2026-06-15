#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UNiagaraSystem;
#include "NiosCore_SkillDeliveryTypes.generated.h"

UENUM(BlueprintType)
enum class ENiosSkillDeliveryType : uint8
{
    Instant UMETA(DisplayName = "Instant"),
    DelayedTarget UMETA(DisplayName = "Delayed Target"),
    DelayedPoint UMETA(DisplayName = "Delayed Point"),
    AreaAtPoint UMETA(DisplayName = "Area At Point"),
    TeleportToPoint UMETA(DisplayName = "Teleport To Point")
};

UENUM(BlueprintType)
enum class ENiosSkillPointSource : uint8
{
    CasterLocation UMETA(DisplayName = "Caster Location"),
    SelectedTargetLocation UMETA(DisplayName = "Selected Target Location"),
    ProvidedPoint UMETA(DisplayName = "Provided Point")
};

USTRUCT(BlueprintType)
struct FNiosSkillDeliveryDefinition
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery")
    ENiosSkillDeliveryType DeliveryType = ENiosSkillDeliveryType::Instant;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery")
    TArray<int32> EffectIndices;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery")
    ENiosSkillPointSource StartPointSource = ENiosSkillPointSource::CasterLocation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::DelayedTarget || DeliveryType == ENiosSkillDeliveryType::DelayedPoint || DeliveryType == ENiosSkillDeliveryType::AreaAtPoint || DeliveryType == ENiosSkillDeliveryType::TeleportToPoint", EditConditionHides))
    ENiosSkillPointSource ImpactPointSource = ENiosSkillPointSource::SelectedTargetLocation;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::DelayedTarget || DeliveryType == ENiosSkillDeliveryType::DelayedPoint", EditConditionHides, ClampMin = "0"))
    float TravelSpeed = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::DelayedTarget || DeliveryType == ENiosSkillDeliveryType::DelayedPoint || DeliveryType == ENiosSkillDeliveryType::AreaAtPoint || DeliveryType == ENiosSkillDeliveryType::TeleportToPoint", EditConditionHides, ClampMin = "0"))
    float ExtraDelay = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::AreaAtPoint", EditConditionHides, ClampMin = "0"))
    float ImpactRadius = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::AreaAtPoint", EditConditionHides))
    int32 MaxTargets = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Cue",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::DelayedTarget || DeliveryType == ENiosSkillDeliveryType::DelayedPoint", EditConditionHides))
    FGameplayTag TravelCueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Cue",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::DelayedTarget || DeliveryType == ENiosSkillDeliveryType::DelayedPoint || DeliveryType == ENiosSkillDeliveryType::AreaAtPoint || DeliveryType == ENiosSkillDeliveryType::TeleportToPoint", EditConditionHides))
    FGameplayTag ImpactCueTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::DelayedTarget || DeliveryType == ENiosSkillDeliveryType::DelayedPoint || DeliveryType == ENiosSkillDeliveryType::AreaAtPoint || DeliveryType == ENiosSkillDeliveryType::TeleportToPoint", EditConditionHides))
    int32 RandomSeed = 0;


    /** Optional soft Niagara FX spawned at StartPointSource. Soft reference avoids blocking loads; if not loaded yet it is requested async. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|FX")
    TSoftObjectPtr<UNiagaraSystem> StartPointNiagara;

    /** Optional soft Niagara FX spawned at ImpactPointSource. Useful for Teleport/Blink end point or AoE impact. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|FX")
    TSoftObjectPtr<UNiagaraSystem> ImpactPointNiagara;

    /** If true, impact Niagara uses direction from start to impact as yaw. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|FX")
    bool bOrientImpactNiagaraToTravelDirection = true;



    /** Adds a small extra lift above the capsule half-height when teleporting to a ground point. Prevents clipping into uneven floors. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint", EditConditionHides, ClampMin = "0"))
    float TeleportGroundOffset = 4.f;

    /** If true, TeleportTo will try collision-safe placement instead of forcing the actor into geometry. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint", EditConditionHides))
    bool bTeleportNoCheck = false;

    /** Optional yaw behavior for simple blink/teleport. If false, keeps current actor rotation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint", EditConditionHides))
    bool bFaceTeleportDirection = false;

    /** If exact teleport point is blocked, try to place the actor at the nearest safe point around it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport|Fallback",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint && !bTeleportNoCheck", EditConditionHides))
    bool bAllowNearbyTeleportFallback = true;

    /** Maximum horizontal distance from requested point used by nearby fallback search. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport|Fallback",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint && !bTeleportNoCheck && bAllowNearbyTeleportFallback", EditConditionHides, ClampMin = "0"))
    float NearbyTeleportSearchRadius = 250.f;

    /** Ring spacing for nearby fallback search. Smaller is more precise but costs more traces/overlaps. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport|Fallback",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint && !bTeleportNoCheck && bAllowNearbyTeleportFallback", EditConditionHides, ClampMin = "10", UIMin = "10"))
    float NearbyTeleportSearchStep = 50.f;

    /** Hard cap for fallback candidate checks. Prevents expensive searches under load. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport|Fallback",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint && !bTeleportNoCheck && bAllowNearbyTeleportFallback", EditConditionHides, ClampMin = "1", UIMin = "1"))
    int32 NearbyTeleportMaxAttempts = 32;

    /** If true, fallback point must have clear line of sight from requested point. Use for blink skills that must not slide around walls. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Skill|Delivery|Teleport|Fallback",
        meta = (EditCondition = "DeliveryType == ENiosSkillDeliveryType::TeleportToPoint && !bTeleportNoCheck && bAllowNearbyTeleportFallback", EditConditionHides))
    bool bNearbyTeleportRequireLineOfSight = true;

    FORCEINLINE bool IsDelayed() const
    {
        return DeliveryType == ENiosSkillDeliveryType::DelayedTarget
            || DeliveryType == ENiosSkillDeliveryType::DelayedPoint
            || TravelSpeed > KINDA_SMALL_NUMBER
            || ExtraDelay > KINDA_SMALL_NUMBER;
    }
};

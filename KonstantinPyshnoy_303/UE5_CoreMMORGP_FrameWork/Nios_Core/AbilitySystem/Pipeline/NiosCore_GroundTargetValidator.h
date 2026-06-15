#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NiosCore_GroundTargetValidator.generated.h"

UENUM(BlueprintType)
enum class ENiosCoreGroundTargetValidationFailReason : uint8
{
    None UMETA(DisplayName="None"),
    MissingWorld UMETA(DisplayName="Missing World"),
    MissingSource UMETA(DisplayName="Missing Source"),
    OutOfRange UMETA(DisplayName="Out Of Range"),
    NoGroundHit UMETA(DisplayName="No Ground Hit"),
    LineOfSightBlocked UMETA(DisplayName="Line Of Sight Blocked"),
    InvalidLocation UMETA(DisplayName="Invalid Location")
};

USTRUCT(BlueprintType)
struct FNiosCoreGroundTargetValidationSettings
{
    GENERATED_BODY()

    // Maximum allowed distance from source actor to requested ground point. <= 0 means unlimited.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target")
    float MaxRange = 1200.f;

    // If true, server clamps requested location to MaxRange instead of rejecting it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target")
    bool bClampToMaxRange = true;

    // Ground targeting range should usually be measured on XY plane. This prevents Z differences from shrinking usable range
    // and guarantees that over-range clicks resolve to the closest point in the clicked direction.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target", meta=(EditCondition="bClampToMaxRange", EditConditionHides))
    bool bClampRangeIn2D = true;

    // If true, server performs its own ground trace around the requested point.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target")
    bool bRequireGroundHit = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target", meta=(EditCondition="bRequireGroundHit"))
    float GroundTraceUp = 500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target", meta=(EditCondition="bRequireGroundHit"))
    float GroundTraceDown = 2500.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target", meta=(EditCondition="bRequireGroundHit"))
    TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

    // Optional LoS validation from source actor to final ground point.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target")
    bool bRequireLineOfSight = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ground Target", meta=(EditCondition="bRequireLineOfSight"))
    TEnumAsByte<ECollisionChannel> LineOfSightTraceChannel = ECC_Visibility;
};

USTRUCT(BlueprintType)
struct FNiosCoreGroundTargetValidationResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Nios|Ground Target")
    bool bValid = false;

    UPROPERTY(BlueprintReadOnly, Category="Nios|Ground Target")
    ENiosCoreGroundTargetValidationFailReason FailReason = ENiosCoreGroundTargetValidationFailReason::None;

    UPROPERTY(BlueprintReadOnly, Category="Nios|Ground Target")
    FVector RequestedLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Nios|Ground Target")
    FVector ValidatedLocation = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, Category="Nios|Ground Target")
    float DistanceFromSource = 0.f;

    UPROPERTY(BlueprintReadOnly, Category="Nios|Ground Target")
    bool bWasClamped = false;
};

/**
 * Server-side validator for client-provided ground-target locations.
 *
 * This class intentionally stores no state and performs no ticking.
 * Client targeting preview is local-only; server validates the final requested point once.
 */
UCLASS()
class NIOS_CORE_API UNiosCore_GroundTargetValidator : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Nios|Ground Target", meta=(WorldContext="WorldContextObject"))
    static bool ValidateGroundTargetLocation(
        const UObject* WorldContextObject,
        AActor* SourceActor,
        const FVector& RequestedLocation,
        const FNiosCoreGroundTargetValidationSettings& Settings,
        FNiosCoreGroundTargetValidationResult& OutResult);
};

#include "AbilitySystem/Pipeline/NiosCore_GroundTargetValidator.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

bool UNiosCore_GroundTargetValidator::ValidateGroundTargetLocation(
    const UObject* WorldContextObject,
    AActor* SourceActor,
    const FVector& RequestedLocation,
    const FNiosCoreGroundTargetValidationSettings& Settings,
    FNiosCoreGroundTargetValidationResult& OutResult)
{
    OutResult = FNiosCoreGroundTargetValidationResult();
    OutResult.RequestedLocation = RequestedLocation;
    OutResult.ValidatedLocation = RequestedLocation;

    if (RequestedLocation.ContainsNaN())
    {
        OutResult.FailReason = ENiosCoreGroundTargetValidationFailReason::InvalidLocation;
        return false;
    }

    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    if (!World)
    {
        OutResult.FailReason = ENiosCoreGroundTargetValidationFailReason::MissingWorld;
        return false;
    }

    if (!SourceActor)
    {
        OutResult.FailReason = ENiosCoreGroundTargetValidationFailReason::MissingSource;
        return false;
    }

    const FVector SourceLocation = SourceActor->GetActorLocation();
    FVector CandidateLocation = RequestedLocation;

    const float MaxRange = FMath::Max(0.f, Settings.MaxRange);
    FVector SourceToCandidate = CandidateLocation - SourceLocation;
    if (Settings.bClampRangeIn2D)
    {
        SourceToCandidate.Z = 0.f;
    }

    OutResult.DistanceFromSource = SourceToCandidate.Size();

    if (MaxRange > 0.f && OutResult.DistanceFromSource > MaxRange)
    {
        if (!Settings.bClampToMaxRange)
        {
            OutResult.FailReason = ENiosCoreGroundTargetValidationFailReason::OutOfRange;
            return false;
        }

        const FVector ClampDirection = SourceToCandidate.GetSafeNormal();
        CandidateLocation = SourceLocation + ClampDirection * MaxRange;

        // Preserve the originally requested Z until the ground trace below resolves the final floor point.
        // Without this, high/low caster Z can make the vertical trace miss valid ground near max range.
        if (Settings.bClampRangeIn2D)
        {
            CandidateLocation.Z = RequestedLocation.Z;
        }

        OutResult.bWasClamped = true;
        OutResult.DistanceFromSource = MaxRange;
    }

    if (Settings.bRequireGroundHit)
    {
        const FVector TraceStart = CandidateLocation + FVector(0.f, 0.f, FMath::Max(0.f, Settings.GroundTraceUp));
        const FVector TraceEnd = CandidateLocation - FVector(0.f, 0.f, FMath::Max(0.f, Settings.GroundTraceDown));

        FHitResult GroundHit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(NiosGroundTargetValidation), false);
        Params.AddIgnoredActor(SourceActor);

        if (!World->LineTraceSingleByChannel(GroundHit, TraceStart, TraceEnd, Settings.GroundTraceChannel, Params) || !GroundHit.bBlockingHit)
        {
            OutResult.FailReason = ENiosCoreGroundTargetValidationFailReason::NoGroundHit;
            return false;
        }

        CandidateLocation = GroundHit.ImpactPoint;
    }

    if (Settings.bRequireLineOfSight)
    {
        FVector EyeLocation = SourceLocation;
        FRotator EyeRotation = FRotator::ZeroRotator;
        SourceActor->GetActorEyesViewPoint(EyeLocation, EyeRotation);

        FHitResult LoSHit;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(NiosGroundTargetLineOfSight), false);
        Params.AddIgnoredActor(SourceActor);

        const FVector LoSTarget = CandidateLocation + FVector(0.f, 0.f, 20.f);
        const bool bBlocked = World->LineTraceSingleByChannel(LoSHit, EyeLocation, LoSTarget, Settings.LineOfSightTraceChannel, Params) && LoSHit.bBlockingHit;
        if (bBlocked)
        {
            OutResult.FailReason = ENiosCoreGroundTargetValidationFailReason::LineOfSightBlocked;
            return false;
        }
    }

    OutResult.bValid = true;
    OutResult.FailReason = ENiosCoreGroundTargetValidationFailReason::None;
    OutResult.ValidatedLocation = CandidateLocation;
    OutResult.DistanceFromSource = FVector::Distance(SourceLocation, CandidateLocation);
    return true;
}

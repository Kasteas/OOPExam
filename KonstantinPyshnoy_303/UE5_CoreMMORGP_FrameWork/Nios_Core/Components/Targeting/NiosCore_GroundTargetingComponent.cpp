#include "Components/Targeting/NiosCore_GroundTargetingComponent.h"

#include "Character/NiosCore_PlayerController.h"
#include "GameFramework/Pawn.h"

UNiosCore_GroundTargetingComponent::UNiosCore_GroundTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(false);
}

void UNiosCore_GroundTargetingComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(false);
}

void UNiosCore_GroundTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (bGroundTargetingActive)
    {
        FinishGroundTargeting(ENiosCoreGroundTargetingFinishReason::Cancelled, false);
    }

    Super::EndPlay(EndPlayReason);
}

void UNiosCore_GroundTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bGroundTargetingActive)
    {
        return;
    }

    UpdateCurrentTargetLocation();
}

void UNiosCore_GroundTargetingComponent::StartGroundTargeting(FName InSkillID, float InMaxRange)
{
    PendingSkillID = InSkillID;
    MaxRange = FMath::Max(0.f, InMaxRange);
    bGroundTargetingActive = true;
    bCurrentTargetValid = false;
    bHasCurrentTargetLocation = false;
    CurrentTargetLocation = FVector::ZeroVector;

    SetComponentTickEnabled(true);
    UpdateCurrentTargetLocation();

    OnGroundTargetingStateChanged.Broadcast(true, PendingSkillID);
    BP_OnGroundTargetingStateChanged(true, PendingSkillID);

    UE_LOG(LogTemp, Warning, TEXT(">>> GROUND TARGETING START: SkillID=%s MaxRange=%.2f Owner=%s"),
        *PendingSkillID.ToString(),
        MaxRange,
        *GetNameSafe(GetOwner()));
}

void UNiosCore_GroundTargetingComponent::CancelGroundTargeting()
{
    if (!bGroundTargetingActive)
    {
        return;
    }

    FinishGroundTargeting(ENiosCoreGroundTargetingFinishReason::Cancelled, false);
}

bool UNiosCore_GroundTargetingComponent::ConfirmGroundTargeting()
{
    if (!bGroundTargetingActive)
    {
        return false;
    }

    UpdateCurrentTargetLocation();

    if (!bCurrentTargetValid)
    {
        if (bConfirmWithLastValidLocationOnTraceMiss && bHasCurrentTargetLocation)
        {
            bCurrentTargetValid = true;
            UE_LOG(LogTemp, Warning, TEXT(">>> GROUND TARGETING CONFIRM USING LAST VALID LOCATION: SkillID=%s Location=%s Owner=%s"),
                *PendingSkillID.ToString(),
                *CurrentTargetLocation.ToString(),
                *GetNameSafe(GetOwner()));
        }
        else
        {
            FinishGroundTargeting(ENiosCoreGroundTargetingFinishReason::Invalid, false);
            return false;
        }
    }

    FinishGroundTargeting(ENiosCoreGroundTargetingFinishReason::Confirmed, true);
    return true;
}

bool UNiosCore_GroundTargetingComponent::UpdateCurrentTargetLocation()
{
    FHitResult Hit;
    if (!GetCursorGroundHit(Hit))
    {
        if (bRequireCursorHit)
        {
            bCurrentTargetValid = bConfirmWithLastValidLocationOnTraceMiss && bHasCurrentTargetLocation;
        }
        else
        {
            bCurrentTargetValid = true;
        }

        OnGroundTargetingUpdated.Broadcast(PendingSkillID, CurrentTargetLocation, bCurrentTargetValid);
        BP_OnGroundTargetingUpdated(PendingSkillID, CurrentTargetLocation, bCurrentTargetValid);
        return bCurrentTargetValid;
    }

    FVector DesiredLocation = Hit.ImpactPoint;

    if (bClampToMaxRange && MaxRange > 0.f)
    {
        const FVector SourceLocation = GetSourceLocationForRange();
        FVector ToTarget = DesiredLocation - SourceLocation;
        if (bClampRangeIn2D)
        {
            ToTarget.Z = 0.f;
        }

        const float Distance = ToTarget.Size();

        if (Distance > MaxRange && Distance > KINDA_SMALL_NUMBER)
        {
            const float OriginalZ = DesiredLocation.Z;
            DesiredLocation = SourceLocation + ToTarget.GetSafeNormal() * MaxRange;
            if (bClampRangeIn2D)
            {
                DesiredLocation.Z = OriginalZ;
            }
        }
    }

    CurrentTargetLocation = DesiredLocation;
    bHasCurrentTargetLocation = true;
    bCurrentTargetValid = true;

    OnGroundTargetingUpdated.Broadcast(PendingSkillID, CurrentTargetLocation, bCurrentTargetValid);
    BP_OnGroundTargetingUpdated(PendingSkillID, CurrentTargetLocation, bCurrentTargetValid);

    return true;
}

bool UNiosCore_GroundTargetingComponent::GetCursorGroundHit(FHitResult& OutHit) const
{
    const APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (!PC)
    {
        return false;
    }

    return PC->GetHitResultUnderCursor(GroundTraceChannel, false, OutHit);
}

FVector UNiosCore_GroundTargetingComponent::GetSourceLocationForRange() const
{
    const APlayerController* PC = Cast<APlayerController>(GetOwner());
    if (PC && PC->GetPawn())
    {
        return PC->GetPawn()->GetActorLocation();
    }

    return GetOwner() ? GetOwner()->GetActorLocation() : FVector::ZeroVector;
}

void UNiosCore_GroundTargetingComponent::FinishGroundTargeting(ENiosCoreGroundTargetingFinishReason Reason, bool bSendToServer)
{
    const FName FinishedSkillID = PendingSkillID;
    const FVector FinishedLocation = CurrentTargetLocation;

    bGroundTargetingActive = false;
    bCurrentTargetValid = false;
    bHasCurrentTargetLocation = false;
    PendingSkillID = NAME_None;
    MaxRange = 0.f;

    SetComponentTickEnabled(false);

    OnGroundTargetingFinished.Broadcast(Reason, FinishedSkillID, FinishedLocation);
    BP_OnGroundTargetingFinished(Reason, FinishedSkillID, FinishedLocation);

    OnGroundTargetingStateChanged.Broadcast(false, FinishedSkillID);
    BP_OnGroundTargetingStateChanged(false, FinishedSkillID);

    UE_LOG(LogTemp, Warning, TEXT(">>> GROUND TARGETING FINISH: Reason=%d SkillID=%s Location=%s SendToServer=%d Owner=%s"),
        static_cast<int32>(Reason),
        *FinishedSkillID.ToString(),
        *FinishedLocation.ToString(),
        bSendToServer ? 1 : 0,
        *GetNameSafe(GetOwner()));

    if (bSendToServer)
    {
        if (ANiosCore_PlayerController* PC = Cast<ANiosCore_PlayerController>(GetOwner()))
        {
            PC->TryActivateGroundSkill(FinishedSkillID, FinishedLocation);
        }
    }
}

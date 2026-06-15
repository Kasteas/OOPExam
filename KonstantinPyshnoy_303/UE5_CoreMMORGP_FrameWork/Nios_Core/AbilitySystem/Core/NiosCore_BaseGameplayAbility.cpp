#include "AbilitySystem/Core/NiosCore_BaseGameplayAbility.h"

#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Components/CapsuleComponent.h"
#include "Components/AudioComponent.h"
#include "GameplayTagContainer.h"
#include "TimerManager.h"
#include "CollisionShape.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

#include "Rules/Targeting/NiosTargetRulesSubsystem.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillTargetResolver.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillEffectProcessor.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillVisualDispatcher.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillPipeline.h"

#include "AbilitySystem/Executions/NiosAbilityExecution.h"
#include "AbilitySystem/Executions/NiosExec_ApplyToSelf.h"
#include "AbilitySystem/Executions/NiosExec_ApplyToTarget.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Character/NiosCore_Character.h"
#include "Components/Feedback/NiosCore_ActionFeedbackComponent.h"
#include "Components/Feedback/NiosCore_ActionFeedbackTypes.h"

static ENiosCoreActionFailReason Nios_MapCastInterruptReasonToActionFailReason(FName Reason)
{
    if (Reason == TEXT("SourceDeadDuringCast"))
    {
        return ENiosCoreActionFailReason::Dead;
    }

    if (Reason == TEXT("TargetInvalidDuringCast"))
    {
        return ENiosCoreActionFailReason::InvalidTarget;
    }

    if (Reason == TEXT("MovedDuringCast"))
    {
        return ENiosCoreActionFailReason::Moved;
    }

    if (Reason == TEXT("InterruptTagDuringCast") || Reason == TEXT("Cancelled"))
    {
        return ENiosCoreActionFailReason::Interrupted;
    }

    return ENiosCoreActionFailReason::AbilityBlocked;
}

UNiosCore_BaseGameplayAbility::UNiosCore_BaseGameplayAbility()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

FName UNiosCore_BaseGameplayAbility::GetSkillID() const
{
    const UNiosCore_AbilitySystemComponent* NiosASC = GetNiosASC();
    if (!NiosASC)
    {
        return NAME_None;
    }

    return NiosASC->GetSkillIDBySpecHandle(CachedSpecHandle);
}

UNiosCore_SkillDataAsset* UNiosCore_BaseGameplayAbility::GetSkillData() const
{
    const FGameplayAbilitySpec* Spec = GetCurrentAbilitySpec();
    return Spec ? Cast<UNiosCore_SkillDataAsset>(Spec->SourceObject.Get()) : nullptr;
}

const FGameplayAbilityTargetDataHandle& UNiosCore_BaseGameplayAbility::GetCachedTargetData() const
{
    return CachedTargetData;
}

AActor* UNiosCore_BaseGameplayAbility::GetCastLockedTarget() const
{
    return GetPrimaryActorFromCachedTargetData();
}

AActor* UNiosCore_BaseGameplayAbility::GetEffectiveLookAtTarget() const
{
    if (AActor* LockedTarget = GetCastLockedTarget())
    {
        return LockedTarget;
    }

    return GetNiosASC() ? GetNiosASC()->GetSelectedTarget() : nullptr;
}

UNiosCore_AbilitySystemComponent* UNiosCore_BaseGameplayAbility::GetNiosASC() const
{
    return Cast<UNiosCore_AbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

void UNiosCore_BaseGameplayAbility::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    FNiosCoreSkillPipeline::Activate(*this, Handle, ActorInfo, ActivationInfo);
}

bool UNiosCore_BaseGameplayAbility::CommitAbilityOnce()
{
    if (bAbilityCommitted)
        return true;

    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();

    if (!ActorInfo)
        return false;

    if (!CanPaySkillCosts())
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> ABILITY COMMIT FAILED: Not enough custom resources. Ability=%s Skill=%s"),
            *GetNameSafe(this),
            *GetNameSafe(GetSkillData()));
        return false;
    }

    if (!CommitAbility(CachedSpecHandle, ActorInfo, CachedActivationInfo))
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> ABILITY COMMIT FAILED: Ability=%s Skill=%s"), *GetNameSafe(this), *GetNameSafe(GetSkillData()));
        return false;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> ABILITY COMMIT OK - PAY ON EXECUTE: Skill=%s"), *GetNameSafe(GetSkillData()));

    PaySkillCosts();

    bAbilityCommitted = true;
    return true;
}

bool UNiosCore_BaseGameplayAbility::ValidateSkillTags() const
{
    const UNiosCore_SkillDataAsset* Skill = GetSkillData();
    const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    if (!Skill || !ASC)
    {
        return false;
    }

    if (!Skill->RequiredTags.IsEmpty() && !ASC->HasAllMatchingGameplayTags(Skill->RequiredTags))
    {
        return false;
    }

    if (!Skill->BlockedTags.IsEmpty() && ASC->HasAnyMatchingGameplayTags(Skill->BlockedTags))
    {
        return false;
    }

    return true;
}

bool UNiosCore_BaseGameplayAbility::CanPaySkillCosts() const
{
    const UNiosCore_SkillDataAsset* Skill = GetSkillData();
    const UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    if (!Skill || !ASC)
    {
        return false;
    }

    for (const FNiosSkillCostDefinition& Cost : Skill->Costs)
    {
        if (!Cost.IsValid() || !Cost.bSpendResource)
        {
            continue;
        }

        if (ASC->GetNumericAttribute(Cost.ResourceAttribute) + KINDA_SMALL_NUMBER < Cost.Value)
        {
            return false;
        }
    }

    return true;
}

void UNiosCore_BaseGameplayAbility::PaySkillCosts()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    if (!Skill || !ASC || !HasAuthority(&CachedActivationInfo))
    {
        return;
    }

    for (const FNiosSkillCostDefinition& Cost : Skill->Costs)
    {
        if (!Cost.IsValid())
        {
            continue;
        }

        const float Delta = Cost.bSpendResource ? -Cost.Value : Cost.Value;
        ASC->ApplyModToAttribute(Cost.ResourceAttribute, EGameplayModOp::Additive, Delta);
        FNiosCoreSkillEffectProcessor::ClampResourceAttribute(ASC, Cost.ResourceAttribute);
    }
}

void UNiosCore_BaseGameplayAbility::AddGrantedTagsWhileActive()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    if (!Skill || !ASC || Skill->GrantedTagsWhileActive.IsEmpty() || bGrantedActiveTagsAdded)
    {
        return;
    }

    ASC->AddLooseGameplayTags(Skill->GrantedTagsWhileActive);
    bGrantedActiveTagsAdded = true;
}

void UNiosCore_BaseGameplayAbility::RemoveGrantedTagsWhileActive()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();

    if (!Skill || !ASC || Skill->GrantedTagsWhileActive.IsEmpty() || !bGrantedActiveTagsAdded)
    {
        return;
    }

    ASC->RemoveLooseGameplayTags(Skill->GrantedTagsWhileActive);
    bGrantedActiveTagsAdded = false;
}

bool UNiosCore_BaseGameplayAbility::BuildTargetDataFromSelectedTarget()
{
    UNiosCore_AbilitySystemComponent* NiosASC = GetNiosASC();
    return NiosASC && FNiosCoreSkillTargetResolver::BuildActorTargetData(NiosASC->GetSelectedTarget(), CachedTargetData);
}

AActor* UNiosCore_BaseGameplayAbility::GetPrimaryActorFromCachedTargetData() const
{
    for (int32 Index = 0; Index < CachedTargetData.Num(); ++Index)
    {
        const FGameplayAbilityTargetData* Data = CachedTargetData.Get(Index);
        if (!Data)
        {
            continue;
        }

        for (const TWeakObjectPtr<AActor>& ActorPtr : Data->GetActors())
        {
            if (AActor* Actor = ActorPtr.Get())
            {
                return Actor;
            }
        }
    }

    return nullptr;
}

AActor* UNiosCore_BaseGameplayAbility::GetLockedOrSelectedTarget() const
{
    if (AActor* LockedTarget = GetPrimaryActorFromCachedTargetData())
    {
        return LockedTarget;
    }

    return GetNiosASC() ? GetNiosASC()->GetSelectedTarget() : nullptr;
}

bool UNiosCore_BaseGameplayAbility::ValidateCachedTargetData() const
{
    const UNiosCore_SkillDataAsset* Skill = GetSkillData();
    if (!Skill)
    {
        return false;
    }

    FName FailureReason = NAME_None;
    const bool bValid = FNiosCoreSkillTargetResolver::ValidateAbilityTarget(
        GetWorld(),
        GetAbilitySystemComponentFromActorInfo(),
        GetAvatarActorFromActorInfo(),
        GetLockedOrSelectedTarget(),
        Skill,
        CachedTargetData,
        FailureReason);

    if (!bValid)
    {
        AActor* SourceActor = GetAvatarActorFromActorInfo();
        AActor* TargetActor = GetLockedOrSelectedTarget();

        UE_LOG(LogTemp, Warning, TEXT(">>> TARGET VALIDATION FAILED: Skill=%s TargetType=%d AffectType=%d Reason=%s SelectedTarget=%s"),
            *GetNameSafe(Skill),
            static_cast<int32>(Skill->TargetType),
            static_cast<int32>(Skill->AffectType),
            *FailureReason.ToString(),
            *GetNameSafe(TargetActor));

        if (FailureReason == TEXT("TargetOutOfRange"))
        {
            FNiosCoreActionFailResult Result(ENiosCoreActionFailReason::TargetOutOfRange);
            Result.ContextId = Skill->GetFName();
            Result.SourceActor = SourceActor;
            Result.TargetActor = TargetActor;
            Result.RequiredValue = Skill->GetRange();
            Result.CurrentValue = FNiosCoreSkillTargetResolver::CalculateActorRangeDistance(SourceActor, TargetActor, Skill);
            Result.DebugReason = FailureReason;
            UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(SourceActor, Result);
        }
    }

    return bValid;
}

void UNiosCore_BaseGameplayAbility::StartCast()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    if (!Skill)
    {
        EndAbilityInternal();
        return;
    }

    const float FinalCastTime = FMath::Max(0.f, Skill->CastTime);

    /*
     *  CastTime  ,
     * CastLoop   .
     */
    if (FinalCastTime <= KINDA_SMALL_NUMBER)
    {
        OnCastFinished();
        return;
    }

    PlayCastLoopVisuals();

    if (HasAuthority(&CachedActivationInfo))
    {
        if (UNiosCore_AbilitySystemComponent* NiosASC = GetNiosASC())
        {
            NiosASC->BeginCastState(Skill, FinalCastTime, GetPrimaryActorFromCachedTargetData());
        }

        AddCastLoopGameplayCue();
    }

    BeginCastInterruptWatch();

    CastDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, FinalCastTime);
    if (!CastDelayTask)
    {
        InterruptCast();
        return;
    }

    CastDelayTask->OnFinish.AddDynamic(this, &UNiosCore_BaseGameplayAbility::OnCastFinished);
    CastDelayTask->ReadyForActivation();
}

void UNiosCore_BaseGameplayAbility::BeginCastInterruptWatch()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    AActor* SourceActor = GetAvatarActorFromActorInfo();
    UWorld* World = GetWorld();

    StopCastInterruptWatch();

    if (!Skill || !SourceActor || !World || !Skill->IsCastTimeSkill())
    {
        return;
    }

    CastStartLocation = SourceActor->GetActorLocation();
    CastInterruptReason = NAME_None;
    bCastInterruptWatchActive = true;

    const float Interval = FMath::Max(0.02f, Skill->CastInterruptCheckInterval);
    World->GetTimerManager().SetTimer(
        CastInterruptPollTimerHandle,
        this,
        &UNiosCore_BaseGameplayAbility::CheckCastInterruptConditions,
        Interval,
        true);

    UE_LOG(LogTemp, Warning, TEXT(">>> CAST INTERRUPT WATCH START: Skill=%s CancelOnMove=%d MoveDistance=%.2f CancelOnDeath=%d CancelOnTargetInvalid=%d InterruptTags=%s"),
        *GetNameSafe(Skill),
        Skill->bCancelCastOnMove ? 1 : 0,
        Skill->CastMoveCancelDistance,
        Skill->bCancelCastOnDeath ? 1 : 0,
        Skill->bCancelCastOnTargetInvalid ? 1 : 0,
        *Skill->CastInterruptTags.ToStringSimple());
}

void UNiosCore_BaseGameplayAbility::StopCastInterruptWatch()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(CastInterruptPollTimerHandle);
    }

    bCastInterruptWatchActive = false;
}

void UNiosCore_BaseGameplayAbility::CheckCastInterruptConditions()
{
    if (!bCastInterruptWatchActive || bWasCancelled || bGameplayExecutionStarted)
    {
        StopCastInterruptWatch();
        return;
    }

    const UNiosCore_SkillDataAsset* Skill = GetSkillData();
    UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
    AActor* SourceActor = GetAvatarActorFromActorInfo();

    if (!Skill || !ASC || !SourceActor)
    {
        InterruptCastWithReason(TEXT("MissingSourceDuringCast"));
        return;
    }

    if (Skill->bCancelCastOnMove)
    {
        const float MaxDistance = FMath::Max(0.f, Skill->CastMoveCancelDistance);
        if (MaxDistance > KINDA_SMALL_NUMBER)
        {
            const float DistSq = FVector::DistSquared2D(CastStartLocation, SourceActor->GetActorLocation());
            if (DistSq > FMath::Square(MaxDistance))
            {
                InterruptCastWithReason(TEXT("MovedDuringCast"));
                return;
            }
        }
    }

    if (!Skill->CastInterruptTags.IsEmpty() && ASC->HasAnyMatchingGameplayTags(Skill->CastInterruptTags))
    {
        InterruptCastWithReason(TEXT("InterruptTagDuringCast"));
        return;
    }

    if (Skill->bCancelCastOnDeath)
    {
        const float Health = ASC->GetNumericAttribute(UNiosCore_AttributeSet::GetHealthAttribute());
        if (Health <= KINDA_SMALL_NUMBER)
        {
            InterruptCastWithReason(TEXT("SourceDeadDuringCast"));
            return;
        }
    }

    if (Skill->bCancelCastOnTargetInvalid && !ValidateCachedTargetData())
    {
        InterruptCastWithReason(TEXT("TargetInvalidDuringCast"));
        return;
    }
}

void UNiosCore_BaseGameplayAbility::OnCastFinished()
{
    StopCastInterruptWatch();
    FNiosCoreSkillPipeline::OnCastFinished(*this);
}

void UNiosCore_BaseGameplayAbility::CancelAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateCancelAbility)
{
    InterruptCast();

    Super::CancelAbility(
        Handle,
        ActorInfo,
        ActivationInfo,
        bReplicateCancelAbility
    );
}

void UNiosCore_BaseGameplayAbility::InterruptCast()
{
    InterruptCastWithReason(TEXT("Cancelled"));
}

void UNiosCore_BaseGameplayAbility::InterruptCastWithReason(FName Reason)
{
    if (bWasCancelled)
        return;

    bWasCancelled = true;
    CastInterruptReason = Reason;

    StopCastInterruptWatch();

    if (CastDelayTask)
    {
        CastDelayTask->EndTask();
        CastDelayTask = nullptr;
    }

    StopCastLoopVisuals();

    if (HasAuthority(&CachedActivationInfo))
    {
        if (UNiosCore_AbilitySystemComponent* NiosASC = GetNiosASC())
        {
            NiosASC->EndCastState();
        }

        RemoveCastLoopGameplayCue();
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> CAST INTERRUPTED: Skill=%s Reason=%s CostPaid=%d GameplayStarted=%d"),
        *GetNameSafe(GetSkillData()),
        *CastInterruptReason.ToString(),
        bAbilityCommitted ? 1 : 0,
        bGameplayExecutionStarted ? 1 : 0);

    if (HasAuthority(&CachedActivationInfo) && !bSkillFeedbackFinalized)
    {
        bSkillFeedbackFinalized = true;

        const ENiosCoreActionFailReason ActionFailReason = Nios_MapCastInterruptReasonToActionFailReason(CastInterruptReason);
        AActor* SourceActor = GetAvatarActorFromActorInfo();
        AActor* TargetActor = GetPrimaryActorFromCachedTargetData();

        if (UNiosCore_AbilitySystemComponent* NiosASC = GetNiosASC())
        {
            NiosASC->NotifySkillActivationInterrupted(
                GetSkillID(),
                GetSkillData(),
                ActionFailReason,
                TargetActor,
                CastInterruptReason);
        }

        if (ActionFailReason != ENiosCoreActionFailReason::None)
        {
            FNiosCoreActionFailResult Result(ActionFailReason);
            Result.ContextId = GetSkillID();
            if (Result.ContextId.IsNone() && GetSkillData())
            {
                Result.ContextId = GetSkillData()->GetFName();
            }
            Result.SourceActor = SourceActor;
            Result.TargetActor = TargetActor;
            Result.DebugReason = CastInterruptReason;
            UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(SourceActor, Result);
        }
    }

    EndAbility(
        CachedSpecHandle,
        GetCurrentActorInfo(),
        CachedActivationInfo,
        true,
        true
    );
}

void UNiosCore_BaseGameplayAbility::ExecuteAbility()
{
    FNiosCoreSkillPipeline::Execute(*this);
}

void UNiosCore_BaseGameplayAbility::FinishAbilityAfterGameplay()
{
    FNiosCoreSkillPipeline::FinishAfterGameplay(*this);
}

void UNiosCore_BaseGameplayAbility::PlayCastLoopVisuals()
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::PlayCastLoopVisuals(Context, true, ActiveCastLoopAudioComponent);
}


void UNiosCore_BaseGameplayAbility::StopCastLoopVisuals()
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::PlayCastLoopVisuals(Context, false, ActiveCastLoopAudioComponent);
}


void UNiosCore_BaseGameplayAbility::ApplySkillGameplay()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    if (!Skill)
        return;

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE START: Skill=%s Policy=%d Effects=%d Deliveries=%d TargetType=%d AffectType=%d"),
        *GetNameSafe(Skill),
        static_cast<int32>(Skill->ExecutionPolicy),
        Skill->SkillEffects.Num(),
        Skill->Deliveries.Num(),
        static_cast<int32>(Skill->TargetType),
        static_cast<int32>(Skill->AffectType));

    ExecuteSkillGameplayCue();

    if (ShouldUseCustomExecutions())
    {
        ExecuteCustomSkillExecutions();
        return;
    }

    // DefaultDataDriven is the normal GD path: no Execution classes are required.
    if (Skill->Deliveries.Num() > 0)
    {
        ExecuteSkillDeliveries();
    }
    else
    {
        ApplySkillEffects();
    }
}

bool UNiosCore_BaseGameplayAbility::ShouldUseCustomExecutions() const
{
    const UNiosCore_SkillDataAsset* Skill = GetSkillData();
    return Skill
        && Skill->ExecutionPolicy == ENiosAbilityExecutionPolicy::CustomExecutions
        && Skill->Executions.Num() > 0;
}

void UNiosCore_BaseGameplayAbility::ExecuteCustomSkillExecutions()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    if (!Skill)
        return;

    for (TSubclassOf<UNiosAbilityExecution> ExecClass : Skill->Executions)
    {
        if (!ExecClass)
            continue;

        UNiosAbilityExecution* Exec = NewObject<UNiosAbilityExecution>(this, ExecClass);
        if (!Exec)
            continue;

        Exec->Execute(this);
    }
}

void UNiosCore_BaseGameplayAbility::ApplySkillEffects()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL EFFECTS START: Skill=%s Effects=%d"),
        *GetNameSafe(Skill),
        Skill ? Skill->SkillEffects.Num() : 0);

    ExecuteSkillEffectsByIndices(TArray<int32>());
}

void UNiosCore_BaseGameplayAbility::ExecuteSkillDeliveries()
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    if (!Skill)
    {
        return;
    }

    for (const FNiosSkillDeliveryDefinition& Delivery : Skill->Deliveries)
    {
        ExecuteSkillDelivery(Delivery);
    }
}

void UNiosCore_BaseGameplayAbility::ExecuteSkillDelivery(const FNiosSkillDeliveryDefinition& Delivery)
{
    const FVector StartPoint = ResolveSkillPoint(Delivery.StartPointSource);
    const FVector ImpactPoint = ResolveSkillPoint(Delivery.ImpactPointSource);

    const FVector TravelDirection = (ImpactPoint - StartPoint);
    const FRotator TravelRotation = TravelDirection.IsNearlyZero() ? FRotator::ZeroRotator : TravelDirection.Rotation();

    SpawnDeliveryNiagaraSoft(Delivery.StartPointNiagara, StartPoint, TravelRotation, TEXT("StartPointNiagara"));

    if (Delivery.TravelCueTag.IsValid())
    {
        ExecuteDeliveryGameplayCue(Delivery.TravelCueTag, StartPoint);
    }

    float Delay = FMath::Max(0.f, Delivery.ExtraDelay);

    if (Delivery.TravelSpeed > KINDA_SMALL_NUMBER &&
        (Delivery.DeliveryType == ENiosSkillDeliveryType::DelayedTarget ||
         Delivery.DeliveryType == ENiosSkillDeliveryType::DelayedPoint))
    {
        Delay += FVector::Distance(StartPoint, ImpactPoint) / Delivery.TravelSpeed;
    }

    if (Delay <= KINDA_SMALL_NUMBER && Delivery.DeliveryType == ENiosSkillDeliveryType::Instant)
    {
        ExecuteSkillEffectsByIndices(Delivery.EffectIndices);
        return;
    }

    if (Delay <= KINDA_SMALL_NUMBER)
    {
        ExecuteSkillDeliveryImpact(Delivery);
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        ExecuteSkillDeliveryImpact(Delivery);
        return;
    }

    BeginAsyncDeliveryImpact();

    FTimerDelegate TimerDelegate;
    TimerDelegate.BindUObject(this, &UNiosCore_BaseGameplayAbility::ExecuteSkillDeliveryImpact, Delivery);

    FTimerHandle TimerHandle;
    World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, Delay, false);
}

void UNiosCore_BaseGameplayAbility::BeginAsyncDeliveryImpact()
{
    ++PendingDeliveryImpacts;
}

void UNiosCore_BaseGameplayAbility::FinishAsyncDeliveryImpact()
{
    if (PendingDeliveryImpacts <= 0)
    {
        return;
    }

    PendingDeliveryImpacts = FMath::Max(0, PendingDeliveryImpacts - 1);
    FinishAbilityAfterGameplay();
}

void UNiosCore_BaseGameplayAbility::ExecuteSkillDeliveryImpact(FNiosSkillDeliveryDefinition Delivery)
{
    const FVector StartPoint = ResolveSkillPoint(Delivery.StartPointSource);
    const FVector ImpactPoint = ResolveSkillPoint(Delivery.ImpactPointSource);
    const FVector TravelDirection = (ImpactPoint - StartPoint);
    const FRotator ImpactRotation = (Delivery.bOrientImpactNiagaraToTravelDirection && !TravelDirection.IsNearlyZero())
        ? TravelDirection.Rotation()
        : FRotator::ZeroRotator;

    SpawnDeliveryNiagaraSoft(Delivery.ImpactPointNiagara, ImpactPoint, ImpactRotation, TEXT("ImpactPointNiagara"));

    const FGameplayTag ImpactCueTag = Delivery.ImpactCueTag.IsValid() ? Delivery.ImpactCueTag : ResolveImpactCueTag();
    if (ImpactCueTag.IsValid())
    {
        ExecuteDeliveryGameplayCue(ImpactCueTag, ImpactPoint);
    }

    if (Delivery.DeliveryType == ENiosSkillDeliveryType::TeleportToPoint)
    {
        const bool bTeleported = TeleportAvatarToPointSafe(ImpactPoint, Delivery);
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TELEPORT DELIVERY: Skill=%s Ground=%s Success=%d"),
            *GetNameSafe(GetSkillData()),
            *ImpactPoint.ToCompactString(),
            bTeleported ? 1 : 0);

        // Movement deliveries are movement-only by default.
        // Important: an empty EffectIndices list means "all effects" for normal effect deliveries,
        // but for TeleportToPoint it must mean "do not apply effects here".
        // Otherwise a skill with AreaAtPoint + TeleportToPoint will apply AoE damage first,
        // then apply the same effects again to the caster/source after teleport.
        if (bTeleported && Delivery.EffectIndices.Num() > 0)
        {
            ExecuteSkillEffectsByIndices(Delivery.EffectIndices);
        }

        FinishAsyncDeliveryImpact();
        return;
    }

    if (Delivery.DeliveryType == ENiosSkillDeliveryType::AreaAtPoint || Delivery.ImpactRadius > KINDA_SMALL_NUMBER)
    {
        TArray<UAbilitySystemComponent*> AreaTargetASCs;
        ResolveAreaTargetASCs(ImpactPoint, Delivery.ImpactRadius, Delivery.MaxTargets, AreaTargetASCs);
        ExecuteSkillEffectsByIndicesWithTargetOverride(Delivery.EffectIndices, AreaTargetASCs);
        FinishAsyncDeliveryImpact();
        return;
    }

    ExecuteSkillEffectsByIndices(Delivery.EffectIndices);
    FinishAsyncDeliveryImpact();
}

bool UNiosCore_BaseGameplayAbility::TeleportAvatarToPointSafe(const FVector& GroundPoint, const FNiosSkillDeliveryDefinition& Delivery)
{
    AActor* AvatarActor = GetAvatarActorFromActorInfo();
    UWorld* World = GetWorld();

    if (!AvatarActor || !World || GroundPoint.ContainsNaN())
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TELEPORT FAILED: invalid input Avatar=%s World=%d Ground=%s"),
            *GetNameSafe(AvatarActor),
            World ? 1 : 0,
            *GroundPoint.ToCompactString());
        return false;
    }

    FRotator TargetRotation = AvatarActor->GetActorRotation();

    const ACharacter* Character = Cast<ACharacter>(AvatarActor);
    const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;

    float HalfHeight = 0.f;
    float Radius = 0.f;
    if (Capsule)
    {
        HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        Radius = Capsule->GetScaledCapsuleRadius();
    }

    if (Delivery.bFaceTeleportDirection)
    {
        const FVector Direction = (GroundPoint - AvatarActor->GetActorLocation()).GetSafeNormal2D();
        if (!Direction.IsNearlyZero())
        {
            TargetRotation = Direction.Rotation();
        }
    }

    const auto BuildTeleportLocationFromGround = [&](const FVector& CandidateGroundPoint) -> FVector
    {
        FVector Location = CandidateGroundPoint;
        if (Capsule)
        {
            Location.Z += HalfHeight + FMath::Max(0.f, Delivery.TeleportGroundOffset);
        }
        else
        {
            Location.Z += FMath::Max(0.f, Delivery.TeleportGroundOffset);
        }
        return Location;
    };

    FCollisionQueryParams Params(SCENE_QUERY_STAT(NiosSkillTeleportSafety), false);
    Params.AddIgnoredActor(AvatarActor);

    const ECollisionChannel Channel = Capsule ? Capsule->GetCollisionObjectType() : ECC_Pawn;
    const FCollisionShape Shape = Capsule
        ? FCollisionShape::MakeCapsule(Radius, HalfHeight)
        : FCollisionShape::MakeSphere(34.f);

    const auto TryFindSafeTeleportLocation = [&](const FVector& CandidateGroundPoint, FVector& OutTeleportLocation) -> bool
    {
        FVector CandidateLocation = BuildTeleportLocationFromGround(CandidateGroundPoint);

        if (!Capsule || Delivery.bTeleportNoCheck)
        {
            OutTeleportLocation = CandidateLocation;
            return true;
        }

        if (!World->OverlapBlockingTestByChannel(CandidateLocation, TargetRotation.Quaternion(), Channel, Shape, Params))
        {
            OutTeleportLocation = CandidateLocation;
            return true;
        }

        // Uneven ground can make the first placement touch geometry. Try a few small upward nudges before rejecting.
        constexpr int32 MaxNudges = 4;
        constexpr float NudgeStep = 10.f;
        for (int32 Nudge = 1; Nudge <= MaxNudges; ++Nudge)
        {
            const FVector NudgedLocation = CandidateLocation + FVector(0.f, 0.f, NudgeStep * Nudge);
            if (!World->OverlapBlockingTestByChannel(NudgedLocation, TargetRotation.Quaternion(), Channel, Shape, Params))
            {
                OutTeleportLocation = NudgedLocation;
                return true;
            }
        }

        return false;
    };

    const auto ResolveGroundAtXY = [&](const FVector& CandidateXY, FVector& OutGroundPoint) -> bool
    {
        const FVector TraceStart(CandidateXY.X, CandidateXY.Y, CandidateXY.Z + 500.f);
        const FVector TraceEnd(CandidateXY.X, CandidateXY.Y, CandidateXY.Z - 1000.f);

        FHitResult Hit;
        if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params) && Hit.bBlockingHit)
        {
            OutGroundPoint = Hit.ImpactPoint;
            return true;
        }

        // If trace fails, keep the original ground Z. This preserves behavior on simple test maps that do not block Visibility.
        OutGroundPoint = CandidateXY;
        return true;
    };

    const auto HasFallbackLineOfSight = [&](const FVector& FromGround, const FVector& ToGround) -> bool
    {
        if (!Delivery.bNearbyTeleportRequireLineOfSight)
        {
            return true;
        }

        const FVector From = BuildTeleportLocationFromGround(FromGround);
        const FVector To = BuildTeleportLocationFromGround(ToGround);

        FHitResult Hit;
        const bool bBlocked = World->LineTraceSingleByChannel(Hit, From, To, ECC_Visibility, Params);
        return !bBlocked;
    };

    FVector TeleportLocation = BuildTeleportLocationFromGround(GroundPoint);
    if (!Capsule || Delivery.bTeleportNoCheck || TryFindSafeTeleportLocation(GroundPoint, TeleportLocation))
    {
        const bool bSuccess = AvatarActor->TeleportTo(TeleportLocation, TargetRotation, false, Delivery.bTeleportNoCheck);
        if (bSuccess)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TELEPORT SUCCESS: Skill=%s Ground=%s Location=%s NoCheck=%d Fallback=0"),
                *GetNameSafe(GetSkillData()),
                *GroundPoint.ToCompactString(),
                *TeleportLocation.ToCompactString(),
                Delivery.bTeleportNoCheck ? 1 : 0);
            return true;
        }

        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TELEPORT FAILED: TeleportTo rejected exact point Skill=%s Ground=%s Location=%s NoCheck=%d"),
            *GetNameSafe(GetSkillData()),
            *GroundPoint.ToCompactString(),
            *TeleportLocation.ToCompactString(),
            Delivery.bTeleportNoCheck ? 1 : 0);
    }

    if (!Capsule || Delivery.bTeleportNoCheck || !Delivery.bAllowNearbyTeleportFallback || Delivery.NearbyTeleportSearchRadius <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TELEPORT BLOCKED: Skill=%s Ground=%s Location=%s Radius=%.2f HalfHeight=%.2f FallbackAllowed=%d"),
            *GetNameSafe(GetSkillData()),
            *GroundPoint.ToCompactString(),
            *BuildTeleportLocationFromGround(GroundPoint).ToCompactString(),
            Radius,
            HalfHeight,
            Delivery.bAllowNearbyTeleportFallback ? 1 : 0);
        return false;
    }

    const float SearchStep = FMath::Max(10.f, Delivery.NearbyTeleportSearchStep);
    const float SearchRadius = FMath::Max(0.f, Delivery.NearbyTeleportSearchRadius);
    const int32 MaxAttempts = FMath::Max(1, Delivery.NearbyTeleportMaxAttempts);
    const int32 RingCount = FMath::Max(1, FMath::CeilToInt(SearchRadius / SearchStep));

    int32 Attempts = 0;
    for (int32 Ring = 1; Ring <= RingCount && Attempts < MaxAttempts; ++Ring)
    {
        const float RingRadius = FMath::Min(SearchRadius, Ring * SearchStep);
        const int32 SamplesOnRing = FMath::Clamp(Ring * 8, 8, 32);

        for (int32 Sample = 0; Sample < SamplesOnRing && Attempts < MaxAttempts; ++Sample)
        {
            ++Attempts;

            const float Angle = (2.f * PI * static_cast<float>(Sample)) / static_cast<float>(SamplesOnRing);
            const FVector Offset(FMath::Cos(Angle) * RingRadius, FMath::Sin(Angle) * RingRadius, 0.f);

            FVector CandidateGroundPoint;
            if (!ResolveGroundAtXY(GroundPoint + Offset, CandidateGroundPoint))
            {
                continue;
            }

            if (!HasFallbackLineOfSight(GroundPoint, CandidateGroundPoint))
            {
                continue;
            }

            FVector CandidateTeleportLocation;
            if (!TryFindSafeTeleportLocation(CandidateGroundPoint, CandidateTeleportLocation))
            {
                continue;
            }

            const bool bSuccess = AvatarActor->TeleportTo(CandidateTeleportLocation, TargetRotation, false, false);
            if (!bSuccess)
            {
                continue;
            }

            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TELEPORT SUCCESS: Skill=%s Ground=%s Location=%s NoCheck=0 Fallback=1 Attempts=%d FallbackGround=%s Distance=%.2f"),
                *GetNameSafe(GetSkillData()),
                *GroundPoint.ToCompactString(),
                *CandidateTeleportLocation.ToCompactString(),
                Attempts,
                *CandidateGroundPoint.ToCompactString(),
                FVector::Dist2D(GroundPoint, CandidateGroundPoint));
            return true;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TELEPORT BLOCKED: Skill=%s Ground=%s SearchRadius=%.2f Step=%.2f Attempts=%d Radius=%.2f HalfHeight=%.2f"),
        *GetNameSafe(GetSkillData()),
        *GroundPoint.ToCompactString(),
        SearchRadius,
        SearchStep,
        Attempts,
        Radius,
        HalfHeight);

    return false;
}

void UNiosCore_BaseGameplayAbility::ExecuteSkillEffectsByIndices(const TArray<int32>& EffectIndices)
{
    ExecuteSkillEffectsByIndicesWithTargetOverride(EffectIndices, TArray<UAbilitySystemComponent*>());
}

void UNiosCore_BaseGameplayAbility::ExecuteSkillEffectsByIndicesWithTargetOverride(
    const TArray<int32>& EffectIndices,
    const TArray<UAbilitySystemComponent*>& OverrideTargetASCs)
{
    UNiosCore_SkillDataAsset* Skill = GetSkillData();
    if (!Skill)
    {
        return;
    }

    FNiosCoreSkillEffectProcessParams Params;
    Params.World = GetWorld();
    Params.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Params.SourceActor = GetAvatarActorFromActorInfo();
    Params.SelectedActor = GetLockedOrSelectedTarget();
    Params.Skill = Skill;
    Params.TargetData = &CachedTargetData;
    Params.OverrideTargetASCs = OverrideTargetASCs;
    Params.OnEffectApplied = [this](UAbilitySystemComponent* TargetASC, const FNiosSkillEffectDefinition& Effect)
    {
        ExecuteImpactGameplayCue(TargetASC, Effect);
    };

    FNiosCoreSkillEffectProcessor::ApplyEffectsByIndices(Params, EffectIndices);
}

FVector UNiosCore_BaseGameplayAbility::ResolveSkillPoint(ENiosSkillPointSource PointSource) const
{
    return FNiosCoreSkillTargetResolver::ResolvePoint(
        GetAvatarActorFromActorInfo(),
        GetLockedOrSelectedTarget(),
        CachedTargetData,
        PointSource);
}

void UNiosCore_BaseGameplayAbility::ResolveAreaTargetASCs(
    const FVector& Point,
    float Radius,
    int32 MaxTargets,
    TArray<UAbilitySystemComponent*>& OutTargetASCs) const
{
    OutTargetASCs.Reset();

    UWorld* World = GetWorld();
    const UNiosCore_SkillDataAsset* Skill = GetSkillData();
    AActor* SourceActor = GetAvatarActorFromActorInfo();

    if (!World || !Skill || Radius <= KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> AREA TARGETS SKIPPED: Skill=%s World=%d Radius=%.2f"),
            *GetNameSafe(Skill),
            World ? 1 : 0,
            Radius);
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NiosSkillArea), false);

    // Harmful AoE should not hit the caster by default. Help/Neutral AoE may include self if the caster is inside the radius.
    if (SourceActor && Skill->AffectType == ENiosAbilityAffectType::Harm)
    {
        QueryParams.AddIgnoredActor(SourceActor);
    }

    FCollisionObjectQueryParams ObjectQueryParams;
    ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
    ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
    ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

    const bool bAnyHit = World->OverlapMultiByObjectType(
        Overlaps,
        Point,
        FQuat::Identity,
        ObjectQueryParams,
        FCollisionShape::MakeSphere(Radius),
        QueryParams
    );

    if (!bAnyHit)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> AREA TARGETS NONE: Skill=%s Point=%s Radius=%.2f"),
            *GetNameSafe(Skill),
            *Point.ToCompactString(),
            Radius);
        return;
    }

    Overlaps.Sort([Point](const FOverlapResult& A, const FOverlapResult& B)
    {
        const AActor* ActorA = A.GetActor();
        const AActor* ActorB = B.GetActor();
        const float DistA = ActorA ? FVector::DistSquared(Point, ActorA->GetActorLocation()) : TNumericLimits<float>::Max();
        const float DistB = ActorB ? FVector::DistSquared(Point, ActorB->GetActorLocation()) : TNumericLimits<float>::Max();
        return DistA < DistB;
    });

    UNiosTargetRulesSubsystem* Rules = World->GetSubsystem<UNiosTargetRulesSubsystem>();

    int32 Candidates = 0;
    int32 RelationBlocked = 0;

    for (const FOverlapResult& Overlap : Overlaps)
    {
        AActor* Actor = Overlap.GetActor();
        if (!Actor)
        {
            continue;
        }

        UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
        if (!TargetASC)
        {
            continue;
        }

        ++Candidates;

        if (SourceActor && Actor != SourceActor && Rules && !Rules->CanUseSkillOnTarget(SourceActor, Actor, Skill))
        {
            ++RelationBlocked;
            continue;
        }

        // Self is allowed for non-harmful AoE, but keep harmful self-hit blocked unless explicitly modeled later.
        if (SourceActor && Actor == SourceActor && Skill->AffectType == ENiosAbilityAffectType::Harm)
        {
            continue;
        }

        OutTargetASCs.AddUnique(TargetASC);

        if (MaxTargets > 0 && OutTargetASCs.Num() >= MaxTargets)
        {
            break;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> AREA TARGETS RESOLVED: Skill=%s Point=%s Radius=%.2f MaxTargets=%d Candidates=%d RelationBlocked=%d ResultASCs=%d"),
        *GetNameSafe(Skill),
        *Point.ToCompactString(),
        Radius,
        MaxTargets,
        Candidates,
        RelationBlocked,
        OutTargetASCs.Num());
}


void UNiosCore_BaseGameplayAbility::SpawnDeliveryNiagaraSoft(
    const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem,
    const FVector& Location,
    const FRotator& Rotation,
    FName DebugName) const
{
    if (NiagaraSystem.IsNull())
    {
        return;
    }

    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    // Cosmetic delivery FX must not be spawned or loaded by dedicated servers.
    // The server calls into the visual dispatcher, which multicasts a lightweight visual request.
    // Clients perform soft/async loading and local Niagara spawn.
    FNiosCoreSkillVisualDispatcher::DispatchDeliveryNiagara(Context, NiagaraSystem, Location, Rotation, DebugName);
}

void UNiosCore_BaseGameplayAbility::ExecuteDeliveryGameplayCue(FGameplayTag CueTag, const FVector& CueLocation)
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::ExecuteDeliveryGameplayCue(Context, CueTag, CueLocation);
}


float UNiosCore_BaseGameplayAbility::CalculateSkillEffectValue(const FNiosSkillEffectDefinition& Effect) const
{
    return FNiosCoreSkillEffectProcessor::CalculateEffectValue(GetAbilitySystemComponentFromActorInfo(), Effect);
}

void UNiosCore_BaseGameplayAbility::ResolveSkillEffectTargetASCs(
    const FNiosSkillEffectDefinition& Effect,
    TArray<UAbilitySystemComponent*>& OutTargetASCs) const
{
    FNiosCoreSkillTargetResolver::ResolveEffectTargetASCs(
        GetWorld(),
        GetAbilitySystemComponentFromActorInfo(),
        GetAvatarActorFromActorInfo(),
        GetLockedOrSelectedTarget(),
        GetSkillData(),
        Effect,
        CachedTargetData,
        OutTargetASCs);
}

void UNiosCore_BaseGameplayAbility::ApplySkillEffectToASC(
    const FNiosSkillEffectDefinition& Effect,
    UAbilitySystemComponent* TargetASC)
{
    if (!TargetASC || !GetSkillData())
    {
        return;
    }

    FNiosCoreSkillEffectProcessParams Params;
    Params.World = GetWorld();
    Params.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Params.SourceActor = GetAvatarActorFromActorInfo();
    Params.SelectedActor = GetLockedOrSelectedTarget();
    Params.Skill = GetSkillData();
    Params.TargetData = &CachedTargetData;
    Params.OverrideTargetASCs.Add(TargetASC);
    Params.OnEffectApplied = [this](UAbilitySystemComponent* AppliedTargetASC, const FNiosSkillEffectDefinition& AppliedEffect)
    {
        ExecuteImpactGameplayCue(AppliedTargetASC, AppliedEffect);
    };

    // Compatibility path for custom executions that still call ability-level ApplySkillEffectToASC.
    FNiosCoreSkillEffectProcessor::ApplyEffectsByIndices(Params, TArray<int32>());
}

FGameplayTag UNiosCore_BaseGameplayAbility::ResolveDefaultCueTag(FName CueEvent) const
{
    return FNiosCoreSkillVisualDispatcher::ResolveDefaultCueTag(GetSkillData(), CueEvent);
}


FGameplayTag UNiosCore_BaseGameplayAbility::ResolveCastLoopCueTag() const
{
    return FNiosCoreSkillVisualDispatcher::ResolveCastLoopCueTag(GetSkillData());
}


FGameplayTag UNiosCore_BaseGameplayAbility::ResolveExecuteCueTag() const
{
    return FNiosCoreSkillVisualDispatcher::ResolveExecuteCueTag(GetSkillData());
}


FGameplayTag UNiosCore_BaseGameplayAbility::ResolveImpactCueTag() const
{
    return FNiosCoreSkillVisualDispatcher::ResolveImpactCueTag(GetSkillData());
}


FGameplayTag UNiosCore_BaseGameplayAbility::ResolveFailCueTag() const
{
    return FNiosCoreSkillVisualDispatcher::ResolveFailCueTag(GetSkillData());
}


void UNiosCore_BaseGameplayAbility::AddCastLoopGameplayCue()
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::AddCastLoopGameplayCue(Context, bCastLoopCueAdded);
}


void UNiosCore_BaseGameplayAbility::RemoveCastLoopGameplayCue()
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::RemoveCastLoopGameplayCue(Context, bCastLoopCueAdded);
}


void UNiosCore_BaseGameplayAbility::ExecuteSkillGameplayCue()
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::ExecuteSkillGameplayCue(Context, bGameplayCueExecuted);
}


void UNiosCore_BaseGameplayAbility::ExecuteImpactGameplayCue(
    UAbilitySystemComponent* TargetASC,
    const FNiosSkillEffectDefinition& Effect)
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::ExecuteImpactGameplayCue(
        Context,
        TargetASC,
        Effect,
        CalculateSkillEffectValue(Effect));
}


void UNiosCore_BaseGameplayAbility::EndAbilityInternal()
{
    StopCastLoopVisuals();
    RemoveGrantedTagsWhileActive();

    if (HasAuthority(&CachedActivationInfo))
    {
        if (UNiosCore_AbilitySystemComponent* NiosASC = GetNiosASC())
        {
            NiosASC->EndCastState();
        }

        RemoveCastLoopGameplayCue();
    }

    EndAbility(
        CachedSpecHandle,
        GetCurrentActorInfo(),
        CachedActivationInfo,
        false,
        false
    );
}
void UNiosCore_BaseGameplayAbility::PlayExecuteVisuals()
{
    FNiosSkillVisualContext Context;
    Context.SourceASC = GetAbilitySystemComponentFromActorInfo();
    Context.SourceActor = GetAvatarActorFromActorInfo();
    Context.Skill = GetSkillData();
    Context.bHasAuthority = HasAuthority(&CachedActivationInfo);

    FNiosCoreSkillVisualDispatcher::PlayExecuteVisuals(Context);
}

void UNiosCore_BaseGameplayAbility::OnTargetDataReady(
    const FGameplayAbilityTargetDataHandle& Data)
{
    FNiosCoreSkillPipeline::HandleTargetDataReady(*this, Data);
}

void UNiosCore_BaseGameplayAbility::OnTargetDataCancelled(
    const FGameplayAbilityTargetDataHandle& Data)
{
    EndAbility(
        CachedSpecHandle,
        GetCurrentActorInfo(),
        CachedActivationInfo,
        true,
        true
    );
}
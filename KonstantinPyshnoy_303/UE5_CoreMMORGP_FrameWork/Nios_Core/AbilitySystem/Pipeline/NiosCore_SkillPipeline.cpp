#include "AbilitySystem/Pipeline/NiosCore_SkillPipeline.h"

#include "AbilitySystem/Core/NiosCore_BaseGameplayAbility.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillRequirementResolver.h"
#include "AbilitySystem/Pipeline/NiosCore_GroundTargetValidator.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillTargetResolver.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "Components/AudioComponent.h"
#include "Components/Cooldown/NiosCore_CooldownComponent.h"
#include "Components/Feedback/NiosCore_ActionFeedbackComponent.h"
#include "Components/Feedback/NiosCore_ActionFeedbackTypes.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"

FNiosSkillPipelineContext FNiosCoreSkillPipeline::BuildContext(const UNiosCore_BaseGameplayAbility& Ability)
{
    FNiosSkillPipelineContext Context;
    Context.Skill = Ability.GetSkillData();
    Context.SourceASC = Ability.GetAbilitySystemComponentFromActorInfo();
    Context.SourceNiosASC = Ability.GetNiosASC();
    Context.SourceActor = Ability.GetAvatarActorFromActorInfo();
    Context.SelectedActor = Context.SourceNiosASC ? Context.SourceNiosASC->GetSelectedTarget() : nullptr;
    Context.bHasAuthority = Ability.HasAuthority(&Ability.CachedActivationInfo);
    return Context;
}

static const TCHAR* Nios_PipelineFailureToDebugString(ENiosSkillPipelineFailure Failure)
{
    switch (Failure)
    {
    case ENiosSkillPipelineFailure::None: return TEXT("None");
    case ENiosSkillPipelineFailure::MissingSkillData: return TEXT("MissingSkillData");
    case ENiosSkillPipelineFailure::MissingASC: return TEXT("MissingASC");
    case ENiosSkillPipelineFailure::RequiredTagsMissing: return TEXT("RequiredTagsMissing");
    case ENiosSkillPipelineFailure::BlockedTagsPresent: return TEXT("BlockedTagsPresent");
    case ENiosSkillPipelineFailure::NotEnoughResources: return TEXT("NotEnoughResources");
    case ENiosSkillPipelineFailure::GlobalCooldownActive: return TEXT("GlobalCooldownActive");
    case ENiosSkillPipelineFailure::SkillCooldownActive: return TEXT("SkillCooldownActive");
    case ENiosSkillPipelineFailure::MissingTarget: return TEXT("MissingTarget");
    case ENiosSkillPipelineFailure::InvalidTarget: return TEXT("InvalidTarget");
    case ENiosSkillPipelineFailure::TargetOutOfRange: return TEXT("TargetOutOfRange");
    case ENiosSkillPipelineFailure::SourceDead: return TEXT("SourceDead");
    case ENiosSkillPipelineFailure::TargetDead: return TEXT("TargetDead");
    case ENiosSkillPipelineFailure::CommitFailed: return TEXT("CommitFailed");
    case ENiosSkillPipelineFailure::Cancelled: return TEXT("Cancelled");
    case ENiosSkillPipelineFailure::Interrupted: return TEXT("Interrupted");
    case ENiosSkillPipelineFailure::RequirementFailed: return TEXT("RequirementFailed");
    case ENiosSkillPipelineFailure::RequirementHandledByAbility: return TEXT("RequirementHandledByAbility");
    default: return TEXT("Unknown");
    }
}

static ENiosCoreActionFailReason Nios_MapSkillPipelineFailureToActionFailReason(ENiosSkillPipelineFailure Failure)
{
    switch (Failure)
    {
    case ENiosSkillPipelineFailure::MissingTarget:
        return ENiosCoreActionFailReason::NoTarget;
    case ENiosSkillPipelineFailure::InvalidTarget:
        return ENiosCoreActionFailReason::InvalidTarget;
    case ENiosSkillPipelineFailure::TargetOutOfRange:
        return ENiosCoreActionFailReason::TargetOutOfRange;
    case ENiosSkillPipelineFailure::SourceDead:
        return ENiosCoreActionFailReason::Dead;
    case ENiosSkillPipelineFailure::TargetDead:
        return ENiosCoreActionFailReason::TargetDead;
    case ENiosSkillPipelineFailure::NotEnoughResources:
        return ENiosCoreActionFailReason::NotEnoughMana;
    case ENiosSkillPipelineFailure::GlobalCooldownActive:
    case ENiosSkillPipelineFailure::SkillCooldownActive:
        return ENiosCoreActionFailReason::OnCooldown;
    case ENiosSkillPipelineFailure::RequiredTagsMissing:
    case ENiosSkillPipelineFailure::BlockedTagsPresent:
    case ENiosSkillPipelineFailure::RequirementFailed:
        return ENiosCoreActionFailReason::AbilityBlocked;
    case ENiosSkillPipelineFailure::RequirementHandledByAbility:
        return ENiosCoreActionFailReason::RequirementHandledByAbility;
    case ENiosSkillPipelineFailure::Cancelled:
    case ENiosSkillPipelineFailure::Interrupted:
        return ENiosCoreActionFailReason::Interrupted;
    case ENiosSkillPipelineFailure::CommitFailed:
        return ENiosCoreActionFailReason::ServerRejected;
    default:
        return ENiosCoreActionFailReason::Unknown;
    }
}

static ENiosSkillPipelineFailure Nios_MapTargetFailureNameToPipelineFailure(FName FailureName)
{
    if (FailureName == TEXT("TargetOutOfRange"))
    {
        return ENiosSkillPipelineFailure::TargetOutOfRange;
    }

    if (FailureName == TEXT("TargetDead"))
    {
        return ENiosSkillPipelineFailure::TargetDead;
    }

    if (FailureName == TEXT("NoTargetASC") || FailureName == TEXT("NoTarget"))
    {
        return ENiosSkillPipelineFailure::MissingTarget;
    }

    return ENiosSkillPipelineFailure::InvalidTarget;
}

static AActor* Nios_ResolvePipelineFeedbackTarget(const UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context)
{
    if (AActor* LockedTarget = Ability.GetPrimaryActorFromCachedTargetData())
    {
        return LockedTarget;
    }

    return Context.SelectedActor.Get();
}

static bool Nios_IsASCDeadByHealth(UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        return false;
    }

    const float MaxHealth = ASC->GetNumericAttribute(UNiosCore_AttributeSet::GetMaxHealthAttribute());
    if (MaxHealth <= KINDA_SMALL_NUMBER)
    {
        return false;
    }

    return ASC->GetNumericAttribute(UNiosCore_AttributeSet::GetHealthAttribute()) <= KINDA_SMALL_NUMBER;
}

static bool Nios_ValidatePipelineTargetData(UNiosCore_BaseGameplayAbility& Ability, FNiosSkillPipelineContext& Context, FName& OutFailureName)
{
    OutFailureName = NAME_None;

    const bool bValid = FNiosCoreSkillTargetResolver::ValidateAbilityTarget(
        Ability.GetWorld(),
        Ability.GetAbilitySystemComponentFromActorInfo(),
        Ability.GetAvatarActorFromActorInfo(),
        Context.SelectedActor.Get(),
        Context.Skill,
        Ability.GetCachedTargetData(),
        OutFailureName);

    if (!bValid)
    {
        Context.FailureReason = Nios_MapTargetFailureNameToPipelineFailure(OutFailureName);
    }

    return bValid;
}

static void Nios_PopulateActionFeedbackFromPipeline(
    UNiosCore_BaseGameplayAbility& Ability,
    const FNiosSkillPipelineContext& Context,
    ENiosCoreActionFailReason ActionFailReason,
    FName SkillID,
    FNiosCoreActionFailResult& Result)
{
    Result.ContextId = SkillID;
    if (Result.ContextId.IsNone() && Context.Skill)
    {
        Result.ContextId = Context.Skill->GetFName();
    }

    Result.SourceActor = Context.SourceActor.Get();
    Result.TargetActor = Nios_ResolvePipelineFeedbackTarget(Ability, Context);
    Result.DebugReason = FName(Nios_PipelineFailureToDebugString(Context.FailureReason));

    if (ActionFailReason == ENiosCoreActionFailReason::TargetOutOfRange && Context.Skill && Result.SourceActor && Result.TargetActor)
    {
        Result.RequiredValue = Context.Skill->GetRange();
        Result.CurrentValue = FNiosCoreSkillTargetResolver::CalculateActorRangeDistance(Result.SourceActor.Get(), Result.TargetActor.Get(), Context.Skill);
    }
}

void FNiosCoreSkillPipeline::Activate(
    UNiosCore_BaseGameplayAbility& Ability,
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo)
{
    Ability.CachedSpecHandle = Handle;
    Ability.CachedActivationInfo = ActivationInfo;

    Ability.bWasCancelled = false;
    Ability.bGameplayCueExecuted = false;
    Ability.bCastLoopCueAdded = false;
    Ability.bAbilityCommitted = false;
    Ability.bGrantedActiveTagsAdded = false;
    Ability.CachedTargetData.Clear();
    Ability.PendingDeliveryImpacts = 0;
    Ability.bGameplayExecutionStarted = false;
    Ability.bSkillFeedbackFinalized = false;
    Ability.bCastInterruptWatchActive = false;
    Ability.CastInterruptReason = NAME_None;
    Ability.CastStartLocation = FVector::ZeroVector;

    if (UWorld* World = Ability.GetWorld())
    {
        World->GetTimerManager().ClearTimer(Ability.CastInterruptPollTimerHandle);
    }

    FNiosSkillPipelineContext Context = BuildContext(Ability);

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE ACTIVATE: Ability=%s HandleValid=%d HasAuthority=%d Avatar=%s Skill=%s SelectedTarget=%s"),
        *GetNameSafe(&Ability),
        Handle.IsValid() ? 1 : 0,
        Context.bHasAuthority ? 1 : 0,
        *GetNameSafe(Context.SourceActor.Get()),
        *GetNameSafe(Context.Skill.Get()),
        *GetNameSafe(Context.SelectedActor.Get()));

    if (Ability.ActiveCastLoopAudioComponent)
    {
        Ability.ActiveCastLoopAudioComponent->Stop();
        Ability.ActiveCastLoopAudioComponent = nullptr;
    }

    if (!ValidateActivation(Ability, Context))
    {
        FailActivation(Ability, Context);
        return;
    }

    if (!PrepareTargetData(Ability, Context))
    {
        FailActivation(Ability, Context);
        return;
    }

    FName TargetFailureName = NAME_None;
    if (!Nios_ValidatePipelineTargetData(Ability, Context, TargetFailureName))
    {
        FailActivation(Ability, Context);
        return;
    }

    if (Context.SourceNiosASC)
    {
        Context.SourceNiosASC->NotifySkillActivationAccepted(
            ResolveSkillCooldownID(Ability, Context),
            Context.Skill,
            Nios_ResolvePipelineFeedbackTarget(Ability, Context),
            TEXT("PipelineValidated"));
    }

    Ability.AddGrantedTagsWhileActive();

    if (Context.Skill->IsCastTimeSkill())
    {
        Ability.StartCast();
        return;
    }

    Ability.ExecuteAbility();
}

bool FNiosCoreSkillPipeline::ValidateActivation(UNiosCore_BaseGameplayAbility& Ability, FNiosSkillPipelineContext& Context)
{
    if (!Context.Skill)
    {
        Context.FailureReason = ENiosSkillPipelineFailure::MissingSkillData;
        return false;
    }

    if (!Context.SourceASC)
    {
        Context.FailureReason = ENiosSkillPipelineFailure::MissingASC;
        return false;
    }

    if (Nios_IsASCDeadByHealth(Context.SourceASC))
    {
        Context.FailureReason = ENiosSkillPipelineFailure::SourceDead;
        return false;
    }

    if (Context.SourceNiosASC)
    {
        Context.SourceNiosASC->PreloadSkillAssets(Context.Skill);
    }

    if (!Ability.ValidateSkillTags())
    {
        // Keep this coarse for now. A future UI layer can split required vs blocked tag messages.
        Context.FailureReason = ENiosSkillPipelineFailure::BlockedTagsPresent;
        return false;
    }

    const FNiosSkillRequirementResult RequirementResult = FNiosCoreSkillRequirementResolver::ResolveRequirements(
        Context.Skill,
        Context.SourceASC,
        Context.SourceActor);

    if (RequirementResult.IsHandledByAbility())
    {
        Context.FailureReason = ENiosSkillPipelineFailure::RequirementHandledByAbility;
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE REQUIREMENT HANDLED: Skill=%s Reason=%s"),
            *GetNameSafe(Context.Skill.Get()),
            FNiosCoreSkillRequirementResolver::FailureToString(RequirementResult.Failure));
        return false;
    }

    if (!RequirementResult.IsPassed())
    {
        Context.FailureReason = ENiosSkillPipelineFailure::RequirementFailed;
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE REQUIREMENT FAILED: Skill=%s Reason=%s"),
            *GetNameSafe(Context.Skill.Get()),
            FNiosCoreSkillRequirementResolver::FailureToString(RequirementResult.Failure));
        return false;
    }

    if (!CheckCooldowns(Ability, Context))
    {
        return false;
    }

    if (!Ability.CanPaySkillCosts())
    {
        Context.FailureReason = ENiosSkillPipelineFailure::NotEnoughResources;

        FNiosCoreActionFailResult Result(ENiosCoreActionFailReason::NotEnoughMana);
        Result.ContextId = ResolveSkillCooldownID(Ability, Context);
        Result.SourceActor = Context.SourceActor.Get();
        Result.TargetActor = Context.SelectedActor.Get();
        Result.DebugReason = TEXT("NotEnoughResources");
        UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(Context.SourceActor.Get(), Result);

        return false;
    }

    return true;
}


bool FNiosCoreSkillPipeline::ExtractGroundTargetLocation(const FGameplayAbilityTargetDataHandle& TargetData, FVector& OutLocation)
{
    OutLocation = FVector::ZeroVector;

    for (int32 Index = 0; Index < TargetData.Num(); ++Index)
    {
        const FGameplayAbilityTargetData* Data = TargetData.Get(Index);
        if (!Data)
        {
            continue;
        }

        if (Data->HasHitResult())
        {
            const FHitResult* Hit = Data->GetHitResult();
            if (Hit)
            {
                OutLocation = Hit->ImpactPoint;
                return true;
            }
        }
    }

    return false;
}

bool FNiosCoreSkillPipeline::PrepareTargetData(UNiosCore_BaseGameplayAbility& Ability, FNiosSkillPipelineContext& Context)
{
    if (!Context.Skill)
    {
        Context.FailureReason = ENiosSkillPipelineFailure::MissingSkillData;
        return false;
    }

    const ENiosAbilityTargetType EffectiveTargetType = Context.Skill->TargetType;

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE TARGET PREPARE: Skill=%s TargetType=%d AffectType=%d SelectedTarget=%s"),
        *GetNameSafe(Context.Skill.Get()),
        static_cast<int32>(EffectiveTargetType),
        static_cast<int32>(Context.Skill->AffectType),
        *GetNameSafe(Context.SelectedActor.Get()));

    if (EffectiveTargetType == ENiosAbilityTargetType::Actor)
    {
        if (!Ability.BuildTargetDataFromSelectedTarget())
        {
            Context.FailureReason = ENiosSkillPipelineFailure::MissingTarget;
            return false;
        }
    }
    else if (EffectiveTargetType == ENiosAbilityTargetType::GroundPoint)
    {
        FVector GroundLocation = FVector::ZeroVector;
        if (!Context.SourceNiosASC || !Context.SourceNiosASC->ConsumePendingGroundTargetLocation(GroundLocation))
        {
            Context.FailureReason = ENiosSkillPipelineFailure::MissingTarget;
            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE GROUND TARGET MISSING: Skill=%s ASC=%s"),
                *GetNameSafe(Context.Skill.Get()),
                *GetNameSafe(Context.SourceNiosASC.Get()));
            return false;
        }

        FNiosCoreGroundTargetValidationResult GroundValidationResult;
        if (!UNiosCore_GroundTargetValidator::ValidateGroundTargetLocation(
            &Ability,
            Context.SourceActor.Get(),
            GroundLocation,
            Context.Skill->GroundTargetValidation,
            GroundValidationResult))
        {
            Context.FailureReason = ENiosSkillPipelineFailure::InvalidTarget;

            FNiosCoreActionFailResult Result(
                GroundValidationResult.FailReason == ENiosCoreGroundTargetValidationFailReason::OutOfRange
                    ? ENiosCoreActionFailReason::TargetOutOfRange
                    : ENiosCoreActionFailReason::InvalidTarget);
            Result.ContextId = Context.Skill->GetFName();
            Result.SourceActor = Context.SourceActor.Get();
            Result.TargetActor = Context.SelectedActor.Get();
            Result.CurrentValue = GroundValidationResult.DistanceFromSource;
            Result.RequiredValue = Context.Skill->GroundTargetValidation.MaxRange;
            Result.DebugReason = FName(*FString::Printf(TEXT("GroundTargetInvalid_%d"), static_cast<int32>(GroundValidationResult.FailReason)));
            UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(Context.SourceActor.Get(), Result);

            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE GROUND TARGET INVALID: Skill=%s Requested=%s Reason=%d Distance=%.2f MaxRange=%.2f"),
                *GetNameSafe(Context.Skill.Get()),
                *GroundLocation.ToCompactString(),
                static_cast<int32>(GroundValidationResult.FailReason),
                GroundValidationResult.DistanceFromSource,
                Context.Skill->GroundTargetValidation.MaxRange);
            return false;
        }

        GroundLocation = GroundValidationResult.ValidatedLocation;
        Context.bHasTargetLocation = true;
        Context.TargetLocation = GroundLocation;

        Ability.CachedTargetData.Clear();

        FHitResult Hit;
        Hit.Location = GroundLocation;
        Hit.ImpactPoint = GroundLocation;
        Hit.TraceEnd = GroundLocation;
        Hit.bBlockingHit = true;

        FGameplayAbilityTargetData_SingleTargetHit* HitData = new FGameplayAbilityTargetData_SingleTargetHit(Hit);
        Ability.CachedTargetData.Add(HitData);

        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE GROUND TARGET PREPARED: Skill=%s Requested=%s Location=%s Clamped=%d Distance=%.2f TargetDataNum=%d"),
            *GetNameSafe(Context.Skill.Get()),
            *GroundValidationResult.RequestedLocation.ToCompactString(),
            *GroundLocation.ToCompactString(),
            GroundValidationResult.bWasClamped ? 1 : 0,
            GroundValidationResult.DistanceFromSource,
            Ability.CachedTargetData.Num());
    }
    else
    {
        Ability.CachedTargetData.Clear();
    }

    return true;
}

void FNiosCoreSkillPipeline::OnCastFinished(UNiosCore_BaseGameplayAbility& Ability)
{
    if (Ability.bWasCancelled)
    {
        return;
    }

    Ability.StopCastLoopVisuals();
    Ability.RemoveGrantedTagsWhileActive();

    if (Ability.HasAuthority(&Ability.CachedActivationInfo))
    {
        if (UNiosCore_AbilitySystemComponent* NiosASC = Ability.GetNiosASC())
        {
            NiosASC->EndCastState();
        }

        Ability.RemoveCastLoopGameplayCue();
    }

    FNiosSkillPipelineContext Context = BuildContext(Ability);
    FName TargetFailureName = NAME_None;
    if (!Nios_ValidatePipelineTargetData(Ability, Context, TargetFailureName))
    {
        InterruptActivation(Ability, Context);
        return;
    }

    Ability.ExecuteAbility();
}

void FNiosCoreSkillPipeline::Execute(UNiosCore_BaseGameplayAbility& Ability)
{
    if (Ability.bGameplayExecutionStarted)
    {
        return;
    }

    Ability.bGameplayExecutionStarted = true;

    FNiosSkillPipelineContext Context = BuildContext(Ability);

    if (Context.Skill && Context.Skill->TargetType == ENiosAbilityTargetType::GroundPoint)
    {
        FVector GroundLocation = FVector::ZeroVector;
        if (ExtractGroundTargetLocation(Ability.CachedTargetData, GroundLocation))
        {
            Context.bHasTargetLocation = true;
            Context.TargetLocation = GroundLocation;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE EXECUTE: Skill=%s HasAuthority=%d Avatar=%s SelectedTarget=%s HasGround=%d Ground=%s"),
        *GetNameSafe(Context.Skill.Get()),
        Context.bHasAuthority ? 1 : 0,
        *GetNameSafe(Context.SourceActor.Get()),
        *GetNameSafe(Context.SelectedActor.Get()),
        Context.bHasTargetLocation ? 1 : 0,
        *Context.TargetLocation.ToCompactString());

    if (!Ability.CommitAbilityOnce())
    {
        Context.FailureReason = ENiosSkillPipelineFailure::CommitFailed;
        FailExecution(Ability, Context);
        return;
    }

    if (Context.bHasAuthority)
    {
        StartCooldownsAfterSuccessfulExecute(Ability, Context);

        if (Context.SourceNiosASC)
        {
            Context.SourceNiosASC->NotifySkillActivationExecuting(
                ResolveSkillCooldownID(Ability, Context),
                Context.Skill,
                Nios_ResolvePipelineFeedbackTarget(Ability, Context),
                TEXT("PipelineExecuting"));
        }
    }

    Ability.PlayExecuteVisuals();

    if (Context.bHasAuthority)
    {
        ExecuteGroundPointFallbackVisualIfNeeded(Ability, Context);
        Ability.ApplySkillGameplay();
    }

    Ability.FinishAbilityAfterGameplay();
}


void FNiosCoreSkillPipeline::ExecuteGroundPointFallbackVisualIfNeeded(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context)
{
    if (!Context.Skill || Context.Skill->TargetType != ENiosAbilityTargetType::GroundPoint)
    {
        return;
    }

    if (!Context.bHasTargetLocation)
    {
        return;
    }

    // GroundPoint skills with deliveries will play their impact cue through Delivery.
    // This fallback is only for debug/simple ground skills that have no delivery yet:
    // confirm point -> execute -> visible/logged impact at that point.
    if (Context.Skill->Deliveries.Num() > 0)
    {
        return;
    }

    const FGameplayTag ImpactCue = Ability.ResolveImpactCueTag();
    if (ImpactCue.IsValid())
    {
        Ability.ExecuteDeliveryGameplayCue(ImpactCue, Context.TargetLocation);
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE GROUND EXECUTE FINISHED: Skill=%s Location=%s Effects=%d Deliveries=%d"),
        *GetNameSafe(Context.Skill.Get()),
        *Context.TargetLocation.ToCompactString(),
        Context.Skill->SkillEffects.Num(),
        Context.Skill->Deliveries.Num());
}

void FNiosCoreSkillPipeline::FinishAfterGameplay(UNiosCore_BaseGameplayAbility& Ability)
{
    if (Ability.PendingDeliveryImpacts > 0)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE WAITING FOR DELIVERIES: Skill=%s Pending=%d"),
            *GetNameSafe(Ability.GetSkillData()), Ability.PendingDeliveryImpacts);
        return;
    }

    FNiosSkillPipelineContext Context = BuildContext(Ability);
    if (!Ability.bSkillFeedbackFinalized && Context.SourceNiosASC)
    {
        Ability.bSkillFeedbackFinalized = true;
        Context.SourceNiosASC->NotifySkillActivationExecuted(
            ResolveSkillCooldownID(Ability, Context),
            Context.Skill,
            Nios_ResolvePipelineFeedbackTarget(Ability, Context),
            TEXT("PipelineExecuted"));
    }

    Ability.EndAbilityInternal();
}

void FNiosCoreSkillPipeline::HandleTargetDataReady(UNiosCore_BaseGameplayAbility& Ability, const FGameplayAbilityTargetDataHandle& Data)
{
    Ability.CachedTargetData = Data;

    FNiosSkillPipelineContext Context = BuildContext(Ability);
    if (!Context.Skill)
    {
        Context.FailureReason = ENiosSkillPipelineFailure::MissingSkillData;
        FailActivation(Ability, Context);
        return;
    }

    FName TargetFailureName = NAME_None;
    if (!Nios_ValidatePipelineTargetData(Ability, Context, TargetFailureName))
    {
        FailActivation(Ability, Context);
        return;
    }

    // Same rule as normal activation: costs are paid in Execute(), not at cast start.
    if (Context.Skill->IsCastTimeSkill())
    {
        Ability.StartCast();
        return;
    }

    Ability.ExecuteAbility();
}


UNiosCore_CooldownComponent* FNiosCoreSkillPipeline::ResolveCooldownComponent(const FNiosSkillPipelineContext& Context)
{
    auto TryActor = [](AActor* Actor) -> UNiosCore_CooldownComponent*
    {
        if (!Actor)
        {
            return nullptr;
        }

        if (Actor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
        {
            if (UNiosCore_CooldownComponent* FromInterface = INiosGASActorInterface::Execute_GetCooldownComponent(Actor))
            {
                return FromInterface;
            }
        }

        return Actor->FindComponentByClass<UNiosCore_CooldownComponent>();
    };

    // Core gameplay components are expected to live on PlayerState in the MMO architecture.
    // Resolve through every reliable ownership path before falling back to the avatar shell.
    if (Context.SourceASC)
    {
        if (UNiosCore_CooldownComponent* FromOwner = TryActor(Context.SourceASC->GetOwnerActor()))
        {
            return FromOwner;
        }
    }

    if (AActor* SourceActor = Context.SourceActor.Get())
    {
        if (UNiosCore_CooldownComponent* FromSource = TryActor(SourceActor))
        {
            return FromSource;
        }

        if (const APawn* SourcePawn = Cast<APawn>(SourceActor))
        {
            if (UNiosCore_CooldownComponent* FromPS = TryActor(SourcePawn->GetPlayerState()))
            {
                return FromPS;
            }

            if (AController* Controller = SourcePawn->GetController())
            {
                if (UNiosCore_CooldownComponent* FromControllerPS = TryActor(Controller->PlayerState))
                {
                    return FromControllerPS;
                }
            }
        }

        if (AActor* Owner = SourceActor->GetOwner())
        {
            if (UNiosCore_CooldownComponent* FromSourceOwner = TryActor(Owner))
            {
                return FromSourceOwner;
            }

            if (const APawn* OwnerPawn = Cast<APawn>(Owner))
            {
                if (UNiosCore_CooldownComponent* FromOwnerPawnPS = TryActor(OwnerPawn->GetPlayerState()))
                {
                    return FromOwnerPawnPS;
                }
            }
        }
    }

    if (Context.SourceASC)
    {
        if (UNiosCore_CooldownComponent* FromAvatar = TryActor(Context.SourceASC->GetAvatarActor()))
        {
            return FromAvatar;
        }
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> COOLDOWN COMPONENT MISSING: Source=%s ASC=%s Owner=%s Avatar=%s"),
        *GetNameSafe(Context.SourceActor.Get()),
        *GetNameSafe(Context.SourceASC.Get()),
        *GetNameSafe(Context.SourceASC ? Context.SourceASC->GetOwnerActor() : nullptr),
        *GetNameSafe(Context.SourceASC ? Context.SourceASC->GetAvatarActor() : nullptr));

    return nullptr;
}

FName FNiosCoreSkillPipeline::ResolveSkillCooldownID(const UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context)
{
    if (Context.SourceNiosASC)
    {
        const FName SkillID = Context.SourceNiosASC->GetSkillIDBySpecHandle(Ability.CachedSpecHandle);
        if (SkillID != NAME_None)
        {
            return SkillID;
        }
    }

    return Context.Skill ? Context.Skill->GetFName() : NAME_None;
}

bool FNiosCoreSkillPipeline::CheckCooldowns(UNiosCore_BaseGameplayAbility& Ability, FNiosSkillPipelineContext& Context)
{
    UNiosCore_CooldownComponent* Cooldowns = ResolveCooldownComponent(Context);
    if (!Context.Skill)
    {
        return true;
    }

    if (!Cooldowns)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL COOLDOWN CHECK SKIPPED: missing cooldown component. Skill=%s"),
            *GetNameSafe(Context.Skill.Get()));
        return true;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL COOLDOWN CHECK: Skill=%s GlobalUse=%d GlobalRemaining=%.2f SkillID=%s SkillRemaining=%.2f"),
        *GetNameSafe(Context.Skill.Get()),
        Context.Skill->bUseGlobalCooldown ? 1 : 0,
        Cooldowns->GetGlobalSkillCooldownRemaining(),
        *ResolveSkillCooldownID(Ability, Context).ToString(),
        Cooldowns->GetSkillCooldownRemaining(ResolveSkillCooldownID(Ability, Context)));

    if (Context.Skill->bUseGlobalCooldown && !Cooldowns->IsGlobalSkillCooldownReady())
    {
        Context.FailureReason = ENiosSkillPipelineFailure::GlobalCooldownActive;

        FNiosCoreActionFailResult Result(ENiosCoreActionFailReason::OnCooldown);
        Result.ContextId = ResolveSkillCooldownID(Ability, Context);
        Result.CurrentValue = Cooldowns->GetGlobalSkillCooldownRemaining();
        Result.RequiredValue = Cooldowns->GetCooldownDuration(ENiosCoreCooldownDomain::GlobalSkill, NAME_None);
        Result.SourceActor = Context.SourceActor.Get();
        Result.TargetActor = Context.SelectedActor.Get();
        Result.DebugReason = TEXT("GlobalSkillCooldownActive");
        UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(Context.SourceActor.Get(), Result);

        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL COOLDOWN BLOCKED: Skill=%s Reason=Global Remaining=%.2f"),
            *GetNameSafe(Context.Skill.Get()),
            Cooldowns->GetGlobalSkillCooldownRemaining());
        return false;
    }

    const FName CooldownID = ResolveSkillCooldownID(Ability, Context);
    if (CooldownID != NAME_None && !Cooldowns->IsSkillCooldownReady(CooldownID))
    {
        Context.FailureReason = ENiosSkillPipelineFailure::SkillCooldownActive;

        FNiosCoreActionFailResult Result(ENiosCoreActionFailReason::OnCooldown);
        Result.ContextId = CooldownID;
        Result.CurrentValue = Cooldowns->GetSkillCooldownRemaining(CooldownID);
        Result.RequiredValue = Cooldowns->GetSkillCooldownDuration(CooldownID);
        Result.SourceActor = Context.SourceActor.Get();
        Result.TargetActor = Context.SelectedActor.Get();
        Result.DebugReason = TEXT("SkillCooldownActive");
        UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(Context.SourceActor.Get(), Result);

        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL COOLDOWN BLOCKED: Skill=%s ID=%s Reason=Local Remaining=%.2f"),
            *GetNameSafe(Context.Skill.Get()),
            *CooldownID.ToString(),
            Cooldowns->GetSkillCooldownRemaining(CooldownID));
        return false;
    }

    return true;
}

void FNiosCoreSkillPipeline::StartCooldownsAfterSuccessfulExecute(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context)
{
    UNiosCore_CooldownComponent* Cooldowns = ResolveCooldownComponent(Context);
    if (!Context.Skill)
    {
        return;
    }

    if (!Cooldowns)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL COOLDOWN START SKIPPED: missing cooldown component. Skill=%s"),
            *GetNameSafe(Context.Skill.Get()));
        return;
    }

    const FName CooldownID = ResolveSkillCooldownID(Ability, Context);

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL COOLDOWN START TRY: Skill=%s ID=%s UseGlobal=%d GlobalOverride=%.2f Local=%.2f"),
        *GetNameSafe(Context.Skill.Get()),
        *CooldownID.ToString(),
        Context.Skill->bUseGlobalCooldown ? 1 : 0,
        Context.Skill->GlobalCooldownOverride,
        Context.Skill->GetLocalCooldownDuration());

    if (Context.Skill->bUseGlobalCooldown)
    {
        const float Override = Context.Skill->GlobalCooldownOverride;
        if (Override != 0.f)
        {
            Cooldowns->StartGlobalSkillCooldown(Override);
        }
    }

    const float LocalCooldown = Context.Skill->GetLocalCooldownDuration();
    if (CooldownID != NAME_None && LocalCooldown > KINDA_SMALL_NUMBER)
    {
        Cooldowns->StartSkillCooldown(CooldownID, LocalCooldown);
    }
}

void FNiosCoreSkillPipeline::FailActivation(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context, bool bReplicateEnd)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE REJECTED: Skill=%s Reason=%s SelectedTarget=%s"),
        *GetNameSafe(Context.Skill.Get()),
        FailureToString(Context.FailureReason),
        *GetNameSafe(Context.SelectedActor.Get()));

    const ENiosCoreActionFailReason ActionFailReason = Nios_MapSkillPipelineFailureToActionFailReason(Context.FailureReason);
    const FName SkillID = ResolveSkillCooldownID(Ability, Context);

    if (Context.SourceNiosASC)
    {
        Context.SourceNiosASC->NotifySkillActivationRejected(
            SkillID,
            Context.Skill,
            ActionFailReason,
            FName(FailureToString(Context.FailureReason)));
    }

    const bool bFailureAlreadyEmittedSpecificFeedback =
        Context.FailureReason == ENiosSkillPipelineFailure::GlobalCooldownActive
        || Context.FailureReason == ENiosSkillPipelineFailure::SkillCooldownActive
        || Context.FailureReason == ENiosSkillPipelineFailure::NotEnoughResources;

    if (!bFailureAlreadyEmittedSpecificFeedback && ActionFailReason != ENiosCoreActionFailReason::None)
    {
        FNiosCoreActionFailResult Result(ActionFailReason);
        Nios_PopulateActionFeedbackFromPipeline(Ability, Context, ActionFailReason, SkillID, Result);
        UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(Context.SourceActor.Get(), Result);
    }

    Ability.bSkillFeedbackFinalized = true;

    Ability.EndAbility(
        Ability.CachedSpecHandle,
        Ability.GetCurrentActorInfo(),
        Ability.CachedActivationInfo,
        bReplicateEnd,
        true);
}

void FNiosCoreSkillPipeline::FailExecution(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context, bool bReplicateEnd)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE EXECUTION FAILED: Skill=%s Reason=%s SelectedTarget=%s"),
        *GetNameSafe(Context.Skill.Get()),
        FailureToString(Context.FailureReason),
        *GetNameSafe(Context.SelectedActor.Get()));

    const ENiosCoreActionFailReason ActionFailReason = Nios_MapSkillPipelineFailureToActionFailReason(Context.FailureReason);
    const FName SkillID = ResolveSkillCooldownID(Ability, Context);
    AActor* FeedbackTarget = Nios_ResolvePipelineFeedbackTarget(Ability, Context);

    if (Context.SourceNiosASC)
    {
        Context.SourceNiosASC->NotifySkillActivationFailed(
            SkillID,
            Context.Skill,
            ActionFailReason,
            FeedbackTarget,
            FName(FailureToString(Context.FailureReason)));
    }

    if (ActionFailReason != ENiosCoreActionFailReason::None)
    {
        FNiosCoreActionFailResult Result(ActionFailReason);
        Nios_PopulateActionFeedbackFromPipeline(Ability, Context, ActionFailReason, SkillID, Result);
        UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(Context.SourceActor.Get(), Result);
    }

    Ability.bSkillFeedbackFinalized = true;

    Ability.EndAbility(
        Ability.CachedSpecHandle,
        Ability.GetCurrentActorInfo(),
        Ability.CachedActivationInfo,
        bReplicateEnd,
        true);
}

void FNiosCoreSkillPipeline::InterruptActivation(UNiosCore_BaseGameplayAbility& Ability, const FNiosSkillPipelineContext& Context, bool bReplicateEnd)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL PIPELINE INTERRUPTED: Skill=%s Reason=%s SelectedTarget=%s"),
        *GetNameSafe(Context.Skill.Get()),
        FailureToString(Context.FailureReason),
        *GetNameSafe(Context.SelectedActor.Get()));

    const ENiosCoreActionFailReason ActionFailReason = Nios_MapSkillPipelineFailureToActionFailReason(Context.FailureReason);
    const FName SkillID = ResolveSkillCooldownID(Ability, Context);
    AActor* FeedbackTarget = Nios_ResolvePipelineFeedbackTarget(Ability, Context);

    if (Context.SourceNiosASC)
    {
        float RequiredValue = 0.f;
        float CurrentValue = 0.f;
        if (ActionFailReason == ENiosCoreActionFailReason::TargetOutOfRange && Context.Skill && Context.SourceActor && FeedbackTarget)
        {
            RequiredValue = Context.Skill->GetRange();
            CurrentValue = FNiosCoreSkillTargetResolver::CalculateActorRangeDistance(Context.SourceActor.Get(), FeedbackTarget, Context.Skill);
        }

        Context.SourceNiosASC->NotifySkillActivationInterrupted(
            SkillID,
            Context.Skill,
            ActionFailReason,
            FeedbackTarget,
            FName(FailureToString(Context.FailureReason)),
            RequiredValue,
            CurrentValue);
    }

    if (ActionFailReason != ENiosCoreActionFailReason::None)
    {
        FNiosCoreActionFailResult Result(ActionFailReason);
        Nios_PopulateActionFeedbackFromPipeline(Ability, Context, ActionFailReason, SkillID, Result);
        UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(Context.SourceActor.Get(), Result);
    }

    Ability.bSkillFeedbackFinalized = true;

    Ability.EndAbility(
        Ability.CachedSpecHandle,
        Ability.GetCurrentActorInfo(),
        Ability.CachedActivationInfo,
        bReplicateEnd,
        true);
}

const TCHAR* FNiosCoreSkillPipeline::FailureToString(ENiosSkillPipelineFailure Failure)
{
    switch (Failure)
    {
    case ENiosSkillPipelineFailure::None: return TEXT("None");
    case ENiosSkillPipelineFailure::MissingSkillData: return TEXT("MissingSkillData");
    case ENiosSkillPipelineFailure::MissingASC: return TEXT("MissingASC");
    case ENiosSkillPipelineFailure::RequiredTagsMissing: return TEXT("RequiredTagsMissing");
    case ENiosSkillPipelineFailure::BlockedTagsPresent: return TEXT("BlockedTagsPresent");
    case ENiosSkillPipelineFailure::NotEnoughResources: return TEXT("NotEnoughResources");
    case ENiosSkillPipelineFailure::GlobalCooldownActive: return TEXT("GlobalCooldownActive");
    case ENiosSkillPipelineFailure::SkillCooldownActive: return TEXT("SkillCooldownActive");
    case ENiosSkillPipelineFailure::MissingTarget: return TEXT("MissingTarget");
    case ENiosSkillPipelineFailure::InvalidTarget: return TEXT("InvalidTarget");
    case ENiosSkillPipelineFailure::TargetOutOfRange: return TEXT("TargetOutOfRange");
    case ENiosSkillPipelineFailure::SourceDead: return TEXT("SourceDead");
    case ENiosSkillPipelineFailure::TargetDead: return TEXT("TargetDead");
    case ENiosSkillPipelineFailure::CommitFailed: return TEXT("CommitFailed");
    case ENiosSkillPipelineFailure::Cancelled: return TEXT("Cancelled");
    case ENiosSkillPipelineFailure::Interrupted: return TEXT("Interrupted");
    case ENiosSkillPipelineFailure::RequirementFailed: return TEXT("RequirementFailed");
    case ENiosSkillPipelineFailure::RequirementHandledByAbility: return TEXT("RequirementHandledByAbility");
    default: return TEXT("Unknown");
    }
}

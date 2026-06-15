#include "AbilitySystem/Pipeline/NiosCore_SkillVisualDispatcher.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "NiagaraSystem.h"

#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "AbilitySystem/Skills/NiosCore_SkillEffectTypes.h"
#include "Character/NiosCore_Character.h"
#include "AbilitySystem/Actors/NiosCore_GASAICharacter.h"

static FNiosLoadedSkillAssets* NiosGetLoadedSkillAssets(UAbilitySystemComponent* SourceASC, UNiosCore_SkillDataAsset* Skill)
{
    UNiosCore_AbilitySystemComponent* NiosASC = Cast<UNiosCore_AbilitySystemComponent>(SourceASC);
    if (!NiosASC || !Skill)
    {
        return nullptr;
    }

    FNiosLoadedSkillAssets* Assets = NiosASC->GetLoadedSkillAssets(Skill);
    if (!Assets)
    {
        NiosASC->PreloadSkillAssets(Skill);
        Assets = NiosASC->GetLoadedSkillAssets(Skill);
    }

    return Assets;
}

void FNiosCoreSkillVisualDispatcher::PlayCastLoopVisuals(const FNiosSkillVisualContext& Context, bool bStart, TObjectPtr<UAudioComponent>& ActiveCastLoopAudioComponent)
{
    UNiosCore_SkillDataAsset* Skill = Context.Skill;
    AActor* SourceActor = Context.SourceActor;
    if (!Skill || !SourceActor)
    {
        return;
    }

    if (Context.bHasAuthority)
    {
        if (ANiosCore_Character* NiosCharacter = Cast<ANiosCore_Character>(SourceActor))
        {
            NiosCharacter->Multicast_PlaySkillCastLoopVisuals(Skill, bStart);
            return;
        }

        if (ANiosCore_GASAICharacter* AICharacter = Cast<ANiosCore_GASAICharacter>(SourceActor))
        {
            AICharacter->Multicast_PlaySkillCastLoopVisuals(Skill, bStart);
            return;
        }
    }

    ACharacter* Character = Cast<ACharacter>(SourceActor);
    if (!Character || !Character->IsLocallyControlled())
    {
        return;
    }

    FNiosLoadedSkillAssets* Assets = NiosGetLoadedSkillAssets(Context.SourceASC, Skill);
    UAnimMontage* Montage = Assets ? Assets->CastLoopMontage : nullptr;
    USoundBase* Sound = Assets ? Assets->CastLoopSound : nullptr;

    if (UAnimInstance* AnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
    {
        if (Montage)
        {
            if (bStart)
            {
                const float PlayedLength = AnimInstance->Montage_Play(Montage);
                UE_LOG(LogTemp, Warning, TEXT(">>> LOCAL SKILL VISUAL CAST START: Montage=%s PlayedLength=%.3f Character=%s Skill=%s"),
                    *GetNameSafe(Montage), PlayedLength, *GetNameSafe(Character), *GetNameSafe(Skill));
            }
            else
            {
                AnimInstance->Montage_Stop(0.15f, Montage);
                UE_LOG(LogTemp, Warning, TEXT(">>> LOCAL SKILL VISUAL CAST STOP: Montage=%s Character=%s Skill=%s"),
                    *GetNameSafe(Montage), *GetNameSafe(Character), *GetNameSafe(Skill));
            }
        }
    }

    if (bStart && Sound)
    {
        ActiveCastLoopAudioComponent = UGameplayStatics::SpawnSoundAttached(Sound, Character->GetRootComponent());
    }
    else if (!bStart && ActiveCastLoopAudioComponent)
    {
        ActiveCastLoopAudioComponent->Stop();
        ActiveCastLoopAudioComponent = nullptr;
    }
}

void FNiosCoreSkillVisualDispatcher::PlayExecuteVisuals(const FNiosSkillVisualContext& Context)
{
    UNiosCore_SkillDataAsset* Skill = Context.Skill;
    AActor* SourceActor = Context.SourceActor;
    if (!Skill || !SourceActor)
    {
        return;
    }

    if (Context.bHasAuthority)
    {
        if (ANiosCore_Character* NiosCharacter = Cast<ANiosCore_Character>(SourceActor))
        {
            NiosCharacter->Multicast_PlaySkillExecuteVisuals(Skill);
            return;
        }

        if (ANiosCore_GASAICharacter* AICharacter = Cast<ANiosCore_GASAICharacter>(SourceActor))
        {
            AICharacter->Multicast_PlaySkillExecuteVisuals(Skill);
            return;
        }
    }

    ACharacter* Character = Cast<ACharacter>(SourceActor);
    if (!Character || !Character->IsLocallyControlled())
    {
        return;
    }

    FNiosLoadedSkillAssets* Assets = NiosGetLoadedSkillAssets(Context.SourceASC, Skill);
    UAnimMontage* Montage = Assets ? Assets->ExecuteMontage : nullptr;
    USoundBase* Sound = Assets ? Assets->ExecuteSound : nullptr;

    if (Montage)
    {
        if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
            {
                const float PlayedLength = AnimInstance->Montage_Play(Montage);
                UE_LOG(LogTemp, Warning, TEXT(">>> LOCAL SKILL VISUAL EXECUTE: Montage=%s PlayedLength=%.3f Character=%s Skill=%s"),
                    *GetNameSafe(Montage), PlayedLength, *GetNameSafe(Character), *GetNameSafe(Skill));
            }
        }
    }

    if (Sound)
    {
        UGameplayStatics::SpawnSoundAttached(Sound, Character->GetRootComponent());
    }
}

FGameplayTag FNiosCoreSkillVisualDispatcher::ResolveDefaultCueTag(const UNiosCore_SkillDataAsset* Skill, FName CueEvent)
{
    FString AffectName = TEXT("Neutral");

    if (Skill)
    {
        switch (Skill->AffectType)
        {
        case ENiosAbilityAffectType::Harm:
            AffectName = TEXT("Harm");
            break;
        case ENiosAbilityAffectType::Help:
            AffectName = TEXT("Help");
            break;
        case ENiosAbilityAffectType::Neutral:
        default:
            AffectName = TEXT("Neutral");
            break;
        }
    }

    const FString EventName = CueEvent.IsNone() ? TEXT("Execute") : CueEvent.ToString();
    return FGameplayTag::RequestGameplayTag(
        FName(*FString::Printf(TEXT("GameplayCue.Skill.%s.%s"), *AffectName, *EventName)),
        false
    );
}

FGameplayTag FNiosCoreSkillVisualDispatcher::ResolveCastLoopCueTag(const UNiosCore_SkillDataAsset* Skill)
{
    if (Skill && !Skill->CueConfig.bUseDefaultCueTags && Skill->CueConfig.CastLoopCueTag.IsValid())
    {
        return Skill->CueConfig.CastLoopCueTag;
    }
    return ResolveDefaultCueTag(Skill, TEXT("CastLoop"));
}

FGameplayTag FNiosCoreSkillVisualDispatcher::ResolveExecuteCueTag(const UNiosCore_SkillDataAsset* Skill)
{
    if (Skill && !Skill->CueConfig.bUseDefaultCueTags && Skill->CueConfig.ExecuteCueTag.IsValid())
    {
        return Skill->CueConfig.ExecuteCueTag;
    }
    return ResolveDefaultCueTag(Skill, TEXT("Execute"));
}

FGameplayTag FNiosCoreSkillVisualDispatcher::ResolveImpactCueTag(const UNiosCore_SkillDataAsset* Skill)
{
    if (Skill && !Skill->CueConfig.bUseDefaultCueTags && Skill->CueConfig.ImpactCueTag.IsValid())
    {
        return Skill->CueConfig.ImpactCueTag;
    }
    return ResolveDefaultCueTag(Skill, TEXT("Impact"));
}

FGameplayTag FNiosCoreSkillVisualDispatcher::ResolveFailCueTag(const UNiosCore_SkillDataAsset* Skill)
{
    if (Skill && !Skill->CueConfig.bUseDefaultCueTags && Skill->CueConfig.FailCueTag.IsValid())
    {
        return Skill->CueConfig.FailCueTag;
    }
    return ResolveDefaultCueTag(Skill, TEXT("Fail"));
}

void FNiosCoreSkillVisualDispatcher::AddCastLoopGameplayCue(const FNiosSkillVisualContext& Context, bool& bCastLoopCueAdded)
{
    if (bCastLoopCueAdded || !Context.SourceASC || !Context.Skill)
    {
        return;
    }

    const FGameplayTag CastLoopCueTag = ResolveCastLoopCueTag(Context.Skill);
    if (!CastLoopCueTag.IsValid())
    {
        return;
    }

    bCastLoopCueAdded = true;

    FGameplayCueParameters Params;
    Params.Instigator = Context.SourceActor;
    Params.EffectCauser = Context.SourceActor;
    Params.SourceObject = Context.Skill;

    Context.SourceASC->AddGameplayCue(CastLoopCueTag, Params);
}

void FNiosCoreSkillVisualDispatcher::RemoveCastLoopGameplayCue(const FNiosSkillVisualContext& Context, bool& bCastLoopCueAdded)
{
    if (!bCastLoopCueAdded || !Context.SourceASC || !Context.Skill)
    {
        return;
    }

    bCastLoopCueAdded = false;

    const FGameplayTag CastLoopCueTag = ResolveCastLoopCueTag(Context.Skill);
    if (CastLoopCueTag.IsValid())
    {
        Context.SourceASC->RemoveGameplayCue(CastLoopCueTag);
    }
}

void FNiosCoreSkillVisualDispatcher::ExecuteSkillGameplayCue(const FNiosSkillVisualContext& Context, bool& bGameplayCueExecuted)
{
    if (bGameplayCueExecuted || !Context.SourceASC || !Context.Skill)
    {
        return;
    }

    const FGameplayTag SkillExecuteCueTag = ResolveExecuteCueTag(Context.Skill);
    if (!SkillExecuteCueTag.IsValid())
    {
        return;
    }

    bGameplayCueExecuted = true;

    FGameplayCueParameters Params;
    Params.Instigator = Context.SourceActor;
    Params.EffectCauser = Context.SourceActor;
    Params.SourceObject = Context.Skill;

    Context.SourceASC->ExecuteGameplayCue(SkillExecuteCueTag, Params);
}


void FNiosCoreSkillVisualDispatcher::DispatchDeliveryNiagara(
    const FNiosSkillVisualContext& Context,
    const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem,
    const FVector& Location,
    const FRotator& Rotation,
    FName DebugName)
{
    if (NiagaraSystem.IsNull() || !Context.SourceActor)
    {
        return;
    }

    if (Context.bHasAuthority)
    {
        if (ANiosCore_Character* NiosCharacter = Cast<ANiosCore_Character>(Context.SourceActor))
        {
            NiosCharacter->Multicast_PlayDeliveryNiagara(NiagaraSystem, Location, Rotation, DebugName);
        }
        return;
    }

    // Non-authority direct path is intentionally left minimal for future prediction.
    // Regular server-authoritative gameplay uses the multicast above.
}

void FNiosCoreSkillVisualDispatcher::ExecuteDeliveryGameplayCue(const FNiosSkillVisualContext& Context, FGameplayTag CueTag, const FVector& CueLocation)
{
    if (!CueTag.IsValid() || !Context.SourceASC || !Context.Skill)
    {
        return;
    }

    FGameplayCueParameters Params;
    Params.Instigator = Context.SourceActor;
    Params.EffectCauser = Context.SourceActor;
    Params.SourceObject = Context.Skill;
    Params.Location = CueLocation;

    Context.SourceASC->ExecuteGameplayCue(CueTag, Params);
}

void FNiosCoreSkillVisualDispatcher::ExecuteImpactGameplayCue(
    const FNiosSkillVisualContext& Context,
    UAbilitySystemComponent* TargetASC,
    const FNiosSkillEffectDefinition& Effect,
    float RawMagnitude)
{
    if (!TargetASC || !Context.Skill)
    {
        return;
    }

    const FGameplayTag ImpactCueTag = ResolveImpactCueTag(Context.Skill);
    if (!ImpactCueTag.IsValid())
    {
        return;
    }

    FGameplayCueParameters Params;
    Params.Instigator = Context.SourceActor;
    Params.EffectCauser = Context.SourceActor;
    Params.SourceObject = Context.Skill;
    Params.RawMagnitude = RawMagnitude;

    if (AActor* TargetActor = TargetASC->GetAvatarActor())
    {
        Params.Location = TargetActor->GetActorLocation();
    }

    TargetASC->ExecuteGameplayCue(ImpactCueTag, Params);
}

#include "AbilitySystem/Cues/NiosGCN_SkillCastLoop.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"

ANiosGCN_SkillCastLoop::ANiosGCN_SkillCastLoop()
{
    //GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Skill.CastLoop"));
    bAutoDestroyOnRemove = true;
}

bool ANiosGCN_SkillCastLoop::OnActive_Implementation(
    AActor* MyTarget,
    const FGameplayCueParameters& Parameters)
{
    if (!MyTarget)
        return false;

    ACharacter* Character = Cast<ACharacter>(MyTarget);
    if (!Character)
        return false;

    if (Character->IsLocallyControlled())
    {
        return false;
    }

    const UNiosCore_SkillDataAsset* Skill =
        Cast<UNiosCore_SkillDataAsset>(Parameters.SourceObject.Get());

    if (!Skill)
        return false;

    UAbilitySystemComponent* ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(MyTarget);

    UNiosCore_AbilitySystemComponent* NiosASC =
        Cast<UNiosCore_AbilitySystemComponent>(ASC);

    UAnimMontage* Montage = nullptr;
    USoundBase* Sound = nullptr;

    if (NiosASC)
    {
        FNiosLoadedSkillAssets* Assets =
            NiosASC->GetLoadedSkillAssets(const_cast<UNiosCore_SkillDataAsset*>(Skill));

        if (!Assets)
        {
            NiosASC->PreloadSkillAssets(const_cast<UNiosCore_SkillDataAsset*>(Skill));
            Assets = NiosASC->GetLoadedSkillAssets(const_cast<UNiosCore_SkillDataAsset*>(Skill));
        }

        if (Assets)
        {
            Montage = Assets->CastLoopMontage;
            Sound = Assets->CastLoopSound;
        }
    }

    // First time seeing an enemy skill on a client, the replicated ASC cache may not be ready yet.
    // Directly resolving the skill soft refs prevents visuals/sounds from appearing only after
    // the local player has used an analogous skill.
    if (!Montage)
    {
        Montage = const_cast<UNiosCore_SkillDataAsset*>(Skill)->LoadCastLoopMontageSynchronous();
    }
    if (!Sound)
    {
        Sound = const_cast<UNiosCore_SkillDataAsset*>(Skill)->LoadCastLoopSoundSynchronous();
    }

    if (Montage)
    {
        if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
            {
                ActiveCastLoopMontage = Montage;
                AnimInstance->Montage_Play(Montage);
            }
        }
    }

    if (Sound)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> GCN CAST LOOP SOUND: Play Sound=%s Target=%s LocalRole=%d LocallyControlled=%d"),
            *GetNameSafe(Sound),
            *GetNameSafe(MyTarget),
            MyTarget ? (int32)MyTarget->GetLocalRole() : -1,
            Character->IsLocallyControlled() ? 1 : 0
        );

        if (ActiveCastLoopAudioComponent)
        {
            ActiveCastLoopAudioComponent->Stop();
            ActiveCastLoopAudioComponent = nullptr;
        }

        ActiveCastLoopAudioComponent = UGameplayStatics::SpawnSoundAttached(
            Sound,
            Character->GetRootComponent()
        );
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT(">>> GCN CAST LOOP SOUND: NULL Skill=%s Target=%s"),
            *GetNameSafe(Skill),
            *GetNameSafe(MyTarget)
        );
    }

    return Montage || Sound;
}

bool ANiosGCN_SkillCastLoop::OnRemove_Implementation(
    AActor* MyTarget,
    const FGameplayCueParameters& Parameters)
{
    if (!MyTarget)
        return false;

    ACharacter* Character = Cast<ACharacter>(MyTarget);
    if (!Character)
        return false;
    if (Character->IsLocallyControlled())
    {
        return false;
    }
    if (ActiveCastLoopMontage)
    {
        if (USkeletalMeshComponent* MeshComp = Character->GetMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
            {
                AnimInstance->Montage_Stop(0.15f, ActiveCastLoopMontage.Get());
            }
        }

        ActiveCastLoopMontage = nullptr;
    }

    if (ActiveCastLoopAudioComponent)
    {
        ActiveCastLoopAudioComponent->Stop();
        ActiveCastLoopAudioComponent = nullptr;
    }

    return true;
}
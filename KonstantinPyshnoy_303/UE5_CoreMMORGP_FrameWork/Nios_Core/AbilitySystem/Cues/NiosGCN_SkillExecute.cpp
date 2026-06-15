#include "AbilitySystem/Cues/NiosGCN_SkillExecute.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"

UNiosGCN_SkillExecute::UNiosGCN_SkillExecute()
{
   // GameplayCueTag = FGameplayTag::RequestGameplayTag(TEXT("GameplayCue.Skill.Execute"));
}

bool UNiosGCN_SkillExecute::OnExecute_Implementation(
    AActor* MyTarget,
    const FGameplayCueParameters& Parameters) const
{
    const UObject* SourceObj = Parameters.SourceObject.Get();

    UE_LOG(LogTemp, Warning, TEXT(">>> GCN EXECUTE CALLED. Target=%s Source=%s Role=%d"),
        *GetNameSafe(MyTarget),
        *GetNameSafe(SourceObj),
        MyTarget ? static_cast<int32>(MyTarget->GetLocalRole()) : -1
    );

    if (!MyTarget)
        return false;

    ACharacter* Character = Cast<ACharacter>(MyTarget);
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT(">>> GCN EXECUTE: Target is not Character"));
        return false;
    }

    if (Character->IsLocallyControlled())
    {
        return false;
    }

    const UNiosCore_SkillDataAsset* Skill =
        Cast<UNiosCore_SkillDataAsset>(SourceObj);

    if (!Skill)
    {
        UE_LOG(LogTemp, Error, TEXT(">>> GCN EXECUTE: Skill cast FAILED"));
        return false;
    }

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
            Montage = Assets->ExecuteMontage;
            Sound = Assets->ExecuteSound;
        }
    }

    // First time seeing an enemy skill on a client, the replicated ASC cache may not be ready yet.
    // Directly resolving the skill soft refs prevents visuals/sounds from appearing only after
    // the local player has used an analogous skill.
    if (!Montage)
    {
        Montage = const_cast<UNiosCore_SkillDataAsset*>(Skill)->LoadExecuteMontageSynchronous();
    }
    if (!Sound)
    {
        Sound = const_cast<UNiosCore_SkillDataAsset*>(Skill)->LoadExecuteSoundSynchronous();
    }

    bool bPlayedSomething = false;

    if (Montage)
    {
        USkeletalMeshComponent* MeshComp = Character->GetMesh();
        if (!MeshComp)
        {
            UE_LOG(LogTemp, Error, TEXT(">>> GCN EXECUTE: Mesh is NULL"));
        }
        else
        {
            UAnimInstance* AnimInstance = MeshComp->GetAnimInstance();
            if (!AnimInstance)
            {
                UE_LOG(LogTemp, Error, TEXT(">>> GCN EXECUTE: AnimInstance is NULL"));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT(">>> GCN EXECUTE: Playing ExecuteMontage %s"),
                    *GetNameSafe(Montage)
                );

                AnimInstance->Montage_Play(Montage);
                bPlayedSomething = true;
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT(">>> GCN EXECUTE: ExecuteMontage is NULL Skill=%s"),
            *GetNameSafe(Skill)
        );
    }

    if (Sound)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> GCN EXECUTE SOUND: Play Sound=%s Target=%s LocalRole=%d LocallyControlled=%d"),
            *GetNameSafe(Sound),
            *GetNameSafe(MyTarget),
            MyTarget ? static_cast<int32>(MyTarget->GetLocalRole()) : -1,
            Character->IsLocallyControlled() ? 1 : 0
        );

        UGameplayStatics::SpawnSoundAttached(
            Sound,
            Character->GetRootComponent()
        );

        bPlayedSomething = true;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT(">>> GCN EXECUTE SOUND: NULL Skill=%s Target=%s"),
            *GetNameSafe(Skill),
            *GetNameSafe(MyTarget)
        );
    }

    return bPlayedSomething;
}
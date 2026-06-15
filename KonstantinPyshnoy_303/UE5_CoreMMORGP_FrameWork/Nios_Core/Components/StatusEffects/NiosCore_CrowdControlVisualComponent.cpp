#include "Components/StatusEffects/NiosCore_CrowdControlVisualComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

UNiosCore_CrowdControlVisualComponent::UNiosCore_CrowdControlVisualComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UNiosCore_CrowdControlVisualComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (bAutoBindStatusEffectComponent && GetOwner())
    {
        InitializeCrowdControlVisuals(GetOwner()->FindComponentByClass<UNiosCore_StatusEffectComponent>());
    }
}

void UNiosCore_CrowdControlVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopCrowdControlVisuals(0.f);

    if (StatusEffectComponent)
    {
        StatusEffectComponent->OnActiveCrowdControlChanged.RemoveDynamic(this, &UNiosCore_CrowdControlVisualComponent::HandleActiveCrowdControlChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void UNiosCore_CrowdControlVisualComponent::InitializeCrowdControlVisuals(UNiosCore_StatusEffectComponent* InStatusEffectComponent)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (StatusEffectComponent == InStatusEffectComponent)
    {
        return;
    }

    if (StatusEffectComponent)
    {
        StatusEffectComponent->OnActiveCrowdControlChanged.RemoveDynamic(this, &UNiosCore_CrowdControlVisualComponent::HandleActiveCrowdControlChanged);
    }

    StatusEffectComponent = InStatusEffectComponent;

    if (StatusEffectComponent)
    {
        StatusEffectComponent->OnActiveCrowdControlChanged.AddUniqueDynamic(this, &UNiosCore_CrowdControlVisualComponent::HandleActiveCrowdControlChanged);
        HandleActiveCrowdControlChanged(StatusEffectComponent->GetHighestPriorityCrowdControlEffect(true, true));
    }
}

void UNiosCore_CrowdControlVisualComponent::HandleActiveCrowdControlChanged(const FNiosReplicatedStatusEffect& CrowdControlEffect)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!CrowdControlEffect.bHasCrowdControl || CrowdControlEffect.CrowdControlType == ENiosCrowdControlType::None)
    {
        StopCrowdControlVisuals();
        return;
    }

    const FName ProfileID = StatusEffectComponent ? StatusEffectComponent->GetPresentationProfileID() : FName(TEXT("Humanoid"));

    FNiosCrowdControlPresentationEntry Presentation;
    if (!UNiosCore_CrowdControlPresentationLibrary::ResolveCrowdControlPresentation(this, ProfileID, CrowdControlEffect.CrowdControlType, CrowdControlEffect.CrowdControlPresentationID, Presentation))
    {
        if (bLogDebug)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> CC VISUAL: No presentation Profile=%s CC=%d PresentationID=%s Owner=%s"),
                *ProfileID.ToString(),
                static_cast<int32>(CrowdControlEffect.CrowdControlType),
                *CrowdControlEffect.CrowdControlPresentationID.ToString(),
                *GetNameSafe(GetOwner()));
        }

        StopCrowdControlVisuals();
        return;
    }

    const bool bSameVisual = ActiveCrowdControlType == CrowdControlEffect.CrowdControlType
        && ActiveCrowdControlPresentationID == CrowdControlEffect.CrowdControlPresentationID;

    if (bSameVisual && ActiveLoopMontage)
    {
        return;
    }

    ApplyPresentation(CrowdControlEffect, Presentation);
}

void UNiosCore_CrowdControlVisualComponent::ApplyPresentation(const FNiosReplicatedStatusEffect& CrowdControlEffect, const FNiosCrowdControlPresentationEntry& Presentation)
{
    StopCrowdControlVisuals();

    ActiveCrowdControlType = CrowdControlEffect.CrowdControlType;
    ActiveCrowdControlPresentationID = CrowdControlEffect.CrowdControlPresentationID;
    OnCrowdControlVisualChanged.Broadcast(CrowdControlEffect, Presentation);

    if (!bPlayNativeMontages)
    {
        return;
    }

    UAnimInstance* AnimInstance = ResolveAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    UAnimMontage* EnterMontage = Presentation.EnterMontage.LoadSynchronous();
    PendingLoopMontage = Presentation.LoopMontage.LoadSynchronous();
    ActiveExitMontage = Presentation.ExitMontage.LoadSynchronous();

    if (EnterMontage)
    {
        const float PlayedLength = AnimInstance->Montage_Play(EnterMontage);
        if (PendingLoopMontage)
        {
            const float Delay = PlayedLength > 0.f ? PlayedLength : 0.05f;
            if (UWorld* World = GetWorld())
            {
                World->GetTimerManager().SetTimer(StartLoopTimerHandle, this, &UNiosCore_CrowdControlVisualComponent::StartLoopMontage, Delay, false);
            }
        }
    }
    else
    {
        StartLoopMontage();
    }
}

void UNiosCore_CrowdControlVisualComponent::StartLoopMontage()
{
    if (!PendingLoopMontage)
    {
        return;
    }

    UAnimInstance* AnimInstance = ResolveAnimInstance();
    if (!AnimInstance)
    {
        return;
    }

    ActiveLoopMontage = PendingLoopMontage;
    PendingLoopMontage = nullptr;
    AnimInstance->Montage_Play(ActiveLoopMontage);
}

void UNiosCore_CrowdControlVisualComponent::StopCrowdControlVisuals(float BlendOutTime)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(StartLoopTimerHandle);
    }

    if (bPlayNativeMontages)
    {
        if (UAnimInstance* AnimInstance = ResolveAnimInstance())
        {
            if (ActiveLoopMontage)
            {
                AnimInstance->Montage_Stop(BlendOutTime, ActiveLoopMontage);
            }

            if (ActiveExitMontage)
            {
                AnimInstance->Montage_Play(ActiveExitMontage);
            }
        }
    }

    ActiveLoopMontage = nullptr;
    ActiveExitMontage = nullptr;
    PendingLoopMontage = nullptr;
    ActiveCrowdControlType = ENiosCrowdControlType::None;
    ActiveCrowdControlPresentationID = NAME_None;
}

UAnimInstance* UNiosCore_CrowdControlVisualComponent::ResolveAnimInstance() const
{
    const AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    USkeletalMeshComponent* MeshComponent = Owner->FindComponentByClass<USkeletalMeshComponent>();
    return MeshComponent ? MeshComponent->GetAnimInstance() : nullptr;
}

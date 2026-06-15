#include "Components/Targeting/NiosCore_TargetFeedbackComponent.h"

#include "Components/DecalComponent.h"
#include "GameFramework/Actor.h"

UNiosCore_TargetFeedbackComponent::UNiosCore_TargetFeedbackComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UNiosCore_TargetFeedbackComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bHideTargetDecalOnBeginPlay)
    {
        SetTargetDecalVisible(false);
    }
}

bool UNiosCore_TargetFeedbackComponent::DispatchLocalTargetFeedback(AActor* TargetActor, ENiosCoreTargetFeedbackEventType EventType, const FNiosCoreTargetFeedbackContext& Context)
{
    if (!IsValid(TargetActor))
    {
        return false;
    }

    UNiosCore_TargetFeedbackComponent* FeedbackComponent = TargetActor->FindComponentByClass<UNiosCore_TargetFeedbackComponent>();
    if (!FeedbackComponent)
    {
        return false;
    }

    FeedbackComponent->HandleLocalTargetFeedback(EventType, Context);
    return true;
}

void UNiosCore_TargetFeedbackComponent::HandleLocalTargetFeedback(ENiosCoreTargetFeedbackEventType EventType, const FNiosCoreTargetFeedbackContext& Context)
{
    if (EventType == ENiosCoreTargetFeedbackEventType::Targeted)
    {
        bLocallyTargeted = true;
        if (bAutoToggleTargetDecal)
        {
            SetTargetDecalVisible(true);
        }

        OnLocallyTargeted.Broadcast(Context);
        BP_OnLocallyTargeted(Context);
    }
    else if (EventType == ENiosCoreTargetFeedbackEventType::Untargeted)
    {
        bLocallyTargeted = false;
        if (bAutoToggleTargetDecal)
        {
            SetTargetDecalVisible(false);
        }

        OnLocallyUntargeted.Broadcast(Context);
        BP_OnLocallyUntargeted(Context);
    }

    OnLocalTargetFeedback.Broadcast(EventType, Context);
    BP_OnLocalTargetFeedback(EventType, Context);
}

UDecalComponent* UNiosCore_TargetFeedbackComponent::ResolveTargetDecalComponent()
{
    if (IsValid(CachedTargetDecalComponent))
    {
        return CachedTargetDecalComponent.Get();
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    TArray<UDecalComponent*> DecalComponents;
    OwnerActor->GetComponents<UDecalComponent>(DecalComponents);

    for (UDecalComponent* DecalComponent : DecalComponents)
    {
        if (DecalComponent && TargetDecalComponentTag != NAME_None && DecalComponent->ComponentHasTag(TargetDecalComponentTag))
        {
            CachedTargetDecalComponent = DecalComponent;
            return CachedTargetDecalComponent.Get();
        }
    }

    for (UDecalComponent* DecalComponent : DecalComponents)
    {
        if (DecalComponent && TargetDecalComponentName != NAME_None && DecalComponent->GetFName() == TargetDecalComponentName)
        {
            CachedTargetDecalComponent = DecalComponent;
            return CachedTargetDecalComponent.Get();
        }
    }

    return nullptr;
}

void UNiosCore_TargetFeedbackComponent::SetTargetDecalVisible(bool bVisible)
{
    if (UDecalComponent* DecalComponent = ResolveTargetDecalComponent())
    {
        DecalComponent->SetVisibility(bVisible, true);
        DecalComponent->SetHiddenInGame(!bVisible, true);
    }
}

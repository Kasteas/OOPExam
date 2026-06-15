#include "AbilitySystem/Targeting/NiosTargetActor_Mouse.h"

#include "Abilities/GameplayAbility.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

ANiosTargetActor_Mouse::ANiosTargetActor_Mouse()
{
    bDestroyOnConfirmation = true;
    ShouldProduceTargetDataOnServer = false;
}

void ANiosTargetActor_Mouse::StartTargeting(UGameplayAbility* Ability)
{
    Super::StartTargeting(Ability);

    SourcePC = nullptr;

    if (!OwningAbility)
        return;

    const FGameplayAbilityActorInfo* ActorInfo = OwningAbility->GetCurrentActorInfo();
    if (!ActorInfo)
        return;

    SourcePC = ActorInfo->PlayerController.Get();

    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET ACTOR: StartTargeting PC=%s"),
        *GetNameSafe(SourcePC)
    );

    ConfirmTargetingAndContinue();
}

void ANiosTargetActor_Mouse::ConfirmTargetingAndContinue()
{
    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET ACTOR: ConfirmTargetingAndContinue"));

    FGameplayAbilityTargetDataHandle TargetData;

    if (!SourcePC)
    {
        UE_LOG(LogTemp, Error, TEXT(">>> TARGET ACTOR: SourcePC NULL"));
        TargetDataReadyDelegate.Broadcast(TargetData);
        return;
    }

    FHitResult Hit;
    SourcePC->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    AActor* HitActor = Hit.GetActor();

    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET ACTOR: HitActor=%s BlockingHit=%d"),
        *GetNameSafe(HitActor),
        Hit.bBlockingHit ? 1 : 0
    );

    if (!HitActor)
    {
        TargetDataReadyDelegate.Broadcast(TargetData);
        return;
    }

    FGameplayAbilityTargetData_ActorArray* Data =
        new FGameplayAbilityTargetData_ActorArray();

    Data->TargetActorArray.Add(HitActor);
    TargetData.Add(Data);

    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET ACTOR: Broadcast TargetData Num=%d"),
        TargetData.Num()
    );

    TargetDataReadyDelegate.Broadcast(TargetData);
}
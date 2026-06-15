// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/Nios_CoreBPLibrary.h"
#include "Core/Nios_Core.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UNios_CoreBPLibrary::UNios_CoreBPLibrary(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{

}

float UNios_CoreBPLibrary::Nios_CoreSampleFunction(float Param)
{
    return -1;
}

UNiosCore_StatusEffectComponent* UNios_CoreBPLibrary::GetNiosStatusEffectComponentFromActor(AActor* Actor)
{
    if (!Actor)
    {
        return nullptr;
    }

    if (Actor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
    {
        if (UNiosCore_StatusEffectComponent* Component = INiosGASActorInterface::Execute_GetStatusEffectComponentGeneric(Actor))
        {
            return Component;
        }
    }

    if (UNiosCore_StatusEffectComponent* Component = Actor->FindComponentByClass<UNiosCore_StatusEffectComponent>())
    {
        return Component;
    }

    if (const APawn* Pawn = Cast<APawn>(Actor))
    {
        if (APlayerState* PlayerState = Pawn->GetPlayerState())
        {
            return GetNiosStatusEffectComponentFromActor(PlayerState);
        }
    }

    return nullptr;
}

UNiosCore_StatusEffectComponent* UNios_CoreBPLibrary::GetNiosStatusEffectComponentFromASC(UAbilitySystemComponent* ASC)
{
    if (!ASC)
    {
        return nullptr;
    }

    if (UNiosCore_StatusEffectComponent* Component = GetNiosStatusEffectComponentFromActor(ASC->GetOwnerActor()))
    {
        return Component;
    }

    if (UNiosCore_StatusEffectComponent* Component = GetNiosStatusEffectComponentFromActor(ASC->GetAvatarActor()))
    {
        return Component;
    }

    return nullptr;
}

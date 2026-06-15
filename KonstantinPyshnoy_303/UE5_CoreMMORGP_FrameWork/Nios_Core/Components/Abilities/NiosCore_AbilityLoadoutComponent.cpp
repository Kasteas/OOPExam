#include "Components/Abilities/NiosCore_AbilityLoadoutComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Abilities/GameplayAbility.h"
#include "GameFramework/Actor.h"

UNiosCore_AbilityLoadoutComponent::UNiosCore_AbilityLoadoutComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(false);
}

void UNiosCore_AbilityLoadoutComponent::BeginPlay()
{
    Super::BeginPlay();

    // Safe auto-grant for server-owned PlayerState/Character setups.
    // PlayerState::InitAvatar also calls this after ASC avatar initialization, so this is intentionally idempotent.
    GrantStartupAbilities();
}

void UNiosCore_AbilityLoadoutComponent::GrantStartupAbilities(UAbilitySystemComponent* ASCOverride)
{
    UAbilitySystemComponent* ASC = ResolveASC(ASCOverride);
    AActor* OwnerActor = GetOwner();

    if (!ASC || !OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }

    if (bStartupAbilitiesGranted)
    {
        return;
    }

    GrantAbilityArray(ASC, CombatAbilities, TEXT("Combat"));
    GrantAbilityArray(ASC, UtilityAbilities, TEXT("Utility"));
    GrantAbilityArray(ASC, ItemAbilities, TEXT("Items"));
    GrantAbilityArray(ASC, EmoteAbilities, TEXT("Emotes"));
    GrantAbilityArray(ASC, MountAbilities, TEXT("Mounts"));
    GrantAbilityArray(ASC, ProfessionAbilities, TEXT("Professions"));
    GrantAbilityArray(ASC, InteractionAbilities, TEXT("Interaction"));
    GrantAbilityArray(ASC, DebugAbilities, TEXT("Debug"));

    bStartupAbilitiesGranted = true;

    UE_LOG(LogTemp, Warning, TEXT(">>> ABILITY LOADOUT: Granted persistent abilities Owner=%s Count=%d"),
        *GetNameSafe(OwnerActor), GrantedAbilityHandles.Num());
}

void UNiosCore_AbilityLoadoutComponent::ClearGrantedStartupAbilities(UAbilitySystemComponent* ASCOverride)
{
    UAbilitySystemComponent* ASC = ResolveASC(ASCOverride);
    AActor* OwnerActor = GetOwner();

    if (!ASC || !OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }

    for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
    {
        if (Handle.IsValid())
        {
            ASC->ClearAbility(Handle);
        }
    }

    GrantedAbilityHandles.Reset();
    bStartupAbilitiesGranted = false;

    UE_LOG(LogTemp, Warning, TEXT(">>> ABILITY LOADOUT: Cleared persistent abilities Owner=%s"), *GetNameSafe(OwnerActor));
}

bool UNiosCore_AbilityLoadoutComponent::TryActivateAbilityByClass(TSubclassOf<UGameplayAbility> AbilityClass, UAbilitySystemComponent* ASCOverride) const
{
    UAbilitySystemComponent* ASC = ResolveASC(ASCOverride);
    if (!ASC || !AbilityClass)
    {
        return false;
    }

    return ASC->TryActivateAbilityByClass(AbilityClass);
}

UAbilitySystemComponent* UNiosCore_AbilityLoadoutComponent::ResolveASC(UAbilitySystemComponent* ASCOverride) const
{
    if (ASCOverride)
    {
        return ASCOverride;
    }

    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    if (const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(OwnerActor))
    {
        return AbilityOwner->GetAbilitySystemComponent();
    }

    return OwnerActor->FindComponentByClass<UAbilitySystemComponent>();
}

void UNiosCore_AbilityLoadoutComponent::GrantAbilityArray(UAbilitySystemComponent* ASC, const TArray<TSubclassOf<UGameplayAbility>>& Abilities, const TCHAR* BucketName)
{
    if (!ASC)
    {
        return;
    }

    for (TSubclassOf<UGameplayAbility> AbilityClass : Abilities)
    {
        if (!AbilityClass)
        {
            continue;
        }

        if (ASCAlreadyHasAbilityClass(ASC, AbilityClass))
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> ABILITY LOADOUT: Skip duplicate Ability=%s Bucket=%s Owner=%s"),
                *GetNameSafe(AbilityClass.Get()), BucketName, *GetNameSafe(GetOwner()));
            continue;
        }

        FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, this);
        const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
        if (Handle.IsValid())
        {
            GrantedAbilityHandles.Add(Handle);
            UE_LOG(LogTemp, Warning, TEXT(">>> ABILITY LOADOUT: Give Ability=%s Bucket=%s Owner=%s HandleValid=1"),
                *GetNameSafe(AbilityClass.Get()), BucketName, *GetNameSafe(GetOwner()));
        }
    }
}

bool UNiosCore_AbilityLoadoutComponent::ASCAlreadyHasAbilityClass(const UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass) const
{
    if (!ASC || !AbilityClass)
    {
        return false;
    }

    for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
    {
        if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
        {
            return true;
        }
    }

    return false;
}

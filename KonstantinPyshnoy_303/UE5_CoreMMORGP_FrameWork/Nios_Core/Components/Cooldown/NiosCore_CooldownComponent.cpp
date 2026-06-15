#include "Components/Cooldown/NiosCore_CooldownComponent.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"

UNiosCore_CooldownComponent::UNiosCore_CooldownComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    // No per-frame replication. Replication is enabled only so owner-client RPCs can deliver
    // cooldown start events to ActionSlot UI.
    SetIsReplicatedByDefault(true);
}

float UNiosCore_CooldownComponent::GetNow() const
{
    return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
}

float UNiosCore_CooldownComponent::GetRemainingForEndTime(float Now, float EndTime)
{
    return FMath::Max(0.f, EndTime - Now);
}

bool UNiosCore_CooldownComponent::IsGlobalSkillCooldownReady() const
{
    if (!bUseGlobalSkillCooldown)
    {
        return true;
    }

    return GetGlobalSkillCooldownRemaining() <= KINDA_SMALL_NUMBER;
}

float UNiosCore_CooldownComponent::GetGlobalSkillCooldownRemaining() const
{
    return GetRemainingForEndTime(GetNow(), GlobalSkillCooldownEntry.EndTime);
}

void UNiosCore_CooldownComponent::StartGlobalSkillCooldown(float DurationOverride)
{
    if (!bUseGlobalSkillCooldown)
    {
        return;
    }

    const float Duration = DurationOverride >= 0.f ? DurationOverride : GlobalSkillCooldown;
    if (Duration <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    ApplyCooldownStartedLocal(ENiosCoreCooldownDomain::GlobalSkill, TEXT("GlobalSkill"), Duration, true);
    NotifyOwningClientCooldownStarted(ENiosCoreCooldownDomain::GlobalSkill, TEXT("GlobalSkill"), Duration);
}

void UNiosCore_CooldownComponent::ClearGlobalSkillCooldown()
{
    GlobalSkillCooldownEntry = FNiosCoreCooldownEntry();
    OnCooldownCleared.Broadcast(ENiosCoreCooldownDomain::GlobalSkill, TEXT("GlobalSkill"));
}

bool UNiosCore_CooldownComponent::IsSkillCooldownReady(FName SkillID) const
{
    return GetSkillCooldownRemaining(SkillID) <= KINDA_SMALL_NUMBER;
}

float UNiosCore_CooldownComponent::GetSkillCooldownRemaining(FName SkillID) const
{
    if (SkillID == NAME_None)
    {
        return 0.f;
    }

    if (const FNiosCoreCooldownEntry* Entry = SkillCooldowns.Find(SkillID))
    {
        return GetRemainingForEndTime(GetNow(), Entry->EndTime);
    }

    return 0.f;
}

float UNiosCore_CooldownComponent::GetSkillCooldownDuration(FName SkillID) const
{
    if (const FNiosCoreCooldownEntry* Entry = SkillCooldowns.Find(SkillID))
    {
        return Entry->Duration;
    }

    return 0.f;
}

void UNiosCore_CooldownComponent::StartSkillCooldown(FName SkillID, float Duration)
{
    if (SkillID == NAME_None || Duration <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    ApplyCooldownStartedLocal(ENiosCoreCooldownDomain::Skill, SkillID, Duration, true);
    NotifyOwningClientCooldownStarted(ENiosCoreCooldownDomain::Skill, SkillID, Duration);
}

void UNiosCore_CooldownComponent::ClearSkillCooldown(FName SkillID)
{
    SkillCooldowns.Remove(SkillID);
    OnCooldownCleared.Broadcast(ENiosCoreCooldownDomain::Skill, SkillID);
}

bool UNiosCore_CooldownComponent::IsItemCooldownReady(FName ItemID) const
{
    return GetItemCooldownRemaining(ItemID) <= KINDA_SMALL_NUMBER;
}

float UNiosCore_CooldownComponent::GetItemCooldownRemaining(FName ItemID) const
{
    if (ItemID == NAME_None)
    {
        return 0.f;
    }

    if (const FNiosCoreCooldownEntry* Entry = ItemCooldowns.Find(ItemID))
    {
        return GetRemainingForEndTime(GetNow(), Entry->EndTime);
    }

    return 0.f;
}

float UNiosCore_CooldownComponent::GetItemCooldownDuration(FName ItemID) const
{
    if (const FNiosCoreCooldownEntry* Entry = ItemCooldowns.Find(ItemID))
    {
        return Entry->Duration;
    }

    return DefaultItemCooldown;
}

void UNiosCore_CooldownComponent::StartItemCooldown(FName ItemID, float Duration)
{
    if (ItemID == NAME_None)
    {
        return;
    }

    const float FinalDuration = Duration >= 0.f ? Duration : DefaultItemCooldown;
    if (FinalDuration <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    ApplyCooldownStartedLocal(ENiosCoreCooldownDomain::Item, ItemID, FinalDuration, true);
    NotifyOwningClientCooldownStarted(ENiosCoreCooldownDomain::Item, ItemID, FinalDuration);
}

void UNiosCore_CooldownComponent::ClearItemCooldown(FName ItemID)
{
    ItemCooldowns.Remove(ItemID);
    OnCooldownCleared.Broadcast(ENiosCoreCooldownDomain::Item, ItemID);
}

void UNiosCore_CooldownComponent::ApplyCooldownStartedLocal(ENiosCoreCooldownDomain Domain, FName ID, float Duration, bool bBroadcast)
{
    if (Duration <= KINDA_SMALL_NUMBER)
    {
        return;
    }

    const float Now = GetNow();
    FNiosCoreCooldownEntry* Entry = nullptr;

    switch (Domain)
    {
    case ENiosCoreCooldownDomain::GlobalSkill:
        Entry = &GlobalSkillCooldownEntry;
        ID = ID == NAME_None ? TEXT("GlobalSkill") : ID;
        break;
    case ENiosCoreCooldownDomain::Skill:
        if (ID == NAME_None)
        {
            return;
        }
        Entry = &SkillCooldowns.FindOrAdd(ID);
        break;
    case ENiosCoreCooldownDomain::Item:
        if (ID == NAME_None)
        {
            return;
        }
        Entry = &ItemCooldowns.FindOrAdd(ID);
        break;
    default:
        return;
    }

    Entry->ID = ID;
    Entry->StartTime = Now;
    Entry->Duration = Duration;
    Entry->EndTime = Now + Duration;

    UE_LOG(LogTemp, Warning, TEXT(">>> COOLDOWN START: Domain=%d ID=%s Duration=%.2f End=%.3f Owner=%s LocalRole=%d"),
        static_cast<int32>(Domain),
        *ID.ToString(),
        Duration,
        Entry->EndTime,
        *GetNameSafe(GetOwner()),
        GetOwner() ? static_cast<int32>(GetOwner()->GetLocalRole()) : -1);

    if (bBroadcast)
    {
        OnCooldownStarted.Broadcast(Domain, ID, Duration);
    }
}

void UNiosCore_CooldownComponent::NotifyOwningClientCooldownStarted(ENiosCoreCooldownDomain Domain, FName ID, float Duration)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority())
    {
        return;
    }

    // Owner-client RPC. The client stores its own local EndTime from Duration and then ActionSlot
    // queries work without server ticking or frequent replication.
    Client_ReceiveCooldownStarted(Domain, ID, Duration);
}

void UNiosCore_CooldownComponent::Client_ReceiveCooldownStarted_Implementation(ENiosCoreCooldownDomain Domain, FName ID, float Duration)
{
    ApplyCooldownStartedLocal(Domain, ID, Duration, true);

    UE_LOG(LogTemp, Warning, TEXT(">>> COOLDOWN CLIENT RECEIVED: Domain=%d ID=%s Duration=%.2f Owner=%s"),
        static_cast<int32>(Domain),
        *ID.ToString(),
        Duration,
        *GetNameSafe(GetOwner()));
}


FNiosCoreCooldownState UNiosCore_CooldownComponent::GetSkillCooldownState(FName SkillID) const
{
    FNiosCoreCooldownState State;
    State.DominantDomain = ENiosCoreCooldownDomain::Skill;
    State.DominantID = SkillID;

    const float GlobalRemaining = GetGlobalSkillCooldownRemaining();
    const float GlobalDuration = GetCooldownDuration(ENiosCoreCooldownDomain::GlobalSkill, TEXT("GlobalSkill"));
    const float LocalRemaining = GetSkillCooldownRemaining(SkillID);
    const float LocalDuration = GetSkillCooldownDuration(SkillID);

    State.bGlobalCooldownActive = GlobalRemaining > KINDA_SMALL_NUMBER;
    State.bLocalCooldownActive = LocalRemaining > KINDA_SMALL_NUMBER;

    if (GlobalRemaining >= LocalRemaining && State.bGlobalCooldownActive)
    {
        State.bOnCooldown = true;
        State.Remaining = GlobalRemaining;
        State.Duration = GlobalDuration > KINDA_SMALL_NUMBER ? GlobalDuration : GlobalRemaining;
        State.DominantDomain = ENiosCoreCooldownDomain::GlobalSkill;
        State.DominantID = TEXT("GlobalSkill");
    }
    else if (State.bLocalCooldownActive)
    {
        State.bOnCooldown = true;
        State.Remaining = LocalRemaining;
        State.Duration = LocalDuration > KINDA_SMALL_NUMBER ? LocalDuration : LocalRemaining;
        State.DominantDomain = ENiosCoreCooldownDomain::Skill;
        State.DominantID = SkillID;
    }

    State.Normalized = State.Duration > KINDA_SMALL_NUMBER ? FMath::Clamp(State.Remaining / State.Duration, 0.f, 1.f) : 0.f;
    return State;
}

FNiosCoreCooldownState UNiosCore_CooldownComponent::GetItemCooldownState(FName ItemID) const
{
    FNiosCoreCooldownState State;
    State.DominantDomain = ENiosCoreCooldownDomain::Item;
    State.DominantID = ItemID;

    const float LocalRemaining = GetItemCooldownRemaining(ItemID);
    const float LocalDuration = GetItemCooldownDuration(ItemID);

    State.bLocalCooldownActive = LocalRemaining > KINDA_SMALL_NUMBER;
    if (State.bLocalCooldownActive)
    {
        State.bOnCooldown = true;
        State.Remaining = LocalRemaining;
        State.Duration = LocalDuration > KINDA_SMALL_NUMBER ? LocalDuration : LocalRemaining;
        State.Normalized = State.Duration > KINDA_SMALL_NUMBER ? FMath::Clamp(State.Remaining / State.Duration, 0.f, 1.f) : 0.f;
    }

    return State;
}

FNiosCoreCooldownState UNiosCore_CooldownComponent::GetCooldownStateForActivationRequest(const FNiosCoreActivationRequest& Request) const
{
    switch (Request.Kind)
    {
    case ENiosCoreActivationKind::Skill:
        return GetSkillCooldownState(Request.ID);

    case ENiosCoreActivationKind::Item:
        return GetItemCooldownState(Request.ID);

    default:
        return FNiosCoreCooldownState();
    }
}

float UNiosCore_CooldownComponent::GetCooldownRemaining(ENiosCoreCooldownDomain Domain, FName ID) const
{
    switch (Domain)
    {
    case ENiosCoreCooldownDomain::GlobalSkill:
        return GetGlobalSkillCooldownRemaining();
    case ENiosCoreCooldownDomain::Skill:
        return GetSkillCooldownRemaining(ID);
    case ENiosCoreCooldownDomain::Item:
        return GetItemCooldownRemaining(ID);
    default:
        return 0.f;
    }
}

float UNiosCore_CooldownComponent::GetCooldownDuration(ENiosCoreCooldownDomain Domain, FName ID) const
{
    switch (Domain)
    {
    case ENiosCoreCooldownDomain::GlobalSkill:
        return GlobalSkillCooldownEntry.Duration > 0.f ? GlobalSkillCooldownEntry.Duration : GlobalSkillCooldown;
    case ENiosCoreCooldownDomain::Skill:
        return GetSkillCooldownDuration(ID);
    case ENiosCoreCooldownDomain::Item:
        return GetItemCooldownDuration(ID);
    default:
        return 0.f;
    }
}

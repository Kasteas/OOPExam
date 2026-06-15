#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillEffectProcessor.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Data/StatusEffects/NiosCore_StatusEffectDataAsset.h"
#include "Data/StatusEffects/NiosCore_StatusEffectCatalog.h"
#include "Engine/OverlapResult.h"
#include "Net/UnrealNetwork.h"
#include "Rules/Targeting/NiosTargetRulesSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

UNiosCore_StatusEffectComponent::UNiosCore_StatusEffectComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    SetIsReplicatedByDefault(true);
}

void UNiosCore_StatusEffectComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(GetOwner() && GetOwner()->HasAuthority());
}

void UNiosCore_StatusEffectComponent::InitializeStatusEffectComponent(UAbilitySystemComponent* InASC, UNiosCore_StatsComponent* InStatsComponent)
{
    AbilitySystemComponent = InASC;
    StatsComponent = InStatsComponent;
    ApplyStartupStatusEffects();
}

void UNiosCore_StatusEffectComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!GetOwner() || !GetOwner()->HasAuthority() || !AbilitySystemComponent)
    {
        return;
    }

    const UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();

    for (FNiosActiveStatusEffect& ActiveEffect : ActiveEffects)
    {
        TickPeriodicOperations(ActiveEffect);
        TickAuraOperations(ActiveEffect);
    }

    RemoveExpiredEffects(Now);
}

void UNiosCore_StatusEffectComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UNiosCore_StatusEffectComponent, PublicStatusEffects);
    DOREPLIFETIME_CONDITION(UNiosCore_StatusEffectComponent, PersonalStatusEffects, COND_OwnerOnly);
}

FGuid UNiosCore_StatusEffectComponent::ApplyStatusEffectByID(FName EffectID, AActor* SourceActor, ENiosStatusEffectSourceType SourceType, FName SourceID)
{
    FNiosStatusEffectCatalogEntry Entry;
    if (!ResolveCatalogEntry(EffectID, Entry) || !Entry.EffectData)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT APPLY FAILED: Catalog missing EffectID=%s Owner=%s"),
            *EffectID.ToString(),
            *GetNameSafe(GetOwner()));
        return FGuid();
    }

    return ApplyStatusEffectInternal(Entry.EffectData, Entry.EffectID, Entry.Name, Entry.Description, Entry.Icon, SourceActor, SourceType, SourceID.IsNone() ? Entry.EffectID : SourceID);
}

FGuid UNiosCore_StatusEffectComponent::ApplyStatusEffect(UNiosCore_StatusEffectDataAsset* Effect, AActor* SourceActor, ENiosStatusEffectSourceType SourceType, FName SourceID)
{
    const FName FallbackEffectID = SourceID.IsNone() ? FName(*GetNameSafe(Effect)) : SourceID;
    return ApplyStatusEffectInternal(Effect, FallbackEffectID, FText::FromName(FallbackEffectID), FText::GetEmpty(), TSoftObjectPtr<UTexture2D>(), SourceActor, SourceType, SourceID);
}

FGuid UNiosCore_StatusEffectComponent::ApplyStatusEffectInternal(
    UNiosCore_StatusEffectDataAsset* Effect,
    FName EffectID,
    const FText& DisplayName,
    const FText& Description,
    TSoftObjectPtr<UTexture2D> Icon,
    AActor* SourceActor,
    ENiosStatusEffectSourceType SourceType,
    FName SourceID)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Effect || !AbilitySystemComponent || EffectID.IsNone())
    {
        return FGuid();
    }

    if (SourceType == ENiosStatusEffectSourceType::Aura && DoesEffectContainAuraOperation(Effect))
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT AURA APPLY BLOCKED: Aura source cannot apply producer/aura effect EffectID=%s Owner=%s SourceID=%s"),
            *EffectID.ToString(),
            *GetNameSafe(GetOwner()),
            *SourceID.ToString());
        return FGuid();
    }

    const UWorld* World = GetWorld();
    const float Now = World ? World->GetTimeSeconds() : 0.f;
    const FName EffectiveSourceID = SourceID.IsNone() ? EffectID : SourceID;

    if (FNiosActiveStatusEffect* Existing = FindStackableEffect(EffectID, Effect, SourceType, EffectiveSourceID))
    {
        switch (Effect->StackPolicy)
        {
        case ENiosStatusEffectStackPolicy::Ignore:
            return Existing->RuntimeID;

        case ENiosStatusEffectStackPolicy::RefreshDuration:
            RefreshDuration(*Existing, Now);
            RebuildReplicatedStatusEffects();
            return Existing->RuntimeID;

        case ENiosStatusEffectStackPolicy::AddStack:
            RemovePersistentOperations(*Existing);
            Existing->AppliedStatSourceIDs.Reset();
            Existing->AppliedGrantedTags.Reset();
            Existing->AppliedRemovedTags.Reset();
            CleanupAuraTargets(*Existing);
            Existing->RuntimeOperationValues.Reset();
            Existing->RuntimeOperationNextTimes.Reset();
            Existing->StackCount = FMath::Clamp(Existing->StackCount + 1, 1, FMath::Max(1, Effect->MaxStacks));
            RefreshDuration(*Existing, Now);
            BuildRuntimeOperationValues(*Existing);
            InitializeOperationRuntimeState(*Existing, Now);
            ApplyPersistentOperations(*Existing);
            RebuildReplicatedStatusEffects();
            return Existing->RuntimeID;

        case ENiosStatusEffectStackPolicy::Replace:
            RemoveStatusEffect(Existing->RuntimeID);
            break;
        }
    }

    FNiosActiveStatusEffect ActiveEffect;
    ActiveEffect.Effect = Effect;
    ActiveEffect.EffectID = EffectID;
    ActiveEffect.DisplayName = DisplayName;
    ActiveEffect.Description = Description;
    ActiveEffect.Icon = Icon;
    ActiveEffect.RuntimeID = FGuid::NewGuid();
    ActiveEffect.SourceType = SourceType;
    ActiveEffect.SourceActor = SourceActor;
    ActiveEffect.SourceID = EffectiveSourceID;
    ActiveEffect.StartTime = Now;
    ActiveEffect.NextTickTime = Effect->HasPeriodicOperations() ? Now + Effect->TickInterval : 0.f;
    RefreshDuration(ActiveEffect, Now);
    BuildRuntimeOperationValues(ActiveEffect);
    InitializeOperationRuntimeState(ActiveEffect, Now);

    if (Effect->DurationPolicy == ENiosStatusEffectDurationPolicy::Instant)
    {
        ApplyInstantOperations(ActiveEffect);
        return ActiveEffect.RuntimeID;
    }

    ApplyPersistentOperations(ActiveEffect);
    ActiveEffects.Add(ActiveEffect);
    RebuildReplicatedStatusEffects();
    OnStatusEffectsChanged.Broadcast();

    return ActiveEffect.RuntimeID;
}

bool UNiosCore_StatusEffectComponent::RemoveStatusEffect(FGuid RuntimeID)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !RuntimeID.IsValid())
    {
        return false;
    }

    for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
    {
        if (ActiveEffects[Index].RuntimeID == RuntimeID)
        {
            RemovePersistentOperations(ActiveEffects[Index]);
            ActiveEffects.RemoveAt(Index);
            RebuildReplicatedStatusEffects();
            OnStatusEffectsChanged.Broadcast();
            return true;
        }
    }

    return false;
}

int32 UNiosCore_StatusEffectComponent::RemoveStatusEffectsBySource(ENiosStatusEffectSourceType SourceType, FName SourceID)
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return 0;
    }

    int32 RemovedCount = 0;
    for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
    {
        if (ActiveEffects[Index].SourceType == SourceType && (SourceID.IsNone() || ActiveEffects[Index].SourceID == SourceID))
        {
            RemovePersistentOperations(ActiveEffects[Index]);
            ActiveEffects.RemoveAt(Index);
            ++RemovedCount;
        }
    }

    if (RemovedCount > 0)
    {
        RebuildReplicatedStatusEffects();
        OnStatusEffectsChanged.Broadcast();
    }

    return RemovedCount;
}

void UNiosCore_StatusEffectComponent::RefreshPersistentEffectsAfterRevive()
{
    if (!GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    const int32 PreviousCount = ActiveEffects.Num();

    for (FNiosActiveStatusEffect& ActiveEffect : ActiveEffects)
    {
        RemovePersistentOperations(ActiveEffect);
    }

    ActiveEffects.Reset();
    bStartupEffectsApplied = false;

    RebuildReplicatedStatusEffects();
    ApplyStartupStatusEffects();

    if (PreviousCount > 0 || ActiveEffects.Num() > 0)
    {
        RebuildReplicatedStatusEffects();
        OnStatusEffectsChanged.Broadcast();
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECTS REFRESHED AFTER REVIVE: Owner=%s Removed=%d ReappliedPassive=%d"),
        *GetNameSafe(GetOwner()),
        PreviousCount,
        ActiveEffects.Num());
}


void UNiosCore_StatusEffectComponent::GetVisibleStatusEffects(bool bIncludePersonal, bool bIncludePublic, TArray<FNiosReplicatedStatusEffect>& OutEffects) const
{
    OutEffects.Reset();

    if (bIncludePersonal)
    {
        OutEffects.Append(PersonalStatusEffects);
    }

    if (bIncludePublic)
    {
        OutEffects.Append(PublicStatusEffects);
    }
}

int32 UNiosCore_StatusEffectComponent::GetVisibleStatusEffectCount(bool bIncludePersonal, bool bIncludePublic) const
{
    int32 Count = 0;

    if (bIncludePersonal)
    {
        Count += PersonalStatusEffects.Num();
    }

    if (bIncludePublic)
    {
        Count += PublicStatusEffects.Num();
    }

    return Count;
}

FNiosReplicatedStatusEffect UNiosCore_StatusEffectComponent::GetHighestPriorityCrowdControlEffect(bool bIncludePersonal, bool bIncludePublic) const
{
    FNiosReplicatedStatusEffect BestEffect;
    bool bHasBest = false;

    TArray<FNiosReplicatedStatusEffect> VisibleEffects;
    GetVisibleStatusEffects(bIncludePersonal, bIncludePublic, VisibleEffects);

    for (const FNiosReplicatedStatusEffect& Effect : VisibleEffects)
    {
        if (!Effect.bHasCrowdControl)
        {
            continue;
        }

        if (!bHasBest || Effect.CrowdControlPriority > BestEffect.CrowdControlPriority)
        {
            BestEffect = Effect;
            bHasBest = true;
        }
    }

    return BestEffect;
}

void UNiosCore_StatusEffectComponent::ApplyStartupStatusEffects()
{
    if (bStartupEffectsApplied || !GetOwner() || !GetOwner()->HasAuthority() || !AbilitySystemComponent)
    {
        return;
    }

    bStartupEffectsApplied = true;

    for (const FName EffectID : StartupStatusEffectIDs)
    {
        ApplyStatusEffectByID(EffectID, GetOwner(), ENiosStatusEffectSourceType::Passive, EffectID);
    }

    for (UNiosCore_StatusEffectDataAsset* Effect : LegacyStartupStatusEffects)
    {
        ApplyStatusEffect(Effect, GetOwner(), ENiosStatusEffectSourceType::Passive, NAME_None);
    }
}

void UNiosCore_StatusEffectComponent::ApplyInstantOperations(const FNiosActiveStatusEffect& ActiveEffect)
{
    if (!ActiveEffect.Effect || !AbilitySystemComponent)
    {
        return;
    }

    for (int32 OperationIndex = 0; OperationIndex < ActiveEffect.Effect->Operations.Num(); ++OperationIndex)
    {
        const FNiosStatusEffectOperation& Operation = ActiveEffect.Effect->Operations[OperationIndex];
        const float RuntimeValue = GetRuntimeOperationValue(ActiveEffect, OperationIndex);
        if (Operation.Type == ENiosStatusEffectOperationType::PeriodicAttributeDelta && Operation.Attribute.IsValid() && !FMath::IsNearlyZero(RuntimeValue))
        {
            AbilitySystemComponent->ApplyModToAttribute(Operation.Attribute, EGameplayModOp::Additive, RuntimeValue * ActiveEffect.StackCount);
            FNiosCoreSkillEffectProcessor::ClampResourceAttribute(AbilitySystemComponent, Operation.Attribute);
        }
    }
}

void UNiosCore_StatusEffectComponent::ApplyPersistentOperations(FNiosActiveStatusEffect& ActiveEffect)
{
    if (!ActiveEffect.Effect || !AbilitySystemComponent)
    {
        return;
    }

    if (!ActiveEffect.Effect->GrantedTags.IsEmpty())
    {
        AbilitySystemComponent->AddLooseGameplayTags(ActiveEffect.Effect->GrantedTags, ActiveEffect.StackCount);
        ActiveEffect.AppliedGrantedTags.AppendTags(ActiveEffect.Effect->GrantedTags);
    }

    for (int32 OperationIndex = 0; OperationIndex < ActiveEffect.Effect->Operations.Num(); ++OperationIndex)
    {
        const FNiosStatusEffectOperation& Operation = ActiveEffect.Effect->Operations[OperationIndex];
        const float RuntimeValue = GetRuntimeOperationValue(ActiveEffect, OperationIndex);

        switch (Operation.Type)
        {
        case ENiosStatusEffectOperationType::AttributeModifier:
            if (StatsComponent && Operation.StatType != ENiosStatType::None && !FMath::IsNearlyZero(RuntimeValue))
            {
                FNiosStatModifierSource Source;
                Source.SourceID = MakeRuntimeSourceID(ActiveEffect, Operation);
                Source.Layer = Operation.ModifierLayer;
                Source.Modifiers.Add({ Operation.StatType, RuntimeValue * ActiveEffect.StackCount });
                StatsComponent->AddOrReplaceModifierSource(Source);
                ActiveEffect.AppliedStatSourceIDs.Add(Source.SourceID);
            }
            break;

        case ENiosStatusEffectOperationType::GrantTag:
        case ENiosStatusEffectOperationType::CrowdControl:
            if (!Operation.Tags.IsEmpty())
            {
                AbilitySystemComponent->AddLooseGameplayTags(Operation.Tags, ActiveEffect.StackCount);
                ActiveEffect.AppliedGrantedTags.AppendTags(Operation.Tags);
            }
            break;

        case ENiosStatusEffectOperationType::RemoveTag:
            if (!Operation.Tags.IsEmpty())
            {
                AbilitySystemComponent->RemoveLooseGameplayTags(Operation.Tags, ActiveEffect.StackCount);
                ActiveEffect.AppliedRemovedTags.AppendTags(Operation.Tags);
            }
            break;

        case ENiosStatusEffectOperationType::Aura:
        case ENiosStatusEffectOperationType::PeriodicAttributeDelta:
        default:
            break;
        }
    }
}

void UNiosCore_StatusEffectComponent::RemovePersistentOperations(FNiosActiveStatusEffect& ActiveEffect)
{
    CleanupAuraTargets(ActiveEffect);

    if (StatsComponent)
    {
        for (const FName SourceID : ActiveEffect.AppliedStatSourceIDs)
        {
            StatsComponent->RemoveModifierSource(SourceID);
        }
    }

    if (AbilitySystemComponent)
    {
        if (!ActiveEffect.AppliedGrantedTags.IsEmpty())
        {
            AbilitySystemComponent->RemoveLooseGameplayTags(ActiveEffect.AppliedGrantedTags, ActiveEffect.StackCount);
        }

        if (!ActiveEffect.AppliedRemovedTags.IsEmpty())
        {
            AbilitySystemComponent->AddLooseGameplayTags(ActiveEffect.AppliedRemovedTags, ActiveEffect.StackCount);
        }
    }
}

void UNiosCore_StatusEffectComponent::TickPeriodicOperations(FNiosActiveStatusEffect& ActiveEffect)
{
    if (!ActiveEffect.Effect || !AbilitySystemComponent || ActiveEffect.Effect->TickInterval <= 0.f)
    {
        return;
    }

    const UWorld* World = GetWorld();
    const float Now = World ? World->GetTimeSeconds() : 0.f;
    if (ActiveEffect.NextTickTime <= 0.f || Now < ActiveEffect.NextTickTime)
    {
        return;
    }

    for (int32 OperationIndex = 0; OperationIndex < ActiveEffect.Effect->Operations.Num(); ++OperationIndex)
    {
        const FNiosStatusEffectOperation& Operation = ActiveEffect.Effect->Operations[OperationIndex];
        const float RuntimeValue = GetRuntimeOperationValue(ActiveEffect, OperationIndex);
        if (Operation.Type == ENiosStatusEffectOperationType::PeriodicAttributeDelta && Operation.Attribute.IsValid() && !FMath::IsNearlyZero(RuntimeValue))
        {
            AbilitySystemComponent->ApplyModToAttribute(Operation.Attribute, EGameplayModOp::Additive, RuntimeValue * ActiveEffect.StackCount);
            FNiosCoreSkillEffectProcessor::ClampResourceAttribute(AbilitySystemComponent, Operation.Attribute);
        }
    }

    ActiveEffect.NextTickTime = Now + ActiveEffect.Effect->TickInterval;
}


void UNiosCore_StatusEffectComponent::TickAuraOperations(FNiosActiveStatusEffect& ActiveEffect)
{
    if (!ActiveEffect.Effect || !AbilitySystemComponent)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    AActor* AuraOriginActor = ResolveAuraOriginActor();
    if (!AuraOriginActor)
    {
        return;
    }

    const float Now = World->GetTimeSeconds();

    for (int32 OperationIndex = 0; OperationIndex < ActiveEffect.Effect->Operations.Num(); ++OperationIndex)
    {
        const FNiosStatusEffectOperation& Operation = ActiveEffect.Effect->Operations[OperationIndex];
        if (Operation.Type != ENiosStatusEffectOperationType::Aura || Operation.GrantedStatusEffectID.IsNone() || Operation.AuraRadius <= 0.f)
        {
            continue;
        }

        if (!ActiveEffect.RuntimeOperationNextTimes.IsValidIndex(OperationIndex))
        {
            ActiveEffect.RuntimeOperationNextTimes.SetNumZeroed(ActiveEffect.Effect->Operations.Num());
            ActiveEffect.RuntimeOperationNextTimes[OperationIndex] = Now;
        }

        if (Now < ActiveEffect.RuntimeOperationNextTimes[OperationIndex])
        {
            continue;
        }

        ActiveEffect.RuntimeOperationNextTimes[OperationIndex] = Now + FMath::Max(0.05f, Operation.AuraScanInterval);

        const FVector OriginLocation = AuraOriginActor->GetActorLocation();
        TArray<FOverlapResult> Overlaps;
        FCollisionObjectQueryParams ObjectQueryParams;
        ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
        ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
        ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(NiosStatusEffectAura), false, AuraOriginActor);
        World->OverlapMultiByObjectType(
            Overlaps,
            OriginLocation,
            FQuat::Identity,
            ObjectQueryParams,
            FCollisionShape::MakeSphere(Operation.AuraRadius),
            QueryParams);

        TSet<UNiosCore_StatusEffectComponent*> CurrentTargets;
        int32 AppliedThisScan = 0;
        const int32 MaxTargets = FMath::Max(0, Operation.AuraMaxTargets);
        const FName AuraSourceID = MakeAuraSourceID(ActiveEffect, Operation, OperationIndex);

        FNiosStatusEffectCatalogEntry AuraChildEntry;
        const bool bResolvedAuraChild = ResolveCatalogEntry(Operation.GrantedStatusEffectID, AuraChildEntry) && AuraChildEntry.EffectData;
        const bool bAuraChildIsInstant = bResolvedAuraChild && AuraChildEntry.EffectData->DurationPolicy == ENiosStatusEffectDurationPolicy::Instant;
        if (!bResolvedAuraChild)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT AURA SKIPPED: Missing child EffectID=%s Parent=%s Owner=%s"),
                *Operation.GrantedStatusEffectID.ToString(),
                *ActiveEffect.EffectID.ToString(),
                *GetNameSafe(GetOwner()));
            continue;
        }

        if (!ValidateAuraGrantedEffect(ActiveEffect, Operation, AuraChildEntry))
        {
            continue;
        }

        // The aura owner can be a valid target too, even when its capsule did not appear in overlap results.
        if (UNiosCore_StatusEffectComponent* OwnerStatusComponent = ResolveStatusEffectComponentFromActor(AuraOriginActor))
        {
            if (IsActorAllowedForAura(AuraOriginActor, AuraOriginActor, Operation))
            {
                CurrentTargets.Add(OwnerStatusComponent);
                if (!HasAuraAppliedToTarget(ActiveEffect, OwnerStatusComponent, AuraSourceID))
                {
                    OwnerStatusComponent->ApplyStatusEffectByID(Operation.GrantedStatusEffectID, AuraOriginActor, ENiosStatusEffectSourceType::Aura, AuraSourceID);
                    if (!bAuraChildIsInstant)
                    {
                        FNiosActiveAuraTarget AppliedTarget;
                        AppliedTarget.TargetComponent = OwnerStatusComponent;
                        AppliedTarget.GrantedStatusEffectID = Operation.GrantedStatusEffectID;
                        AppliedTarget.AuraSourceID = AuraSourceID;
                        ActiveEffect.ActiveAuraTargets.Add(AppliedTarget);
                    }
                    ++AppliedThisScan;
                }
            }
        }

        for (const FOverlapResult& Overlap : Overlaps)
        {
            if (MaxTargets > 0 && AppliedThisScan >= MaxTargets)
            {
                break;
            }

            AActor* CandidateActor = Overlap.GetActor();
            if (!CandidateActor || !IsActorAllowedForAura(AuraOriginActor, CandidateActor, Operation))
            {
                continue;
            }

            UNiosCore_StatusEffectComponent* TargetComponent = ResolveStatusEffectComponentFromActor(CandidateActor);
            if (!TargetComponent)
            {
                continue;
            }

            CurrentTargets.Add(TargetComponent);
            if (HasAuraAppliedToTarget(ActiveEffect, TargetComponent, AuraSourceID))
            {
                continue;
            }

            TargetComponent->ApplyStatusEffectByID(Operation.GrantedStatusEffectID, AuraOriginActor, ENiosStatusEffectSourceType::Aura, AuraSourceID);

            if (!bAuraChildIsInstant)
            {
                FNiosActiveAuraTarget AppliedTarget;
                AppliedTarget.TargetComponent = TargetComponent;
                AppliedTarget.GrantedStatusEffectID = Operation.GrantedStatusEffectID;
                AppliedTarget.AuraSourceID = AuraSourceID;
                ActiveEffect.ActiveAuraTargets.Add(AppliedTarget);
            }
            ++AppliedThisScan;
        }

        if (Operation.bAuraRemoveOnExit)
        {
            for (int32 TargetIndex = ActiveEffect.ActiveAuraTargets.Num() - 1; TargetIndex >= 0; --TargetIndex)
            {
                FNiosActiveAuraTarget& AppliedTarget = ActiveEffect.ActiveAuraTargets[TargetIndex];
                if (AppliedTarget.AuraSourceID != AuraSourceID)
                {
                    continue;
                }

                UNiosCore_StatusEffectComponent* TargetComponent = AppliedTarget.TargetComponent.Get();
                if (!TargetComponent || !CurrentTargets.Contains(TargetComponent))
                {
                    if (TargetComponent)
                    {
                        TargetComponent->RemoveStatusEffectsBySource(ENiosStatusEffectSourceType::Aura, AuraSourceID);
                    }
                    ActiveEffect.ActiveAuraTargets.RemoveAt(TargetIndex);
                }
            }
        }
    }
}

void UNiosCore_StatusEffectComponent::CleanupAuraTargets(FNiosActiveStatusEffect& ActiveEffect)
{
    for (FNiosActiveAuraTarget& AppliedTarget : ActiveEffect.ActiveAuraTargets)
    {
        if (UNiosCore_StatusEffectComponent* TargetComponent = AppliedTarget.TargetComponent.Get())
        {
            TargetComponent->RemoveStatusEffectsBySource(ENiosStatusEffectSourceType::Aura, AppliedTarget.AuraSourceID);
        }
    }

    ActiveEffect.ActiveAuraTargets.Reset();
}

void UNiosCore_StatusEffectComponent::InitializeOperationRuntimeState(FNiosActiveStatusEffect& ActiveEffect, float Now)
{
    ActiveEffect.RuntimeOperationNextTimes.Reset();

    if (!ActiveEffect.Effect)
    {
        return;
    }

    ActiveEffect.RuntimeOperationNextTimes.SetNumZeroed(ActiveEffect.Effect->Operations.Num());
    for (int32 OperationIndex = 0; OperationIndex < ActiveEffect.Effect->Operations.Num(); ++OperationIndex)
    {
        const FNiosStatusEffectOperation& Operation = ActiveEffect.Effect->Operations[OperationIndex];
        if (Operation.Type == ENiosStatusEffectOperationType::Aura)
        {
            // First scan runs on the next component tick, not through a separate timer.
            ActiveEffect.RuntimeOperationNextTimes[OperationIndex] = Now;
        }
    }
}

AActor* UNiosCore_StatusEffectComponent::ResolveAuraOriginActor() const
{
    if (AbilitySystemComponent)
    {
        if (AActor* AvatarActor = AbilitySystemComponent->GetAvatarActor())
        {
            return AvatarActor;
        }

        if (AActor* OwnerActor = AbilitySystemComponent->GetOwnerActor())
        {
            return OwnerActor;
        }
    }

    AActor* OwnerActor = GetOwner();
    if (APlayerState* PlayerState = Cast<APlayerState>(OwnerActor))
    {
        return PlayerState->GetPawn();
    }

    return OwnerActor;
}

UNiosCore_StatusEffectComponent* UNiosCore_StatusEffectComponent::ResolveStatusEffectComponentFromActor(AActor* Actor) const
{
    if (!Actor)
    {
        return nullptr;
    }

    if (UNiosCore_StatusEffectComponent* Component = Actor->FindComponentByClass<UNiosCore_StatusEffectComponent>())
    {
        return Component;
    }

    if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor))
    {
        if (AActor* OwnerActor = ASC->GetOwnerActor())
        {
            if (UNiosCore_StatusEffectComponent* Component = OwnerActor->FindComponentByClass<UNiosCore_StatusEffectComponent>())
            {
                return Component;
            }
        }

        if (AActor* AvatarActor = ASC->GetAvatarActor())
        {
            if (UNiosCore_StatusEffectComponent* Component = AvatarActor->FindComponentByClass<UNiosCore_StatusEffectComponent>())
            {
                return Component;
            }

            if (const APawn* Pawn = Cast<APawn>(AvatarActor))
            {
                if (APlayerState* PlayerState = Pawn->GetPlayerState())
                {
                    return PlayerState->FindComponentByClass<UNiosCore_StatusEffectComponent>();
                }
            }
        }
    }

    return nullptr;
}

bool UNiosCore_StatusEffectComponent::ValidateAuraGrantedEffect(const FNiosActiveStatusEffect& ActiveEffect, const FNiosStatusEffectOperation& Operation, const FNiosStatusEffectCatalogEntry& GrantedEntry) const
{
    if (Operation.GrantedStatusEffectID.IsNone() || !GrantedEntry.EffectData)
    {
        return false;
    }

    if (!ActiveEffect.EffectID.IsNone() && Operation.GrantedStatusEffectID == ActiveEffect.EffectID)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT AURA LOOP BLOCKED: Parent=%s cannot grant itself. Owner=%s"),
            *ActiveEffect.EffectID.ToString(),
            *GetNameSafe(GetOwner()));
        return false;
    }

    if (DoesEffectContainAuraOperation(GrantedEntry.EffectData))
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT AURA LOOP BLOCKED: Parent=%s tried to grant aura/producer effect %s. Aura -> Aura is disabled by default. Owner=%s"),
            *ActiveEffect.EffectID.ToString(),
            *Operation.GrantedStatusEffectID.ToString(),
            *GetNameSafe(GetOwner()));
        return false;
    }

    return true;
}

bool UNiosCore_StatusEffectComponent::DoesEffectContainAuraOperation(const UNiosCore_StatusEffectDataAsset* Effect) const
{
    if (!Effect)
    {
        return false;
    }

    for (const FNiosStatusEffectOperation& Operation : Effect->Operations)
    {
        if (Operation.Type == ENiosStatusEffectOperationType::Aura)
        {
            return true;
        }
    }

    return false;
}

bool UNiosCore_StatusEffectComponent::IsActorAllowedForAura(AActor* AuraOriginActor, AActor* CandidateActor, const FNiosStatusEffectOperation& Operation) const
{
    if (!AuraOriginActor || !CandidateActor)
    {
        return false;
    }

    if (Operation.AuraTargetPolicy == ENiosCoreTargetApplicationPolicy::AnyIncludingNeutral)
    {
        return true;
    }

    const bool bIsSource = AuraOriginActor == CandidateActor;
    if (Operation.AuraTargetPolicy == ENiosCoreTargetApplicationPolicy::SourceOnly)
    {
        return bIsSource;
    }

    UWorld* World = GetWorld();
    const UNiosTargetRulesSubsystem* Rules = World ? World->GetSubsystem<UNiosTargetRulesSubsystem>() : nullptr;
    if (!Rules)
    {
        return false;
    }

    switch (Operation.AuraTargetPolicy)
    {
    case ENiosCoreTargetApplicationPolicy::EnemiesOnly:
        return Rules->CanHarm(AuraOriginActor, CandidateActor);

    case ENiosCoreTargetApplicationPolicy::FriendliesOnly:
        return Rules->CanHelp(AuraOriginActor, CandidateActor);

    case ENiosCoreTargetApplicationPolicy::NonNeutralOnly:
        return Rules->GetRelation(AuraOriginActor, CandidateActor) != ENiosCoreTargetRelation::Neutral;

    case ENiosCoreTargetApplicationPolicy::AutoFromEffectType:
        // Aura defaults to friendly/self, because most passive auras are buffs. Explicitly pick EnemiesOnly for debuff auras.
        return Rules->CanHelp(AuraOriginActor, CandidateActor);

    case ENiosCoreTargetApplicationPolicy::SourceOnly:
    case ENiosCoreTargetApplicationPolicy::AnyIncludingNeutral:
    default:
        return false;
    }
}

FName UNiosCore_StatusEffectComponent::MakeAuraSourceID(const FNiosActiveStatusEffect& ActiveEffect, const FNiosStatusEffectOperation& Operation, int32 OperationIndex) const
{
    const FString EffectName = ActiveEffect.EffectID.IsNone() ? TEXT("StatusEffect") : ActiveEffect.EffectID.ToString();
    const FString GrantedEffectName = Operation.GrantedStatusEffectID.IsNone() ? TEXT("None") : Operation.GrantedStatusEffectID.ToString();
    return FName(*FString::Printf(TEXT("Aura_%s_%s_Op%d_Grants_%s"), *EffectName, *ActiveEffect.RuntimeID.ToString(EGuidFormats::Digits), OperationIndex, *GrantedEffectName));
}

bool UNiosCore_StatusEffectComponent::HasAuraAppliedToTarget(const FNiosActiveStatusEffect& ActiveEffect, UNiosCore_StatusEffectComponent* TargetComponent, FName AuraSourceID) const
{
    if (!TargetComponent)
    {
        return false;
    }

    for (const FNiosActiveAuraTarget& AppliedTarget : ActiveEffect.ActiveAuraTargets)
    {
        if (AppliedTarget.TargetComponent.Get() == TargetComponent && AppliedTarget.AuraSourceID == AuraSourceID)
        {
            return true;
        }
    }

    return false;
}

void UNiosCore_StatusEffectComponent::RemoveExpiredEffects(float Now)
{
    bool bChanged = false;

    for (int32 Index = ActiveEffects.Num() - 1; Index >= 0; --Index)
    {
        FNiosActiveStatusEffect& ActiveEffect = ActiveEffects[Index];
        if (ActiveEffect.Effect && ActiveEffect.Effect->DurationPolicy == ENiosStatusEffectDurationPolicy::Duration && ActiveEffect.EndTime > 0.f && Now >= ActiveEffect.EndTime)
        {
            RemovePersistentOperations(ActiveEffect);
            ActiveEffects.RemoveAt(Index);
            bChanged = true;
        }
    }

    if (bChanged)
    {
        RebuildReplicatedStatusEffects();
        OnStatusEffectsChanged.Broadcast();
    }
}

void UNiosCore_StatusEffectComponent::RefreshDuration(FNiosActiveStatusEffect& ActiveEffect, float Now) const
{
    if (!ActiveEffect.Effect)
    {
        return;
    }

    ActiveEffect.StartTime = Now;
    ActiveEffect.EndTime = ActiveEffect.Effect->DurationPolicy == ENiosStatusEffectDurationPolicy::Duration
        ? Now + FMath::Max(0.f, ActiveEffect.Effect->Duration)
        : 0.f;
}

void UNiosCore_StatusEffectComponent::BuildRuntimeOperationValues(FNiosActiveStatusEffect& ActiveEffect) const
{
    ActiveEffect.RuntimeOperationValues.Reset();

    if (!ActiveEffect.Effect)
    {
        return;
    }

    UAbilitySystemComponent* SourceASC = ResolveSourceAbilitySystemComponent(ActiveEffect);

    for (const FNiosStatusEffectOperation& Operation : ActiveEffect.Effect->Operations)
    {
        float RuntimeValue = Operation.Value;

        if (SourceASC)
        {
            for (const FNiosStatusEffectStatScaling& Scaling : Operation.Scalings)
            {
                if (!Scaling.SourceAttribute.IsValid() || FMath::IsNearlyZero(Scaling.Scale))
                {
                    continue;
                }

                RuntimeValue += SourceASC->GetNumericAttribute(Scaling.SourceAttribute) * Scaling.Scale;
            }
        }

        ActiveEffect.RuntimeOperationValues.Add(RuntimeValue);
    }
}

float UNiosCore_StatusEffectComponent::GetRuntimeOperationValue(const FNiosActiveStatusEffect& ActiveEffect, int32 OperationIndex) const
{
    if (ActiveEffect.RuntimeOperationValues.IsValidIndex(OperationIndex))
    {
        return ActiveEffect.RuntimeOperationValues[OperationIndex];
    }

    if (ActiveEffect.Effect && ActiveEffect.Effect->Operations.IsValidIndex(OperationIndex))
    {
        return ActiveEffect.Effect->Operations[OperationIndex].Value;
    }

    return 0.f;
}

UAbilitySystemComponent* UNiosCore_StatusEffectComponent::ResolveSourceAbilitySystemComponent(const FNiosActiveStatusEffect& ActiveEffect) const
{
    if (AActor* SourceActor = ActiveEffect.SourceActor.Get())
    {
        return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);
    }

    return nullptr;
}

FNiosActiveStatusEffect* UNiosCore_StatusEffectComponent::FindStackableEffect(FName EffectID, UNiosCore_StatusEffectDataAsset* Effect, ENiosStatusEffectSourceType SourceType, FName SourceID)
{
    for (FNiosActiveStatusEffect& ActiveEffect : ActiveEffects)
    {
        const bool bSameEffect = !EffectID.IsNone() ? ActiveEffect.EffectID == EffectID : ActiveEffect.Effect == Effect;
        if (bSameEffect && ActiveEffect.SourceType == SourceType && ActiveEffect.SourceID == SourceID)
        {
            return &ActiveEffect;
        }
    }

    return nullptr;
}

void UNiosCore_StatusEffectComponent::RebuildReplicatedStatusEffects()
{
    PublicStatusEffects.Reset();
    PersonalStatusEffects.Reset();

    for (const FNiosActiveStatusEffect& ActiveEffect : ActiveEffects)
    {
        if (!ActiveEffect.Effect)
        {
            continue;
        }

        if (ActiveEffect.Effect->Visibility == ENiosStatusEffectVisibility::Public)
        {
            PublicStatusEffects.Add(MakeReplicatedEffect(ActiveEffect));
        }
        else if (ActiveEffect.Effect->Visibility == ENiosStatusEffectVisibility::Personal)
        {
            PersonalStatusEffects.Add(MakeReplicatedEffect(ActiveEffect));
        }
    }

    OnPublicStatusEffectsChanged.Broadcast(PublicStatusEffects);
    OnPersonalStatusEffectsChanged.Broadcast(PersonalStatusEffects);
    BroadcastActiveCrowdControlChanged();
}

FNiosReplicatedStatusEffect UNiosCore_StatusEffectComponent::MakeReplicatedEffect(const FNiosActiveStatusEffect& ActiveEffect) const
{
    FNiosReplicatedStatusEffect Rep;
    if (!ActiveEffect.Effect)
    {
        return Rep;
    }

    Rep.EffectID = ActiveEffect.EffectID;
    Rep.DisplayName = ActiveEffect.DisplayName;
    Rep.Description = ActiveEffect.Description;
    Rep.Icon = ActiveEffect.Icon;
    Rep.Visibility = ActiveEffect.Effect->Visibility;
    Rep.StackCount = ActiveEffect.StackCount;
    Rep.StartTime = ActiveEffect.StartTime;
    Rep.EndTime = ActiveEffect.EndTime;
    Rep.Tags = ActiveEffect.Effect->GrantedTags;

    for (const FNiosStatusEffectOperation& Operation : ActiveEffect.Effect->Operations)
    {
        Rep.Tags.AppendTags(Operation.Tags);

        if (Operation.Type == ENiosStatusEffectOperationType::CrowdControl && Operation.CrowdControlType != ENiosCrowdControlType::None)
        {
            if (!Rep.bHasCrowdControl || Operation.CrowdControlPriority > Rep.CrowdControlPriority)
            {
                Rep.bHasCrowdControl = true;
                Rep.CrowdControlType = Operation.CrowdControlType;
                Rep.CrowdControlPriority = Operation.CrowdControlPriority;
                Rep.CrowdControlPresentationID = Operation.CrowdControlPresentationID;
            }
        }
    }

    return Rep;
}

void UNiosCore_StatusEffectComponent::BroadcastActiveCrowdControlChanged()
{
    OnActiveCrowdControlChanged.Broadcast(GetHighestPriorityCrowdControlEffect(true, true));
}

FName UNiosCore_StatusEffectComponent::MakeRuntimeSourceID(const FNiosActiveStatusEffect& ActiveEffect, const FNiosStatusEffectOperation& Operation) const
{
    const FString EffectName = ActiveEffect.EffectID.IsNone() ? TEXT("StatusEffect") : ActiveEffect.EffectID.ToString();
    const FString OperationName = StaticEnum<ENiosStatusEffectOperationType>()->GetNameStringByValue(static_cast<int64>(Operation.Type));
    return FName(*FString::Printf(TEXT("SE_%s_%s_%s"), *EffectName, *ActiveEffect.RuntimeID.ToString(EGuidFormats::Digits), *OperationName));
}

bool UNiosCore_StatusEffectComponent::ResolveCatalogEntry(FName EffectID, FNiosStatusEffectCatalogEntry& OutEntry) const
{
    OutEntry = FNiosStatusEffectCatalogEntry();
    return UNiosCore_StatusEffectCatalogLibrary::ResolveStatusEffectByID(this, EffectID, OutEntry);
}

void UNiosCore_StatusEffectComponent::OnRep_PublicStatusEffects()
{
    OnPublicStatusEffectsChanged.Broadcast(PublicStatusEffects);
    OnStatusEffectsChanged.Broadcast();
    BroadcastActiveCrowdControlChanged();
}

void UNiosCore_StatusEffectComponent::OnRep_PersonalStatusEffects()
{
    OnPersonalStatusEffectsChanged.Broadcast(PersonalStatusEffects);
    OnStatusEffectsChanged.Broadcast();
    BroadcastActiveCrowdControlChanged();
}

#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"

#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Components/Feedback/NiosCore_ActionFeedbackComponent.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Components/Targeting/NiosCore_TargetFeedbackComponent.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

namespace
{
    template <typename TObjectType>
    void Nios_AddSoftObjectPathIfValid(
        TArray<FSoftObjectPath>& OutPaths,
        const TSoftObjectPtr<TObjectType>& SoftObject)
    {
        if (SoftObject.IsNull())
            return;

        const FSoftObjectPath Path = SoftObject.ToSoftObjectPath();

        if (Path.IsValid())
        {
            OutPaths.AddUnique(Path);
        }
    }

    bool Nios_AreSkillAssetsResolved(
        const UNiosCore_SkillDataAsset* Skill,
        const FNiosLoadedSkillAssets& Assets)
    {
        if (!Skill)
            return true;

        const bool bCastLoopMontageReady =
            Skill->CastLoopMontage.IsNull() || Assets.CastLoopMontage != nullptr;

        const bool bExecuteMontageReady =
            Skill->ExecuteMontage.IsNull() || Assets.ExecuteMontage != nullptr;

        const bool bCastLoopSoundReady =
            Skill->CastLoopSound.IsNull() || Assets.CastLoopSound != nullptr;

        const bool bExecuteSoundReady =
            Skill->ExecuteSound.IsNull() || Assets.ExecuteSound != nullptr;

        return bCastLoopMontageReady
            && bExecuteMontageReady
            && bCastLoopSoundReady
            && bExecuteSoundReady;
    }

    APlayerController* Nios_ASCResolveOwningPlayerController(const UNiosCore_AbilitySystemComponent* ASC)
    {
        if (!ASC)
        {
            return nullptr;
        }

        auto TryActor = [](AActor* Actor) -> APlayerController*
        {
            if (!Actor)
            {
                return nullptr;
            }

            if (APlayerController* PC = Cast<APlayerController>(Actor))
            {
                return PC;
            }

            if (APawn* Pawn = Cast<APawn>(Actor))
            {
                return Cast<APlayerController>(Pawn->GetController());
            }

            if (APlayerState* PlayerState = Cast<APlayerState>(Actor))
            {
                return PlayerState->GetPlayerController();
            }

            return nullptr;
        };

        if (APlayerController* PC = TryActor(ASC->GetAvatarActor()))
        {
            return PC;
        }

        if (APlayerController* PC = TryActor(ASC->GetOwnerActor()))
        {
            return PC;
        }

        for (AActor* Owner = ASC->GetOwner(); Owner; Owner = Owner->GetOwner())
        {
            if (APlayerController* PC = TryActor(Owner))
            {
                return PC;
            }
        }

        return nullptr;
    }

    bool Nios_GetMaxAttributeForResourceAttribute(const FGameplayAttribute& Attribute, FGameplayAttribute& OutMaxAttribute)
    {
        if (Attribute == UNiosCore_AttributeSet::GetHealthAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxHealthAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetManaAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxManaAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetEnergyAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxEnergyAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetRageAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxRageAttribute();
            return true;
        }

        return false;
    }

    bool Nios_GetResourceAttributeForMaxAttribute(const FGameplayAttribute& Attribute, FGameplayAttribute& OutResourceAttribute)
    {
        if (Attribute == UNiosCore_AttributeSet::GetMaxHealthAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetHealthAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetMaxManaAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetManaAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetMaxEnergyAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetEnergyAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetMaxRageAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetRageAttribute();
            return true;
        }

        return false;
    }

    bool Nios_GetResourceStatForAttribute(const FGameplayAttribute& Attribute, ENiosStatType& OutResourceStat)
    {
        if (Attribute == UNiosCore_AttributeSet::GetHealthAttribute())
        {
            OutResourceStat = ENiosStatType::Health;
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetManaAttribute())
        {
            OutResourceStat = ENiosStatType::Mana;
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetEnergyAttribute())
        {
            OutResourceStat = ENiosStatType::Energy;
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetRageAttribute())
        {
            OutResourceStat = ENiosStatType::Rage;
            return true;
        }

        OutResourceStat = ENiosStatType::None;
        return false;
    }

    UNiosCore_StatsComponent* Nios_ResolveStatsComponentForASC(UAbilitySystemComponent* ASC)
    {
        if (!ASC)
        {
            return nullptr;
        }

        if (AActor* AvatarActor = ASC->GetAvatarActor())
        {
            if (UNiosCore_StatsComponent* Stats = AvatarActor->FindComponentByClass<UNiosCore_StatsComponent>())
            {
                return Stats;
            }

            if (const APawn* Pawn = Cast<APawn>(AvatarActor))
            {
                if (APlayerState* PlayerState = Pawn->GetPlayerState())
                {
                    if (UNiosCore_StatsComponent* Stats = PlayerState->FindComponentByClass<UNiosCore_StatsComponent>())
                    {
                        return Stats;
                    }
                }
            }
        }

        if (AActor* OwnerActor = ASC->GetOwnerActor())
        {
            if (UNiosCore_StatsComponent* Stats = OwnerActor->FindComponentByClass<UNiosCore_StatsComponent>())
            {
                return Stats;
            }
        }

        if (AActor* Owner = ASC->GetOwner())
        {
            return Owner->FindComponentByClass<UNiosCore_StatsComponent>();
        }

        return nullptr;
    }

    void Nios_SyncCurrentResourceRuntimeSource(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float NewValue)
    {
        ENiosStatType ResourceStat = ENiosStatType::None;
        if (!Nios_GetResourceStatForAttribute(Attribute, ResourceStat))
        {
            return;
        }

        if (UNiosCore_StatsComponent* Stats = Nios_ResolveStatsComponentForASC(ASC))
        {
            Stats->SetCurrentResourceRuntimeValue(ResourceStat, NewValue, false);
        }
    }

    AActor* Nios_ResolveLifecycleActorForASC(UAbilitySystemComponent* ASC)
    {
        if (!ASC)
        {
            return nullptr;
        }

        AActor* ResolvedAvatarActor = ASC->GetAvatarActor();
        if (ResolvedAvatarActor)
        {
            return ResolvedAvatarActor;
        }

        AActor* ResolvedOwnerActor = ASC->GetOwnerActor();
        if (APlayerState* PlayerStateOwner = Cast<APlayerState>(ResolvedOwnerActor))
        {
            if (const AController* OwningController = Cast<AController>(PlayerStateOwner->GetOwner()))
            {
                if (APawn* ControlledPawn = OwningController->GetPawn())
                {
                    return ControlledPawn;
                }
            }
        }

        return ResolvedOwnerActor;
    }

    void Nios_UpdateDeathLifecycleAfterHealthWrite(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float OldValue, float NewValue)
    {
        if (!ASC || Attribute != UNiosCore_AttributeSet::GetHealthAttribute())
        {
            return;
        }

        AActor* LifecycleActor = Nios_ResolveLifecycleActorForASC(ASC);
        if (!LifecycleActor)
        {
            return;
        }

        if (NewValue <= KINDA_SMALL_NUMBER)
        {
            if (LifecycleActor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
            {
                INiosGASActorInterface::Execute_HandleDeath(LifecycleActor, nullptr);
            }
            return;
        }

        // A resurrection/heal pipeline may legitimately raise Health from zero. In that case the server-side
        // dead flag must be cleared in one place instead of leaving an actor with Health > 0 but IsAlive=false.
        if (OldValue <= KINDA_SMALL_NUMBER)
        {
            static const FName HandleReviveName(TEXT("HandleRevive"));
            if (UFunction* ReviveFunction = LifecycleActor->FindFunction(HandleReviveName))
            {
                LifecycleActor->ProcessEvent(ReviveFunction, nullptr);
            }
        }
    }

    float Nios_ClampAttributeValueForWrite(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, float NewValue)
    {
        if (!ASC || !Attribute.IsValid())
        {
            return NewValue;
        }

        FGameplayAttribute MaxAttribute;
        if (Nios_GetMaxAttributeForResourceAttribute(Attribute, MaxAttribute))
        {
            const float MaxValue = FMath::Max(0.f, ASC->GetNumericAttribute(MaxAttribute));
            return FMath::Clamp(NewValue, 0.f, MaxValue);
        }

        FGameplayAttribute ResourceAttribute;
        if (Nios_GetResourceAttributeForMaxAttribute(Attribute, ResourceAttribute))
        {
            return FMath::Max(0.f, NewValue);
        }

        return NewValue;
    }

    void Nios_ClampPairedResourceAfterWrite(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute)
    {
        if (!ASC || !Attribute.IsValid())
        {
            return;
        }

        auto ClampResourceToMax = [ASC](const FGameplayAttribute& ResourceAttribute, const FGameplayAttribute& MaxAttribute)
        {
            const float CurrentValue = ASC->GetNumericAttribute(ResourceAttribute);
            const float MaxValue = FMath::Max(0.f, ASC->GetNumericAttribute(MaxAttribute));
            const float ClampedValue = FMath::Clamp(CurrentValue, 0.f, MaxValue);

            if (!FMath::IsNearlyEqual(CurrentValue, ClampedValue))
            {
                ASC->SetNumericAttributeBase(ResourceAttribute, ClampedValue);
                Nios_SyncCurrentResourceRuntimeSource(ASC, ResourceAttribute, ClampedValue);
                Nios_UpdateDeathLifecycleAfterHealthWrite(ASC, ResourceAttribute, CurrentValue, ClampedValue);

                UE_LOG(LogTemp, Warning, TEXT(">>> ASC RESOURCE CLAMPED AFTER WRITE: Resource=%s Before=%.2f After=%.2f Max=%.2f ASC=%s"),
                    *ResourceAttribute.GetName(),
                    CurrentValue,
                    ClampedValue,
                    MaxValue,
                    *GetNameSafe(ASC));
            }
        };

        FGameplayAttribute MaxAttribute;
        if (Nios_GetMaxAttributeForResourceAttribute(Attribute, MaxAttribute))
        {
            ClampResourceToMax(Attribute, MaxAttribute);
            return;
        }

        FGameplayAttribute ResourceAttribute;
        if (Nios_GetResourceAttributeForMaxAttribute(Attribute, ResourceAttribute))
        {
            ClampResourceToMax(ResourceAttribute, Attribute);
        }
    }
}

UNiosCore_AbilitySystemComponent::UNiosCore_AbilitySystemComponent()
{
    SetIsReplicatedByDefault(true);
}

void UNiosCore_AbilitySystemComponent::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UNiosCore_AbilitySystemComponent, SelectedTarget);
    DOREPLIFETIME(UNiosCore_AbilitySystemComponent, bIsCasting);
    DOREPLIFETIME(UNiosCore_AbilitySystemComponent, CurrentCastSkill);
    DOREPLIFETIME(UNiosCore_AbilitySystemComponent, CastLockedTarget);
    DOREPLIFETIME(UNiosCore_AbilitySystemComponent, CastStartTime);
    DOREPLIFETIME(UNiosCore_AbilitySystemComponent, CastEndTime);
    DOREPLIFETIME(UNiosCore_AbilitySystemComponent, GrantedSkills);
}

void UNiosCore_AbilitySystemComponent::InitAvatar(AActor* Pawn)
{
    InitOwnerAvatar(Pawn, Pawn, nullptr);
}

void UNiosCore_AbilitySystemComponent::InitOwnerAvatar(AActor* InOwnerActor, AActor* InAvatarActor, UNiosCore_AttributeSet* AttributeSetOverride)
{
    AActor* EffectiveOwner = InOwnerActor ? InOwnerActor : InAvatarActor;
    AActor* EffectiveAvatar = InAvatarActor ? InAvatarActor : EffectiveOwner;

    if (!EffectiveOwner || !EffectiveAvatar)
    {
        return;
    }

    InitAbilityActorInfo(EffectiveOwner, EffectiveAvatar);

    if (AttributeSetOverride)
    {
        StatsAttributeSet = AttributeSetOverride;

        if (!GetSet<UNiosCore_AttributeSet>())
        {
            AddAttributeSetSubobject(StatsAttributeSet.Get());
        }

        ForceReplication();
        return;
    }

    const UNiosCore_AttributeSet* ExistingSet = GetSet<UNiosCore_AttributeSet>();

    if (ExistingSet)
    {
        StatsAttributeSet = const_cast<UNiosCore_AttributeSet*>(ExistingSet);
        return;
    }

    StatsAttributeSet = NewObject<UNiosCore_AttributeSet>(
        EffectiveOwner,
        UNiosCore_AttributeSet::StaticClass(),
        TEXT("StatsAttributeSet")
    );

    AddAttributeSetSubobject(StatsAttributeSet.Get());

    ForceReplication();
}

UNiosCore_AttributeSet* UNiosCore_AbilitySystemComponent::GetStatsAttributeSet() const
{
    return StatsAttributeSet.Get();
}

FGameplayAbilitySpecHandle UNiosCore_AbilitySystemComponent::GiveAbilityWithSkill(
    FName SkillID,
    TSubclassOf<UGameplayAbility> AbilityClass,
    UNiosCore_SkillDataAsset* Skill,
    int32 Level)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return FGameplayAbilitySpecHandle();
    }

    if (SkillID.IsNone() || !AbilityClass || !Skill)
    {
        return FGameplayAbilitySpecHandle();
    }

    PreloadSkillAssets(Skill);

    FGameplayAbilitySpec Spec(
        AbilityClass,
        Level,
        INDEX_NONE,
        Skill
    );

    const FGameplayAbilitySpecHandle Handle = GiveAbility(Spec);

    if (!Handle.IsValid())
    {
        return FGameplayAbilitySpecHandle();
    }

    FNiosCoreGrantedSkill GrantedSkill;
    GrantedSkill.SkillID = SkillID;
    GrantedSkill.SpecHandle = Handle;

    GrantedSkills.Add(GrantedSkill);

    SkillIDToSpecHandle.Add(SkillID, Handle);
    SpecHandleToSkillID.Add(Handle, SkillID);

    OnGrantedSkillsChanged.Broadcast();

    ForceReplication();

    return Handle;
}

void UNiosCore_AbilitySystemComponent::OnRep_GrantedSkills()
{
    RebuildSkillMaps();
    PreloadGrantedSkillAssetsFromSpecs();

    OnGrantedSkillsChanged.Broadcast();
}

void UNiosCore_AbilitySystemComponent::PreloadSkillAssets(UNiosCore_SkillDataAsset* Skill)
{
    if (!Skill)
        return;

    if (FNiosLoadedSkillAssets* ExistingAssets = LoadedSkillAssets.Find(Skill))
    {
        if (Nios_AreSkillAssetsResolved(Skill, *ExistingAssets))
        {
            return;
        }
    }

    if (LoadingSkillAssetHandles.Contains(Skill))
    {
        return;
    }

    TArray<FSoftObjectPath> AssetPaths;

    Nios_AddSoftObjectPathIfValid(AssetPaths, Skill->CastLoopMontage);
    Nios_AddSoftObjectPathIfValid(AssetPaths, Skill->ExecuteMontage);
    Nios_AddSoftObjectPathIfValid(AssetPaths, Skill->CastLoopSound);
    Nios_AddSoftObjectPathIfValid(AssetPaths, Skill->ExecuteSound);

    if (AssetPaths.IsEmpty())
    {
        LoadedSkillAssets.FindOrAdd(Skill);
        return;
    }

    TWeakObjectPtr<UNiosCore_AbilitySystemComponent> WeakASC(this);
    TWeakObjectPtr<UNiosCore_SkillDataAsset> WeakSkill(Skill);

    TSharedPtr<FStreamableHandle> Handle =
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            AssetPaths,
            FStreamableDelegate::CreateLambda([WeakASC, WeakSkill]()
                {
                    UNiosCore_AbilitySystemComponent* ASC = WeakASC.Get();
                    UNiosCore_SkillDataAsset* LoadedSkill = WeakSkill.Get();

                    if (!ASC || !LoadedSkill)
                        return;

                    FNiosLoadedSkillAssets& Assets =
                        ASC->LoadedSkillAssets.FindOrAdd(LoadedSkill);

                    Assets.CastLoopMontage = LoadedSkill->CastLoopMontage.Get();
                    Assets.ExecuteMontage = LoadedSkill->ExecuteMontage.Get();
                    Assets.CastLoopSound = LoadedSkill->CastLoopSound.Get();
                    Assets.ExecuteSound = LoadedSkill->ExecuteSound.Get();

                    ASC->LoadingSkillAssetHandles.Remove(LoadedSkill);

                    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL ASSETS ASYNC LOADED: Skill=%s CastLoop=%s Execute=%s CastSound=%s ExecuteSound=%s"),
                        *GetNameSafe(LoadedSkill),
                        *GetNameSafe(Assets.CastLoopMontage.Get()),
                        *GetNameSafe(Assets.ExecuteMontage.Get()),
                        *GetNameSafe(Assets.CastLoopSound.Get()),
                        *GetNameSafe(Assets.ExecuteSound.Get())
                    );
                })
        );

    if (Handle.IsValid())
    {
        LoadingSkillAssetHandles.Add(Skill, Handle);
    }
}

FNiosLoadedSkillAssets* UNiosCore_AbilitySystemComponent::GetLoadedSkillAssets(
    UNiosCore_SkillDataAsset* Skill)
{
    if (!Skill)
        return nullptr;

    return LoadedSkillAssets.Find(Skill);
}

void UNiosCore_AbilitySystemComponent::PreloadGrantedSkillAssetsFromSpecs()
{
    for (const FNiosCoreGrantedSkill& GrantedSkill : GrantedSkills)
    {
        if (!GrantedSkill.SpecHandle.IsValid())
            continue;

        FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(GrantedSkill.SpecHandle);
        if (!Spec)
            continue;

        UNiosCore_SkillDataAsset* Skill =
            Cast<UNiosCore_SkillDataAsset>(Spec->SourceObject.Get());

        if (!Skill)
            continue;

        PreloadSkillAssets(Skill);
    }
}

bool UNiosCore_AbilitySystemComponent::TryActivateSkillByID(FName SkillID)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> TRY ACTIVATE SKILL REQUEST: SkillID=%s Owner=%s Avatar=%s SelectedTarget=%s DisplayedTarget=%s Role=%d"),
        *SkillID.ToString(),
        *GetNameSafe(GetOwnerActor()),
        *GetNameSafe(GetAvatarActor()),
        *GetNameSafe(GetSelectedTarget()),
        *GetNameSafe(GetDisplayedSelectedTarget()),
        static_cast<int32>(GetOwnerRole())
    );

    FGameplayAbilitySpecHandle FoundHandle;
    UNiosCore_SkillDataAsset* FoundSkill = nullptr;
    FindGrantedSkillByID(SkillID, FoundHandle, FoundSkill);

    if (GetOwnerRole() != ROLE_Authority)
    {
        // Immediate local UX hook: hotbar can flash/pending before 120ms server confirmation.
        BroadcastSkillActivationFeedback(SkillID, FoundSkill, ENiosCoreSkillActivationFeedbackState::Requested, ENiosCoreActionFailReason::None, TEXT("ClientRequested"));

        UE_LOG(LogTemp, Warning, TEXT(">>> TRY ACTIVATE ROUTED TO SERVER: SkillID=%s"), *SkillID.ToString());
        Server_TryActivateSkillByID(SkillID);
        return true;
    }

    if (SkillID.IsNone())
    {
        UE_LOG(LogTemp, Error, TEXT(">>> TRY ACTIVATE FAILED: SkillID is None"));
        NotifySkillActivationRejected(SkillID, FoundSkill, ENiosCoreActionFailReason::AbilityBlocked, TEXT("SkillIDNone"));
        return false;
    }

    if (!FoundHandle.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT(">>> TRY ACTIVATE FAILED: No GrantedSkill found for SkillID=%s"),
            *SkillID.ToString()
        );
        NotifySkillActivationRejected(SkillID, FoundSkill, ENiosCoreActionFailReason::AbilityBlocked, TEXT("SkillNotGranted"));
        return false;
    }

    if (FoundSkill)
    {
        PreloadSkillAssets(FoundSkill);
    }

    const bool bActivated = TryActivateAbility(FoundHandle);

    UE_LOG(LogTemp, Warning, TEXT(">>> TRY ACTIVATE RESULT: SkillID=%s HandleValid=%d Activated=%d SourceSkill=%s"),
        *SkillID.ToString(),
        FoundHandle.IsValid() ? 1 : 0,
        bActivated ? 1 : 0,
        *GetNameSafe(FoundSkill)
    );

    // If GAS refuses before our pipeline starts (blocked ability, cooldown GE tags, etc.), clear the client's pending state.
    if (!bActivated)
    {
        NotifySkillActivationRejected(SkillID, FoundSkill, ENiosCoreActionFailReason::AbilityBlocked, TEXT("TryActivateAbilityFalse"));
    }

    return bActivated;
}

void UNiosCore_AbilitySystemComponent::Server_TryActivateSkillByID_Implementation(FName SkillID)
{
    TryActivateSkillByID(SkillID);
}

bool UNiosCore_AbilitySystemComponent::TryActivateSkillByIDAtLocation(FName SkillID, FVector TargetLocation)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> TRY ACTIVATE SKILL AT LOCATION REQUEST: SkillID=%s Location=%s Owner=%s Avatar=%s Role=%d"),
        *SkillID.ToString(),
        *TargetLocation.ToCompactString(),
        *GetNameSafe(GetOwnerActor()),
        *GetNameSafe(GetAvatarActor()),
        static_cast<int32>(GetOwnerRole()));

    if (GetOwnerRole() != ROLE_Authority)
    {
        FGameplayAbilitySpecHandle FoundHandle;
        UNiosCore_SkillDataAsset* FoundSkill = nullptr;
        FindGrantedSkillByID(SkillID, FoundHandle, FoundSkill);
        BroadcastSkillActivationFeedback(SkillID, FoundSkill, ENiosCoreSkillActivationFeedbackState::Requested, ENiosCoreActionFailReason::None, TEXT("ClientRequestedGroundTarget"));
        Server_TryActivateSkillByIDAtLocation(SkillID, FVector_NetQuantize(TargetLocation));
        return true;
    }

    SetPendingGroundTargetLocation(TargetLocation);
    return TryActivateSkillByID(SkillID);
}

void UNiosCore_AbilitySystemComponent::Server_TryActivateSkillByIDAtLocation_Implementation(FName SkillID, FVector_NetQuantize TargetLocation)
{
    TryActivateSkillByIDAtLocation(SkillID, FVector(TargetLocation));
}

void UNiosCore_AbilitySystemComponent::SetPendingGroundTargetLocation(const FVector& Location)
{
    PendingGroundTargetLocation = Location;
    bHasPendingGroundTargetLocation = true;

    UE_LOG(LogTemp, Warning, TEXT(">>> GROUND TARGET LOCATION SET: Location=%s ASC=%s"),
        *PendingGroundTargetLocation.ToCompactString(),
        *GetNameSafe(this));
}

bool UNiosCore_AbilitySystemComponent::ConsumePendingGroundTargetLocation(FVector& OutLocation)
{
    if (!bHasPendingGroundTargetLocation)
    {
        return false;
    }

    OutLocation = PendingGroundTargetLocation;
    PendingGroundTargetLocation = FVector::ZeroVector;
    bHasPendingGroundTargetLocation = false;
    return true;
}

bool UNiosCore_AbilitySystemComponent::PeekPendingGroundTargetLocation(FVector& OutLocation) const
{
    if (!bHasPendingGroundTargetLocation)
    {
        return false;
    }

    OutLocation = PendingGroundTargetLocation;
    return true;
}

bool UNiosCore_AbilitySystemComponent::HasSkillByID(FName SkillID) const
{
    return SkillIDToSpecHandle.Contains(SkillID);
}

FName UNiosCore_AbilitySystemComponent::GetSkillIDBySpecHandle(
    FGameplayAbilitySpecHandle SpecHandle) const
{
    if (const FName* SkillID = SpecHandleToSkillID.Find(SpecHandle))
    {
        return *SkillID;
    }

    return NAME_None;
}

const TArray<FNiosCoreGrantedSkill>& UNiosCore_AbilitySystemComponent::GetGrantedSkills() const
{
    return GrantedSkills;
}

void UNiosCore_AbilitySystemComponent::RebuildSkillMaps()
{
    SkillIDToSpecHandle.Empty();
    SpecHandleToSkillID.Empty();

    for (const FNiosCoreGrantedSkill& GrantedSkill : GrantedSkills)
    {
        if (GrantedSkill.SkillID.IsNone() || !GrantedSkill.SpecHandle.IsValid())
        {
            continue;
        }

        SkillIDToSpecHandle.Add(GrantedSkill.SkillID, GrantedSkill.SpecHandle);
        SpecHandleToSkillID.Add(GrantedSkill.SpecHandle, GrantedSkill.SkillID);
    }
}

bool UNiosCore_AbilitySystemComponent::IsValidTargetActor(AActor* TargetActor) const
{
    if (!TargetActor)
        return false;

    UAbilitySystemComponent* TargetASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

    return TargetASC != nullptr;
}


bool UNiosCore_AbilitySystemComponent::FindGrantedSkillByID(FName SkillID, FGameplayAbilitySpecHandle& OutHandle, UNiosCore_SkillDataAsset*& OutSkill)
{
    OutHandle = FGameplayAbilitySpecHandle();
    OutSkill = nullptr;

    if (SkillID.IsNone())
    {
        return false;
    }

    for (const FNiosCoreGrantedSkill& GrantedSkill : GrantedSkills)
    {
        if (GrantedSkill.SkillID != SkillID || !GrantedSkill.SpecHandle.IsValid())
        {
            continue;
        }

        if (const FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(GrantedSkill.SpecHandle))
        {
            OutHandle = GrantedSkill.SpecHandle;
            OutSkill = Cast<UNiosCore_SkillDataAsset>(Spec->SourceObject.Get());

            UE_LOG(LogTemp, Warning, TEXT(">>> TRY ACTIVATE FOUND SPEC: RequestSkillID=%s GrantedSkillID=%s HandleValid=%d Ability=%s SourceObject=%s CastedSkill=%s"),
                *SkillID.ToString(),
                *GrantedSkill.SkillID.ToString(),
                GrantedSkill.SpecHandle.IsValid() ? 1 : 0,
                *GetNameSafe(Spec->Ability),
                *GetNameSafe(Spec->SourceObject.Get()),
                *GetNameSafe(OutSkill)
            );

            return true;
        }
    }

    return false;
}

void UNiosCore_AbilitySystemComponent::BroadcastSkillActivationFeedback(
    FName SkillID,
    UNiosCore_SkillDataAsset* Skill,
    ENiosCoreSkillActivationFeedbackState State,
    ENiosCoreActionFailReason FailureReason,
    FName DebugReason,
    AActor* TargetActor,
    float RequiredValue,
    float CurrentValue)
{
    FNiosCoreSkillActivationFeedback Feedback;
    Feedback.SkillID = SkillID;
    Feedback.Skill = Skill;
    Feedback.State = State;
    Feedback.FailureReason = FailureReason;
    Feedback.TargetActor = TargetActor;
    Feedback.RequiredValue = RequiredValue;
    Feedback.CurrentValue = CurrentValue;
    Feedback.DebugReason = DebugReason;

    UE_LOG(LogTemp, Warning, TEXT(">>> SKILL FEEDBACK LOCAL: SkillID=%s Skill=%s State=%d Fail=%d Target=%s Required=%.2f Current=%.2f Debug=%s Owner=%s"),
        *SkillID.ToString(),
        *GetNameSafe(Skill),
        static_cast<int32>(State),
        static_cast<int32>(FailureReason),
        *GetNameSafe(TargetActor),
        RequiredValue,
        CurrentValue,
        *DebugReason.ToString(),
        *GetNameSafe(GetOwner()));

    OnSkillActivationFeedback.Broadcast(Feedback);
}

void UNiosCore_AbilitySystemComponent::RouteSkillActivationFeedbackToOwningClient(FNiosCoreSkillActivationFeedback Feedback)
{
    APlayerController* OwningPC = Nios_ASCResolveOwningPlayerController(this);
    if (!OwningPC)
    {
        return;
    }

    if (OwningPC->IsLocalController())
    {
        BroadcastSkillActivationFeedback(Feedback.SkillID, Feedback.Skill, Feedback.State, Feedback.FailureReason, Feedback.DebugReason, Feedback.TargetActor, Feedback.RequiredValue, Feedback.CurrentValue);
    }
    else
    {
        Client_ReceiveSkillActivationFeedback(Feedback);
    }
}

void UNiosCore_AbilitySystemComponent::NotifySkillActivationAccepted(FName SkillID, UNiosCore_SkillDataAsset* Skill, AActor* TargetActor, FName DebugReason)
{
    FNiosCoreSkillActivationFeedback Feedback;
    Feedback.SkillID = SkillID;
    Feedback.Skill = Skill;
    Feedback.State = ENiosCoreSkillActivationFeedbackState::Accepted;
    Feedback.TargetActor = TargetActor;
    Feedback.DebugReason = DebugReason.IsNone() ? FName(TEXT("ServerAccepted")) : DebugReason;
    RouteSkillActivationFeedbackToOwningClient(Feedback);
}

void UNiosCore_AbilitySystemComponent::NotifySkillActivationExecuting(FName SkillID, UNiosCore_SkillDataAsset* Skill, AActor* TargetActor, FName DebugReason)
{
    FNiosCoreSkillActivationFeedback Feedback;
    Feedback.SkillID = SkillID;
    Feedback.Skill = Skill;
    Feedback.State = ENiosCoreSkillActivationFeedbackState::Executing;
    Feedback.TargetActor = TargetActor;
    Feedback.DebugReason = DebugReason.IsNone() ? FName(TEXT("ServerExecuting")) : DebugReason;
    RouteSkillActivationFeedbackToOwningClient(Feedback);
}

void UNiosCore_AbilitySystemComponent::NotifySkillActivationExecuted(FName SkillID, UNiosCore_SkillDataAsset* Skill, AActor* TargetActor, FName DebugReason)
{
    FNiosCoreSkillActivationFeedback Feedback;
    Feedback.SkillID = SkillID;
    Feedback.Skill = Skill;
    Feedback.State = ENiosCoreSkillActivationFeedbackState::Executed;
    Feedback.TargetActor = TargetActor;
    Feedback.DebugReason = DebugReason.IsNone() ? FName(TEXT("ServerExecuted")) : DebugReason;
    RouteSkillActivationFeedbackToOwningClient(Feedback);
}

void UNiosCore_AbilitySystemComponent::NotifySkillActivationInterrupted(FName SkillID, UNiosCore_SkillDataAsset* Skill, ENiosCoreActionFailReason FailureReason, AActor* TargetActor, FName DebugReason, float RequiredValue, float CurrentValue)
{
    FNiosCoreSkillActivationFeedback Feedback;
    Feedback.SkillID = SkillID;
    Feedback.Skill = Skill;
    Feedback.State = ENiosCoreSkillActivationFeedbackState::Interrupted;
    Feedback.FailureReason = FailureReason;
    Feedback.TargetActor = TargetActor;
    Feedback.RequiredValue = RequiredValue;
    Feedback.CurrentValue = CurrentValue;
    Feedback.DebugReason = DebugReason.IsNone() ? FName(TEXT("ServerInterrupted")) : DebugReason;
    RouteSkillActivationFeedbackToOwningClient(Feedback);
}

void UNiosCore_AbilitySystemComponent::NotifySkillActivationFailed(FName SkillID, UNiosCore_SkillDataAsset* Skill, ENiosCoreActionFailReason FailureReason, AActor* TargetActor, FName DebugReason, float RequiredValue, float CurrentValue)
{
    FNiosCoreSkillActivationFeedback Feedback;
    Feedback.SkillID = SkillID;
    Feedback.Skill = Skill;
    Feedback.State = ENiosCoreSkillActivationFeedbackState::Failed;
    Feedback.FailureReason = FailureReason;
    Feedback.TargetActor = TargetActor;
    Feedback.RequiredValue = RequiredValue;
    Feedback.CurrentValue = CurrentValue;
    Feedback.DebugReason = DebugReason.IsNone() ? FName(TEXT("ServerFailed")) : DebugReason;
    RouteSkillActivationFeedbackToOwningClient(Feedback);
}

void UNiosCore_AbilitySystemComponent::NotifySkillActivationRejected(FName SkillID, UNiosCore_SkillDataAsset* Skill, ENiosCoreActionFailReason FailureReason, FName DebugReason)
{
    FNiosCoreSkillActivationFeedback Feedback;
    Feedback.SkillID = SkillID;
    Feedback.Skill = Skill;
    Feedback.State = ENiosCoreSkillActivationFeedbackState::Rejected;
    Feedback.FailureReason = FailureReason;
    Feedback.DebugReason = DebugReason.IsNone() ? FName(TEXT("ServerRejected")) : DebugReason;
    RouteSkillActivationFeedbackToOwningClient(Feedback);
}

void UNiosCore_AbilitySystemComponent::Client_ReceiveSkillActivationFeedback_Implementation(FNiosCoreSkillActivationFeedback Feedback)
{
    BroadcastSkillActivationFeedback(Feedback.SkillID, Feedback.Skill, Feedback.State, Feedback.FailureReason, Feedback.DebugReason, Feedback.TargetActor, Feedback.RequiredValue, Feedback.CurrentValue);
}

void UNiosCore_AbilitySystemComponent::SetSelectedTarget(AActor* NewTarget)
{
    if (NewTarget && !IsValidTargetActor(NewTarget))
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> TARGET: Rejected actor without ASC: %s"),
            *GetNameSafe(NewTarget)
        );

        if (GetOwnerRole() != ROLE_Authority)
        {
            FNiosCoreActionFailResult Result(ENiosCoreActionFailReason::InvalidTarget);
            Result.SourceActor = GetAvatarActor();
            Result.TargetActor = NewTarget;
            Result.DebugReason = TEXT("ClientRejectedTargetWithoutASC");
            UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(GetAvatarActor(), Result);
        }
        return;
    }

    if (GetOwnerRole() == ROLE_Authority)
    {
        SelectedTarget = NewTarget;
        LocalSelectedTargetPreview = nullptr;

        UE_LOG(LogTemp, Warning, TEXT(">>> TARGET: Server selected target = %s"),
            *GetNameSafe(SelectedTarget.Get())
        );

        OnRep_SelectedTarget();
        ForceReplication();
        return;
    }

    // Optimistic local preview: UI/highlight responds immediately; server still owns the real SelectedTarget.
    LocalSelectedTargetPreview = NewTarget;
    NotifyLocalDisplayedTargetChanged(GetDisplayedSelectedTarget(), false, FName(TEXT("PreviewSelected")));
    OnSelectedTargetChanged.Broadcast(GetDisplayedSelectedTarget(), false);

    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET: Client previews and requests selected target = %s"),
        *GetNameSafe(NewTarget)
    );

    Server_SetSelectedTarget(NewTarget);
}

bool UNiosCore_AbilitySystemComponent::SetAttributeValue(
    FGameplayAttribute Attribute,
    float NewValue)
{
    if (GetOwnerRole() != ROLE_Authority)
        return false;

    if (!Attribute.IsValid())
        return false;

    if (!GetSet<UNiosCore_AttributeSet>())
    {
        if (!StatsAttributeSet)
        {
            if (AActor* OwningActor = GetOwner())
            {
                if (OwningActor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
                {
                    StatsAttributeSet = INiosGASActorInterface::Execute_GetNiosAttributeSet(OwningActor);
                }
            }
        }

        if (StatsAttributeSet)
        {
            AddAttributeSetSubobject(StatsAttributeSet.Get());
        }
    }

    if (!GetSet<UNiosCore_AttributeSet>())
    {
        UE_LOG(LogTemp, Warning, TEXT("Nios ASC: skipped SetAttributeValue for %s on %s because UNiosCore_AttributeSet is not registered."),
            *Attribute.GetName(),
            *GetNameSafe(GetOwner()));
        return false;
    }

    const float OldValue = GetNumericAttribute(Attribute);
    const float ClampedValue = Nios_ClampAttributeValueForWrite(this, Attribute, NewValue);

    SetNumericAttributeBase(Attribute, ClampedValue);
    Nios_SyncCurrentResourceRuntimeSource(this, Attribute, ClampedValue);
    Nios_ClampPairedResourceAfterWrite(this, Attribute);
    Nios_UpdateDeathLifecycleAfterHealthWrite(this, Attribute, OldValue, GetNumericAttribute(Attribute));
    ForceReplication();

    return true;
}

bool UNiosCore_AbilitySystemComponent::GetAttributeValue(
    FGameplayAttribute Attribute,
    float& OutValue) const
{
    OutValue = 0.f;

    if (!Attribute.IsValid())
        return false;

    OutValue = GetNumericAttribute(Attribute);
    return true;
}

bool UNiosCore_AbilitySystemComponent::AddAttributeValue(
    FGameplayAttribute Attribute,
    float DeltaValue)
{
    if (GetOwnerRole() != ROLE_Authority)
        return false;

    if (!Attribute.IsValid())
        return false;

    if (FMath::IsNearlyZero(DeltaValue))
        return true;

    const float CurrentValue = GetNumericAttribute(Attribute);
    const float NewValue = CurrentValue + DeltaValue;
    const float ClampedValue = Nios_ClampAttributeValueForWrite(this, Attribute, NewValue);

    SetNumericAttributeBase(Attribute, ClampedValue);
    Nios_SyncCurrentResourceRuntimeSource(this, Attribute, ClampedValue);
    Nios_ClampPairedResourceAfterWrite(this, Attribute);
    Nios_UpdateDeathLifecycleAfterHealthWrite(this, Attribute, CurrentValue, GetNumericAttribute(Attribute));
    ForceReplication();

    return true;
}

void UNiosCore_AbilitySystemComponent::Server_SetSelectedTarget_Implementation(AActor* NewTarget)
{
    if (NewTarget && !IsValidTargetActor(NewTarget))
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> TARGET: Server rejected actor without ASC: %s"),
            *GetNameSafe(NewTarget)
        );

        Client_ReceiveSelectedTargetRejected(NewTarget);
        return;
    }

    SelectedTarget = NewTarget;

    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET: Server_SetSelectedTarget = %s"),
        *GetNameSafe(SelectedTarget.Get())
    );

    OnRep_SelectedTarget();
    ForceReplication();
}

void UNiosCore_AbilitySystemComponent::Client_ReceiveSelectedTargetRejected_Implementation(AActor* RejectedTarget)
{
    if (LocalSelectedTargetPreview == RejectedTarget)
    {
        LocalSelectedTargetPreview = nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET: Client received target rejection = %s Confirmed=%s"),
        *GetNameSafe(RejectedTarget),
        *GetNameSafe(SelectedTarget.Get())
    );

    NotifyLocalDisplayedTargetChanged(GetDisplayedSelectedTarget(), true, FName(TEXT("ServerRejected")));

    OnSelectedTargetRejected.Broadcast(RejectedTarget);
    OnSelectedTargetChanged.Broadcast(GetDisplayedSelectedTarget(), true);

    FNiosCoreActionFailResult Result(ENiosCoreActionFailReason::InvalidTarget);
    Result.SourceActor = GetAvatarActor();
    Result.TargetActor = RejectedTarget;
    Result.DebugReason = TEXT("ServerRejectedTarget");
    UNiosCore_ActionFeedbackComponent::NotifyActionFailedForActor(GetAvatarActor(), Result);
}

AActor* UNiosCore_AbilitySystemComponent::GetSelectedTarget() const
{
    return SelectedTarget.Get();
}

AActor* UNiosCore_AbilitySystemComponent::GetDisplayedSelectedTarget() const
{
    if (LocalSelectedTargetPreview)
    {
        return LocalSelectedTargetPreview.Get();
    }

    return SelectedTarget.Get();
}

AActor* UNiosCore_AbilitySystemComponent::GetLocalSelectedTargetPreview() const
{
    return LocalSelectedTargetPreview.Get();
}

void UNiosCore_AbilitySystemComponent::ClearSelectedTarget()
{
    SetSelectedTarget(nullptr);
}

void UNiosCore_AbilitySystemComponent::OnRep_SelectedTarget()
{
    if (!LocalSelectedTargetPreview || LocalSelectedTargetPreview == SelectedTarget)
    {
        LocalSelectedTargetPreview = nullptr;
    }

    UE_LOG(LogTemp, Warning, TEXT(">>> TARGET: OnRep_SelectedTarget = %s Preview=%s"),
        *GetNameSafe(SelectedTarget.Get()),
        *GetNameSafe(LocalSelectedTargetPreview.Get())
    );

    NotifyLocalDisplayedTargetChanged(GetDisplayedSelectedTarget(), true, FName(TEXT("ServerConfirmed")));
    OnSelectedTargetChanged.Broadcast(GetDisplayedSelectedTarget(), true);
}

bool UNiosCore_AbilitySystemComponent::ShouldDispatchLocalTargetFeedback() const
{
    const AActor* CurrentAvatarActor = GetAvatarActor();
    if (const APawn* AvatarPawn = Cast<APawn>(CurrentAvatarActor))
    {
        return AvatarPawn->IsLocallyControlled();
    }

    const AActor* CurrentOwnerActor = GetOwnerActor();
    if (const APawn* OwnerPawn = Cast<APawn>(CurrentOwnerActor))
    {
        return OwnerPawn->IsLocallyControlled();
    }

    if (const APlayerController* OwnerController = Cast<APlayerController>(CurrentOwnerActor))
    {
        return OwnerController->IsLocalController();
    }

    return false;
}

void UNiosCore_AbilitySystemComponent::NotifyLocalDisplayedTargetChanged(AActor* NewDisplayedTarget, bool bServerConfirmed, FName Reason)
{
    if (!ShouldDispatchLocalTargetFeedback())
    {
        return;
    }

    AActor* OldDisplayedTarget = LastLocallyNotifiedDisplayedTarget.Get();
    if (OldDisplayedTarget == NewDisplayedTarget)
    {
        return;
    }

    AActor* TargetingActor = GetAvatarActor();
    if (!TargetingActor)
    {
        TargetingActor = GetOwnerActor();
    }

    APlayerController* TargetingController = nullptr;
    if (APawn* TargetingPawn = Cast<APawn>(TargetingActor))
    {
        TargetingController = Cast<APlayerController>(TargetingPawn->GetController());
    }

    if (!TargetingController)
    {
        TargetingController = Cast<APlayerController>(GetOwnerActor());
    }

    FNiosCoreTargetFeedbackContext Context;
    Context.TargetingActor = TargetingActor;
    Context.TargetingController = TargetingController;
    Context.TargetingAbilitySystem = this;
    Context.PreviousTarget = OldDisplayedTarget;
    Context.NewTarget = NewDisplayedTarget;
    Context.bServerConfirmed = bServerConfirmed;
    Context.Reason = Reason;

    if (OldDisplayedTarget)
    {
        Context.TargetActor = OldDisplayedTarget;
        UNiosCore_TargetFeedbackComponent::DispatchLocalTargetFeedback(OldDisplayedTarget, ENiosCoreTargetFeedbackEventType::Untargeted, Context);
    }

    LastLocallyNotifiedDisplayedTarget = NewDisplayedTarget;

    if (NewDisplayedTarget)
    {
        Context.TargetActor = NewDisplayedTarget;
        UNiosCore_TargetFeedbackComponent::DispatchLocalTargetFeedback(NewDisplayedTarget, ENiosCoreTargetFeedbackEventType::Targeted, Context);
    }
}

bool UNiosCore_AbilitySystemComponent::IsCasting() const
{
    return bIsCasting;
}

UNiosCore_SkillDataAsset* UNiosCore_AbilitySystemComponent::GetCurrentCastSkill() const
{
    return CurrentCastSkill.Get();
}

AActor* UNiosCore_AbilitySystemComponent::GetCastLockedTarget() const
{
    return CastLockedTarget.Get();
}

AActor* UNiosCore_AbilitySystemComponent::GetCurrentLookAtTarget() const
{
    if (bIsCasting && CastLockedTarget)
    {
        return CastLockedTarget.Get();
    }

    return GetDisplayedSelectedTarget();
}

float UNiosCore_AbilitySystemComponent::GetCastStartTime() const
{
    return CastStartTime;
}

float UNiosCore_AbilitySystemComponent::GetCastEndTime() const
{
    return CastEndTime;
}

float UNiosCore_AbilitySystemComponent::GetCastDuration() const
{
    return FMath::Max(0.f, CastEndTime - CastStartTime);
}

float UNiosCore_AbilitySystemComponent::GetCastRemainingTime() const
{
    if (!bIsCasting)
        return 0.f;

    UWorld* World = GetWorld();
    if (!World)
        return 0.f;

    return FMath::Max(0.f, CastEndTime - World->GetTimeSeconds());
}

float UNiosCore_AbilitySystemComponent::GetCastProgress01() const
{
    if (!bIsCasting)
        return 0.f;

    const float Duration = GetCastDuration();
    if (Duration <= 0.f)
        return 1.f;

    UWorld* World = GetWorld();
    if (!World)
        return 0.f;

    const float Now = World->GetTimeSeconds();
    return FMath::Clamp((Now - CastStartTime) / Duration, 0.f, 1.f);
}

void UNiosCore_AbilitySystemComponent::BeginCastState(UNiosCore_SkillDataAsset* Skill, float CastTime, AActor* LockedTarget)
{
    if (GetOwnerRole() != ROLE_Authority)
        return;

    UWorld* World = GetWorld();
    if (!World)
        return;

    bIsCasting = true;
    CurrentCastSkill = Skill;
    CastLockedTarget = LockedTarget;
    CastStartTime = World->GetTimeSeconds();
    CastEndTime = CastStartTime + FMath::Max(0.f, CastTime);

    OnRep_CastState();
    ForceReplication();
}

void UNiosCore_AbilitySystemComponent::EndCastState()
{
    if (GetOwnerRole() != ROLE_Authority)
        return;

    bIsCasting = false;
    CurrentCastSkill = nullptr;
    CastLockedTarget = nullptr;
    CastStartTime = 0.f;
    CastEndTime = 0.f;

    OnRep_CastState();
    ForceReplication();
}

void UNiosCore_AbilitySystemComponent::OnRep_CastState()
{
    UE_LOG(LogTemp, Warning, TEXT(">>> CAST STATE: IsCasting=%d Skill=%s LockedTarget=%s LookAt=%s Start=%f End=%f"),
        bIsCasting ? 1 : 0,
        *GetNameSafe(CurrentCastSkill.Get()),
        *GetNameSafe(CastLockedTarget.Get()),
        *GetNameSafe(GetCurrentLookAtTarget()),
        CastStartTime,
        CastEndTime
    );

    OnCastStateChanged.Broadcast();
}
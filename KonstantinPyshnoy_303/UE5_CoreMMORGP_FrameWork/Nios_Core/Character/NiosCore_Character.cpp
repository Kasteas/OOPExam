#include "Character/NiosCore_Character.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Character/NiosCore_PlayerState.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Components/Equipment/NiosCore_EquipmentComponent.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "Components/Targeting/NiosCore_TargetFeedbackComponent.h"
#include "Net/UnrealNetwork.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "MOMissionsManager.h"

ANiosCore_Character::ANiosCore_Character()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    TargetFeedbackComponent = CreateDefaultSubobject<UNiosCore_TargetFeedbackComponent>(TEXT("TargetFeedbackComponent"));

    // Let UE CharacterMovement do its normal prediction, correction and smoothing.
    // Custom replicated transform interpolation here was fighting CharacterMovement and caused visible jitter.
    SetReplicateMovement(true);
}

void ANiosCore_Character::BeginPlay()
{
    Super::BeginPlay();
    InitAbilitySystemFromPlayerState();
    ApplyDeathMovementState(bDead);
}

void ANiosCore_Character::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ANiosCore_Character, bDead);
}

void ANiosCore_Character::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    InitAbilitySystemFromPlayerState();
}

void ANiosCore_Character::OnRep_PlayerState()
{
    Super::OnRep_PlayerState();
    InitAbilitySystemFromPlayerState();
}

void ANiosCore_Character::UnPossessed()
{
    Super::UnPossessed();
    bAbilitySystemInitialized = false;
}

UAbilitySystemComponent* ANiosCore_Character::GetAbilitySystemComponent() const
{
    return GetNiosAbilitySystemComponent();
}

UNiosCore_AbilitySystemComponent* ANiosCore_Character::GetNiosAbilitySystemComponent() const
{
    const ANiosCore_PlayerState* NiosPS = GetPlayerState<ANiosCore_PlayerState>();
    return NiosPS ? NiosPS->GetNiosAbilitySystemComponent() : nullptr;
}

UNiosCore_AbilitySystemComponent* ANiosCore_Character::GetNiosASC_Implementation() const
{
    return GetNiosAbilitySystemComponent();
}

UAbilitySystemComponent* ANiosCore_Character::GetAbilitySystemComponentGeneric_Implementation() const
{
    return GetAbilitySystemComponent();
}

UNiosCore_AttributeSet* ANiosCore_Character::GetNiosAttributeSet_Implementation() const
{
    const ANiosCore_PlayerState* NiosPS = GetPlayerState<ANiosCore_PlayerState>();
    return NiosPS ? NiosPS->GetStatsAttributeSet() : nullptr;
}

UNiosCore_StatsComponent* ANiosCore_Character::GetNiosStatsComponent_Implementation() const
{
    const ANiosCore_PlayerState* NiosPS = GetPlayerState<ANiosCore_PlayerState>();
    return NiosPS ? NiosPS->StatsComponent : nullptr;
}

UNiosCore_StatusEffectComponent* ANiosCore_Character::GetStatusEffectComponentGeneric_Implementation() const
{
    const ANiosCore_PlayerState* NiosPS = GetPlayerState<ANiosCore_PlayerState>();
    return NiosPS ? NiosPS->StatusEffectComponent : nullptr;
}

bool ANiosCore_Character::IsAlive_Implementation() const
{
    if (bDead)
    {
        return false;
    }

    if (const UNiosCore_AttributeSet* AttributeSet = GetNiosAttributeSet_Implementation())
    {
        return AttributeSet->GetHealth() > 0.f;
    }

    return true;
}

bool ANiosCore_Character::CanBeTargetedBy_Implementation(AActor* SourceActor) const
{
    return SourceActor != this && IsAlive_Implementation();
}

FName ANiosCore_Character::GetTeamID_Implementation() const
{
    return NiosCoreTeams::Player;
}

void ANiosCore_Character::HandleDeath_Implementation(AActor* Killer)
{
    const bool bWasDead = bDead;
    bDead = true;

    // Always enforce movement lock, even if the pawn had already been marked dead by
    // another code path. Self-damage can set the dead flag before the visual/pawn
    // lifecycle receives its final HandleDeath call.
    ApplyDeathMovementState(true);

    if (bWasDead)
    {
        return;
    }

    BP_OnDeath(Killer);
}

void ANiosCore_Character::HandleRevive()
{
    const bool bWasDead = bDead;
    bDead = false;

    if (HasAuthority())
    {
        if (UNiosCore_StatusEffectComponent* StatusEffects = GetStatusEffectComponentGeneric_Implementation())
        {
            StatusEffects->RefreshPersistentEffectsAfterRevive();
        }

        if (ANiosCore_PlayerState* NiosPS = GetPlayerState<ANiosCore_PlayerState>())
        {
            if (UNiosCore_EquipmentComponent* Equipment = NiosPS->GetEquipmentComponent())
            {
                Equipment->RefreshEquipmentStatusEffectsAfterRevive();
            }
        }
    }

    ApplyDeathMovementState(false);

    if (!bWasDead)
    {
        return;
    }

    BP_OnRevive();
}

void ANiosCore_Character::OnRep_Dead()
{
    ApplyDeathMovementState(bDead);

    if (bDead)
    {
        BP_OnDeath(nullptr);
    }
    else
    {
        BP_OnRevive();
    }
}

void ANiosCore_Character::ApplyDeathMovementState(bool bInDeadState)
{
    UCharacterMovementComponent* MovementComponent = GetCharacterMovement();

    // MovementMode replication is not enough for owning-client self-damage: local prediction can
    // keep feeding input for a short window, and in some BP setups it looks like the corpse can
    // still run. Lock Controller move input as well, but make it idempotent so repeated death
    // notifications do not stack SetIgnoreMoveInput(true).
    if (AController* OwningController = GetController())
    {
        if (bInDeadState && !bDeathMoveInputLocked)
        {
            OwningController->SetIgnoreMoveInput(true);
            bDeathMoveInputLocked = true;
        }
        else if (!bInDeadState && bDeathMoveInputLocked)
        {
            OwningController->SetIgnoreMoveInput(false);
            bDeathMoveInputLocked = false;
        }
    }

    if (!MovementComponent)
    {
        return;
    }

    if (bInDeadState)
    {
        MovementComponent->StopMovementImmediately();
        MovementComponent->Velocity = FVector::ZeroVector;
        MovementComponent->DisableMovement();
        return;
    }

    if (MovementComponent->MovementMode == MOVE_None)
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
    }
}

UMOMissionsManager* ANiosCore_Character::GetNiosMissionManager_Implementation() const
{
    const ANiosCore_PlayerState* NiosPS = GetPlayerState<ANiosCore_PlayerState>();
    return NiosPS ? NiosPS->GetMissionManager() : nullptr;
}

UMOMissionsManager* ANiosCore_Character::GetNiosMissionsManager() const
{
    return GetNiosMissionManager_Implementation();
}

void ANiosCore_Character::SetCombatRangeRadiusOverride(float NewRadius)
{
    CombatRangeRadiusOverride = FMath::Max(0.f, NewRadius);
}

void ANiosCore_Character::InitAbilitySystemFromPlayerState()
{
    if (bAbilitySystemInitialized)
    {
        return;
    }

    ANiosCore_PlayerState* NiosPS = GetPlayerState<ANiosCore_PlayerState>();
    if (!NiosPS)
    {
        return;
    }

    NiosPS->InitAvatar(this);

    if (UNiosCore_StatsComponent* Stats = NiosPS->StatsComponent)
    {
        Stats->InitializeAbilitySystemComponent(NiosPS->GetNiosAbilitySystemComponent());
        Stats->InitializeStatsComponent();
    }

    if (UNiosCore_StatusEffectComponent* StatusEffects = NiosPS->GetStatusEffectComponent())
    {
        StatusEffects->InitializeStatusEffectComponent(NiosPS->GetNiosAbilitySystemComponent(), NiosPS->StatsComponent);
    }

    bAbilitySystemInitialized = true;
    BP_OnAbilitySystemInitialized();
}


void ANiosCore_Character::Multicast_PlaySkillExecuteVisuals_Implementation(UNiosCore_SkillDataAsset* Skill)
{
    PlaySkillExecuteVisualsLocal(Skill);
}

void ANiosCore_Character::Multicast_PlaySkillCastLoopVisuals_Implementation(UNiosCore_SkillDataAsset* Skill, bool bStart)
{
    PlaySkillCastLoopVisualsLocal(Skill, bStart);
}


void ANiosCore_Character::Multicast_PlayDeliveryNiagara_Implementation(
    const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem,
    FVector Location,
    FRotator Rotation,
    FName DebugName)
{
    PlayDeliveryNiagaraLocal(NiagaraSystem, Location, Rotation, DebugName);
}

void ANiosCore_Character::PlayDeliveryNiagaraLocal(
    const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem,
    FVector Location,
    FRotator Rotation,
    FName DebugName)
{
    if (GetNetMode() == NM_DedicatedServer || NiagaraSystem.IsNull())
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    auto SpawnLoaded = [World, Location, Rotation, DebugName](UNiagaraSystem* LoadedSystem)
    {
        if (!World || !LoadedSystem)
        {
            return;
        }

        UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World,
            LoadedSystem,
            Location,
            Rotation,
            FVector(1.f),
            true,
            true,
            ENCPoolMethod::AutoRelease,
            true);

        UE_LOG(LogTemp, Verbose, TEXT(">>> SKILL DELIVERY NIAGARA CLIENT SPAWNED: FX=%s Name=%s Location=%s"),
            *GetNameSafe(LoadedSystem),
            *DebugName.ToString(),
            *Location.ToCompactString());
    };

    if (UNiagaraSystem* LoadedSystem = NiagaraSystem.Get())
    {
        SpawnLoaded(LoadedSystem);
        return;
    }

    const FSoftObjectPath SoftPath = NiagaraSystem.ToSoftObjectPath();
    if (!SoftPath.IsValid())
    {
        return;
    }

    TWeakObjectPtr<ANiosCore_Character> WeakThis(this);
    TSoftObjectPtr<UNiagaraSystem> NiagaraCopy = NiagaraSystem;

    UAssetManager::GetStreamableManager().RequestAsyncLoad(
        SoftPath,
        FStreamableDelegate::CreateLambda([WeakThis, NiagaraCopy, Location, Rotation, DebugName]() mutable
        {
            if (!WeakThis.IsValid())
            {
                return;
            }

            ANiosCore_Character* Character = WeakThis.Get();
            if (!Character || Character->GetNetMode() == NM_DedicatedServer)
            {
                return;
            }

            UWorld* LoadedWorld = Character->GetWorld();
            UNiagaraSystem* LoadedSystem = NiagaraCopy.Get();
            if (!LoadedWorld || !LoadedSystem)
            {
                return;
            }

            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                LoadedWorld,
                LoadedSystem,
                Location,
                Rotation,
                FVector(1.f),
                true,
                true,
                ENCPoolMethod::AutoRelease,
                true);

            UE_LOG(LogTemp, Verbose, TEXT(">>> SKILL DELIVERY NIAGARA CLIENT ASYNC SPAWNED: FX=%s Name=%s Location=%s"),
                *GetNameSafe(LoadedSystem),
                *DebugName.ToString(),
                *Location.ToCompactString());
        }),
        FStreamableManager::AsyncLoadHighPriority,
        false);
}


void ANiosCore_Character::PlaySkillExecuteVisualsLocal(UNiosCore_SkillDataAsset* Skill)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!Skill)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL EXECUTE: Missing skill Character=%s"), *GetNameSafe(this));
        return;
    }

    UNiosCore_AbilitySystemComponent* ASC = GetNiosAbilitySystemComponent();
    FNiosLoadedSkillAssets* Assets = nullptr;
    if (ASC)
    {
        Assets = ASC->GetLoadedSkillAssets(Skill);
        if (!Assets)
        {
            ASC->PreloadSkillAssets(Skill);
            Assets = ASC->GetLoadedSkillAssets(Skill);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL EXECUTE: Missing ASC, using skill soft refs Character=%s Skill=%s"), *GetNameSafe(this), *GetNameSafe(Skill));
    }

    UAnimMontage* Montage = Assets ? Assets->ExecuteMontage : nullptr;
    USoundBase* Sound = Assets ? Assets->ExecuteSound : nullptr;

    // Remote pawns/AI may receive the multicast or cue before their local ASC asset cache has finished.
    // Fall back to the skill soft refs so first-seen enemy skills still have montage/sound.
    if (!Montage)
    {
        Montage = Skill->LoadExecuteMontageSynchronous();
    }
    if (!Sound)
    {
        Sound = Skill->LoadExecuteSoundSynchronous();
    }

    if (Montage)
    {
        if (USkeletalMeshComponent* MeshComp = GetMesh())
        {
            if (UAnimInstance* AnimInstance = MeshComp->GetAnimInstance())
            {
                const float PlayedLength = AnimInstance->Montage_Play(Montage);
                UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL EXECUTE: Montage=%s PlayedLength=%.3f Character=%s Skill=%s"),
                    *GetNameSafe(Montage), PlayedLength, *GetNameSafe(this), *GetNameSafe(Skill));
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL EXECUTE: Missing AnimInstance Character=%s Montage=%s"), *GetNameSafe(this), *GetNameSafe(Montage));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL EXECUTE: No execute montage Character=%s Skill=%s"), *GetNameSafe(this), *GetNameSafe(Skill));
    }

    if (Sound)
    {
        UGameplayStatics::SpawnSoundAttached(Sound, GetRootComponent());
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL EXECUTE: Sound=%s Character=%s Skill=%s"), *GetNameSafe(Sound), *GetNameSafe(this), *GetNameSafe(Skill));
    }
}

void ANiosCore_Character::PlaySkillCastLoopVisualsLocal(UNiosCore_SkillDataAsset* Skill, bool bStart)
{
    if (GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    if (!Skill)
    {
        return;
    }

    UNiosCore_AbilitySystemComponent* ASC = GetNiosAbilitySystemComponent();
    FNiosLoadedSkillAssets* Assets = nullptr;
    if (ASC)
    {
        Assets = ASC->GetLoadedSkillAssets(Skill);
        if (!Assets)
        {
            ASC->PreloadSkillAssets(Skill);
            Assets = ASC->GetLoadedSkillAssets(Skill);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL CAST: Missing ASC, using skill soft refs Character=%s Skill=%s Start=%d"), *GetNameSafe(this), *GetNameSafe(Skill), bStart ? 1 : 0);
    }

    UAnimMontage* Montage = Assets ? Assets->CastLoopMontage : nullptr;
    USoundBase* Sound = Assets ? Assets->CastLoopSound : nullptr;

    // Remote pawns/AI may receive the multicast or cue before their local ASC asset cache has finished.
    // Fall back to the skill soft refs so first-seen enemy skills still have montage/sound.
    if (!Montage)
    {
        Montage = Skill->LoadCastLoopMontageSynchronous();
    }
    if (!Sound)
    {
        Sound = Skill->LoadCastLoopSoundSynchronous();
    }

    if (UAnimInstance* AnimInstance = GetMesh() ? GetMesh()->GetAnimInstance() : nullptr)
    {
        if (Montage)
        {
            if (bStart)
            {
                const float PlayedLength = AnimInstance->Montage_Play(Montage);
                UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL CAST START: Montage=%s PlayedLength=%.3f Character=%s Skill=%s"),
                    *GetNameSafe(Montage), PlayedLength, *GetNameSafe(this), *GetNameSafe(Skill));
            }
            else
            {
                AnimInstance->Montage_Stop(0.15f, Montage);
                UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL CAST STOP: Montage=%s Character=%s Skill=%s"),
                    *GetNameSafe(Montage), *GetNameSafe(this), *GetNameSafe(Skill));
            }
        }
    }

    if (bStart && Sound)
    {
        UGameplayStatics::SpawnSoundAttached(Sound, GetRootComponent());
        UE_LOG(LogTemp, Warning, TEXT(">>> SKILL VISUAL CAST START: Sound=%s Character=%s Skill=%s"), *GetNameSafe(Sound), *GetNameSafe(this), *GetNameSafe(Skill));
    }
}

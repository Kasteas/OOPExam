#include "AbilitySystem/Actors/NiosCore_GASAICharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/AudioComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "Components/Targeting/NiosCore_TargetFeedbackComponent.h"
#include "Components/Loot/NiosCore_LootSourceComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Sound/SoundBase.h"

ANiosCore_GASAICharacter::ANiosCore_GASAICharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    SetReplicateMovement(true);

    AbilitySystemComponent = CreateDefaultSubobject<UNiosCore_AbilitySystemComponent>(TEXT("AbilitySystemComponent"));
    AttributeSet = CreateDefaultSubobject<UNiosCore_AttributeSet>(TEXT("AttributeSet"));
    StatsComponent = CreateDefaultSubobject<UNiosCore_StatsComponent>(TEXT("StatsComponent"));
    StatusEffectComponent = CreateDefaultSubobject<UNiosCore_StatusEffectComponent>(TEXT("StatusEffectComponent"));
    TargetFeedbackComponent = CreateDefaultSubobject<UNiosCore_TargetFeedbackComponent>(TEXT("TargetFeedbackComponent"));
    LootSourceComponent = CreateDefaultSubobject<UNiosCore_LootSourceComponent>(TEXT("LootSourceComponent"));
}

void ANiosCore_GASAICharacter::BeginPlay()
{
    Super::BeginPlay();

    if (bDisableMovementOnBeginPlay)
    {
        if (UCharacterMovementComponent* Movement = GetCharacterMovement())
        {
            Movement->DisableMovement();
            Movement->StopMovementImmediately();
        }
    }

    ApplyDeathMovementState(bDead);

    if (AbilitySystemComponent)
    {
        AbilitySystemComponent->InitOwnerAvatar(this, this, AttributeSet);
    }

    if (StatsComponent)
    {
        StatsComponent->InitializeAbilitySystemComponent(AbilitySystemComponent);
        StatsComponent->InitializeStatsComponent();
    }

    if (StatusEffectComponent)
    {
        StatusEffectComponent->InitializeStatusEffectComponent(AbilitySystemComponent, StatsComponent);
    }
}

void ANiosCore_GASAICharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ANiosCore_GASAICharacter, bDead);
}

UAbilitySystemComponent* ANiosCore_GASAICharacter::GetAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

UNiosCore_AbilitySystemComponent* ANiosCore_GASAICharacter::GetNiosAbilitySystemComponent() const
{
    return AbilitySystemComponent;
}

void ANiosCore_GASAICharacter::SetRelationToPlayer(ENiosCoreTargetRelation NewRelation)
{
    RelationToPlayer = NewRelation;
}

void ANiosCore_GASAICharacter::SetImmortal(bool bNewImmortal)
{
    bImmortal = bNewImmortal;
}

void ANiosCore_GASAICharacter::SetCanBeTargeted(bool bNewCanBeTargeted)
{
    bCanBeTargeted = bNewCanBeTargeted;
}

void ANiosCore_GASAICharacter::SetCanDealDamage(bool bNewCanDealDamage)
{
    bCanDealDamage = bNewCanDealDamage;
}

void ANiosCore_GASAICharacter::SetCombatRangeRadiusOverride(float NewRadius)
{
    CombatRangeRadiusOverride = FMath::Max(0.f, NewRadius);
}


void ANiosCore_GASAICharacter::Multicast_PlaySkillExecuteVisuals_Implementation(UNiosCore_SkillDataAsset* Skill)
{
    PlaySkillExecuteVisualsLocal(Skill);
}

void ANiosCore_GASAICharacter::Multicast_PlaySkillCastLoopVisuals_Implementation(UNiosCore_SkillDataAsset* Skill, bool bStart)
{
    PlaySkillCastLoopVisualsLocal(Skill, bStart);
}

void ANiosCore_GASAICharacter::PlaySkillExecuteVisualsLocal(UNiosCore_SkillDataAsset* Skill)
{
    if (GetNetMode() == NM_DedicatedServer || !Skill)
    {
        return;
    }

    FNiosLoadedSkillAssets* Assets = nullptr;
    if (AbilitySystemComponent)
    {
        Assets = AbilitySystemComponent->GetLoadedSkillAssets(Skill);
        if (!Assets)
        {
            AbilitySystemComponent->PreloadSkillAssets(Skill);
            Assets = AbilitySystemComponent->GetLoadedSkillAssets(Skill);
        }
    }

    UAnimMontage* Montage = Assets ? Assets->ExecuteMontage : nullptr;
    USoundBase* Sound = Assets ? Assets->ExecuteSound : nullptr;

    // Remote AI pawns can receive the multicast before their local ASC asset cache has finished.
    // Resolve the skill soft refs directly so first-seen enemy skills still play sound/animation.
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
                AnimInstance->Montage_Play(Montage);
            }
        }
    }

    if (Sound)
    {
        UGameplayStatics::SpawnSoundAttached(Sound, GetRootComponent());
    }
}

void ANiosCore_GASAICharacter::PlaySkillCastLoopVisualsLocal(UNiosCore_SkillDataAsset* Skill, bool bStart)
{
    if (GetNetMode() == NM_DedicatedServer || !Skill)
    {
        return;
    }

    FNiosLoadedSkillAssets* Assets = nullptr;
    if (AbilitySystemComponent)
    {
        Assets = AbilitySystemComponent->GetLoadedSkillAssets(Skill);
        if (!Assets)
        {
            AbilitySystemComponent->PreloadSkillAssets(Skill);
            Assets = AbilitySystemComponent->GetLoadedSkillAssets(Skill);
        }
    }

    UAnimMontage* Montage = Assets ? Assets->CastLoopMontage : nullptr;
    USoundBase* Sound = Assets ? Assets->CastLoopSound : nullptr;

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
                AnimInstance->Montage_Play(Montage);
            }
            else
            {
                AnimInstance->Montage_Stop(0.15f, Montage);
            }
        }
    }

    if (bStart && Sound)
    {
        if (ActiveCastLoopAudioComponent)
        {
            ActiveCastLoopAudioComponent->Stop();
            ActiveCastLoopAudioComponent = nullptr;
        }

        ActiveCastLoopAudioComponent = UGameplayStatics::SpawnSoundAttached(Sound, GetRootComponent());
    }
    else if (!bStart && ActiveCastLoopAudioComponent)
    {
        ActiveCastLoopAudioComponent->Stop();
        ActiveCastLoopAudioComponent = nullptr;
    }
}

UNiosCore_AbilitySystemComponent* ANiosCore_GASAICharacter::GetNiosASC_Implementation() const
{
    return AbilitySystemComponent;
}

UAbilitySystemComponent* ANiosCore_GASAICharacter::GetAbilitySystemComponentGeneric_Implementation() const
{
    return AbilitySystemComponent;
}

UNiosCore_AttributeSet* ANiosCore_GASAICharacter::GetNiosAttributeSet_Implementation() const
{
    return AttributeSet;
}

UNiosCore_StatsComponent* ANiosCore_GASAICharacter::GetNiosStatsComponent_Implementation() const
{
    return StatsComponent;
}

UNiosCore_StatusEffectComponent* ANiosCore_GASAICharacter::GetStatusEffectComponentGeneric_Implementation() const
{
    return StatusEffectComponent;
}

bool ANiosCore_GASAICharacter::IsAlive_Implementation() const
{
    if (bDead)
    {
        return false;
    }

    if (!AttributeSet)
    {
        return true;
    }

    const float Health = AttributeSet->GetHealth();
    return Health > 0.f || bTreatZeroHealthAsAliveForTesting;
}

bool ANiosCore_GASAICharacter::CanBeTargetedBy_Implementation(AActor* SourceActor) const
{
    return bCanBeTargeted && !bImmortal && SourceActor != this && IsAlive_Implementation();
}

FName ANiosCore_GASAICharacter::GetTeamID_Implementation() const
{
    return ResolveTeamIDFromRelation();
}

void ANiosCore_GASAICharacter::HandleDeath_Implementation(AActor* Killer)
{
    const bool bWasDead = bDead;
    bDead = true;

    // Death movement lock must be idempotent. Self-damage and replicated health
    // updates can mark the actor dead before this final pawn lifecycle call.
    ApplyDeathMovementState(true);

    if (bWasDead)
    {
        return;
    }

    BP_OnDeath(Killer);
}

void ANiosCore_GASAICharacter::HandleRevive()
{
    const bool bWasDead = bDead;
    bDead = false;

    if (HasAuthority() && StatusEffectComponent)
    {
        StatusEffectComponent->RefreshPersistentEffectsAfterRevive();
    }

    ApplyDeathMovementState(false);

    if (!bWasDead)
    {
        return;
    }

    BP_OnRevive();
}

void ANiosCore_GASAICharacter::OnRep_Dead()
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

void ANiosCore_GASAICharacter::ApplyDeathMovementState(bool bInDeadState)
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

FName ANiosCore_GASAICharacter::ResolveTeamIDFromRelation() const
{
    switch (RelationToPlayer)
    {
    case ENiosCoreTargetRelation::Friendly:
        return NiosCoreTeams::Friendly;
    case ENiosCoreTargetRelation::Enemy:
        return NiosCoreTeams::Enemy;
    case ENiosCoreTargetRelation::Neutral:
    default:
        return NiosCoreTeams::Neutral;
    }
}

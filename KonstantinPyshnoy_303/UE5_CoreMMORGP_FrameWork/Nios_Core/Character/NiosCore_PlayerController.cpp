#include "Character/NiosCore_PlayerController.h"

#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "Character/NiosCore_Character.h"
#include "Character/NiosCore_GraveyardActor.h"
#include "Character/NiosCore_PlayerState.h"
#include "Data/Respawn/NiosCore_GraveyardRegistryDataAsset.h"
#include "Components/Targeting/NiosCore_GroundTargetingComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerState.h"
#include "Kismet/GameplayStatics.h"

namespace
{
    static bool Nios_IsPawnDeadForRespawn(APawn* Pawn, const UNiosCore_AbilitySystemComponent* ASC)
    {
        if (!Pawn)
        {
            return false;
        }

        if (const ANiosCore_Character* NiosCharacter = Cast<ANiosCore_Character>(Pawn))
        {
            if (NiosCharacter->IsDead())
            {
                return true;
            }
        }

        if (ASC)
        {
            float Health = 0.f;
            if (ASC->GetAttributeValue(UNiosCore_AttributeSet::GetHealthAttribute(), Health) && Health <= 0.f)
            {
                return true;
            }
        }

        return false;
    }

    static UNiosCore_AbilitySystemComponent* Nios_ResolveNiosASC(APlayerController* PC)
    {
        if (!PC)
        {
            return nullptr;
        }

        UAbilitySystemComponent* RawASC = nullptr;

        if (PC->PlayerState)
        {
            if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC->PlayerState))
            {
                RawASC = ASI->GetAbilitySystemComponent();
            }
        }

        if (!RawASC && PC->GetPawn())
        {
            if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PC->GetPawn()))
            {
                RawASC = ASI->GetAbilitySystemComponent();
            }
        }

        return Cast<UNiosCore_AbilitySystemComponent>(RawASC);
    }

    static FName Nios_GetRespawnPointName(AActor* Actor)
    {
        if (const ANiosCore_GraveyardActor* Graveyard = Cast<ANiosCore_GraveyardActor>(Actor))
        {
            if (!Graveyard->GetGraveyardID().IsNone())
            {
                return Graveyard->GetGraveyardID();
            }
        }

        return Actor ? FName(*Actor->GetName()) : NAME_None;
    }
}

ANiosCore_PlayerController::ANiosCore_PlayerController()
{
    GroundTargetingComponent = CreateDefaultSubobject<UNiosCore_GroundTargetingComponent>(TEXT("GroundTargetingComponent"));
}

void ANiosCore_PlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    RefreshDeathMovementInputLock();
}

void ANiosCore_PlayerController::RefreshDeathMovementInputLock()
{
    APawn* ControlledPawn = GetPawn();
    UNiosCore_AbilitySystemComponent* NiosASC = Nios_ResolveNiosASC(this);
    const bool bShouldLockMovement = Nios_IsPawnDeadForRespawn(ControlledPawn, NiosASC);

    if (bShouldLockMovement)
    {
        if (!bDeathMoveInputLocked)
        {
            SetIgnoreMoveInput(true);
            bDeathMoveInputLocked = true;
        }

        // Owning-client safety net for self-damage deaths. If pawn OnRep_Dead is delayed or
        // death was resolved through PlayerState-owned ASC, this still kills local prediction.
        if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
        {
            if (UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement())
            {
                MovementComponent->StopMovementImmediately();
                MovementComponent->Velocity = FVector::ZeroVector;
                MovementComponent->DisableMovement();
            }
        }
        return;
    }

    if (bDeathMoveInputLocked)
    {
        SetIgnoreMoveInput(false);
        bDeathMoveInputLocked = false;

        if (ACharacter* ControlledCharacter = Cast<ACharacter>(ControlledPawn))
        {
            if (UCharacterMovementComponent* MovementComponent = ControlledCharacter->GetCharacterMovement())
            {
                if (MovementComponent->MovementMode == MOVE_None)
                {
                    MovementComponent->SetMovementMode(MOVE_Walking);
                }
            }
        }
    }
}

void ANiosCore_PlayerController::TryActivateGroundSkill(FName SkillID, FVector TargetLocation)
{
    if (SkillID.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> GROUND SKILL REQUEST FAILED: SkillID=None PC=%s"), *GetNameSafe(this));
        return;
    }

    if (!HasAuthority())
    {
        Server_TryActivateGroundSkill(SkillID, TargetLocation);
        return;
    }

    Server_TryActivateGroundSkill_Implementation(SkillID, TargetLocation);
}

void ANiosCore_PlayerController::Server_TryActivateGroundSkill_Implementation(FName SkillID, FVector TargetLocation)
{
    UE_LOG(LogTemp, Warning, TEXT(">>> GROUND SKILL SERVER REQUEST: SkillID=%s Location=%s PC=%s"),
        *SkillID.ToString(),
        *TargetLocation.ToString(),
        *GetNameSafe(this));

    UAbilitySystemComponent* RawASC = nullptr;

    if (PlayerState)
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PlayerState))
        {
            RawASC = ASI->GetAbilitySystemComponent();
        }
    }

    if (!RawASC && GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetPawn()))
        {
            RawASC = ASI->GetAbilitySystemComponent();
        }
    }

    UNiosCore_AbilitySystemComponent* NiosASC = Cast<UNiosCore_AbilitySystemComponent>(RawASC);
    if (!NiosASC)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> GROUND SKILL SERVER FAILED: Missing NiosASC SkillID=%s PC=%s"),
            *SkillID.ToString(),
            *GetNameSafe(this));
        return;
    }

    // This function may already exist from the ground skill activation path patch.
    // If it does not exist in your current ASC yet, add it there next:
    // Server stores pending ground location on ASC/ability context and activates the granted skill by SkillID.
    NiosASC->TryActivateSkillByIDAtLocation(SkillID, TargetLocation);
}


void ANiosCore_PlayerController::SetPvPEnabled(bool bNewPvPEnabled)
{
    if (ANiosCore_PlayerState* NiosPlayerState = GetPlayerState<ANiosCore_PlayerState>())
    {
        NiosPlayerState->SetPvPEnabled(bNewPvPEnabled);
    }
}

void ANiosCore_PlayerController::TogglePvPEnabled()
{
    SetPvPEnabled(!IsPvPEnabled());
}

bool ANiosCore_PlayerController::IsPvPEnabled() const
{
    const ANiosCore_PlayerState* NiosPlayerState = GetPlayerState<ANiosCore_PlayerState>();
    return NiosPlayerState ? NiosPlayerState->IsPvPEnabled() : false;
}

void ANiosCore_PlayerController::RequestRespawnAtNearestGraveyard()
{
    if (!bAllowRespawnAtNearestGraveyard)
    {
        BroadcastRespawnAtGraveyardResult(false, ENiosCoreRespawnRequestResult::Disabled, NAME_None);
        return;
    }

    if (!HasAuthority())
    {
        Server_RequestRespawnAtNearestGraveyard();
        return;
    }

    Server_RequestRespawnAtNearestGraveyard_Implementation();
}

void ANiosCore_PlayerController::Server_RequestRespawnAtNearestGraveyard_Implementation()
{
    if (!bAllowRespawnAtNearestGraveyard)
    {
        Client_ReceiveRespawnAtGraveyardResult(false, ENiosCoreRespawnRequestResult::Disabled, NAME_None);
        return;
    }

    APawn* DeadPawn = GetPawn();
    if (!DeadPawn)
    {
        Client_ReceiveRespawnAtGraveyardResult(false, ENiosCoreRespawnRequestResult::MissingPawn, NAME_None);
        return;
    }

    UNiosCore_AbilitySystemComponent* NiosASC = Nios_ResolveNiosASC(this);
    if (!NiosASC)
    {
        Client_ReceiveRespawnAtGraveyardResult(false, ENiosCoreRespawnRequestResult::MissingAbilitySystem, NAME_None);
        return;
    }

    if (!Nios_IsPawnDeadForRespawn(DeadPawn, NiosASC))
    {
        Client_ReceiveRespawnAtGraveyardResult(false, ENiosCoreRespawnRequestResult::NotDead, NAME_None);
        return;
    }

    float MaxHealth = 0.f;
    if (!NiosASC->GetAttributeValue(UNiosCore_AttributeSet::GetMaxHealthAttribute(), MaxHealth) || MaxHealth <= 0.f)
    {
        Client_ReceiveRespawnAtGraveyardResult(false, ENiosCoreRespawnRequestResult::InvalidHealthState, NAME_None);
        return;
    }

    FTransform RespawnTransform;
    FName RespawnPointName = NAME_None;
    if (!FindNearestRespawnTransform(DeadPawn, RespawnTransform, RespawnPointName))
    {
        Client_ReceiveRespawnAtGraveyardResult(false, ENiosCoreRespawnRequestResult::NoRespawnPoint, NAME_None);
        return;
    }

    if (UCharacterMovementComponent* Movement = DeadPawn->FindComponentByClass<UCharacterMovementComponent>())
    {
        Movement->StopMovementImmediately();
    }

    const FVector RespawnLocation = RespawnTransform.GetLocation();
    const FRotator RespawnRotation = RespawnTransform.Rotator();

    const bool bTeleported = DeadPawn->TeleportTo(RespawnLocation, RespawnRotation, false, true);
    if (!bTeleported)
    {
        DeadPawn->SetActorLocationAndRotation(RespawnLocation, RespawnRotation, false, nullptr, ETeleportType::TeleportPhysics);
    }

    SetControlRotation(RespawnRotation);

    const float DesiredHealth = FMath::Clamp(
        FMath::Max(GraveyardRespawnMinimumHealth, MaxHealth * GraveyardRespawnHealthPercent),
        1.f,
        MaxHealth);

    // This authoritative write will trigger HandleRevive() through ASC death lifecycle if the pawn was dead.
    NiosASC->SetAttributeValue(UNiosCore_AttributeSet::GetHealthAttribute(), DesiredHealth);

    DeadPawn->ForceNetUpdate();
    Client_ReceiveRespawnAtGraveyardResult(true, ENiosCoreRespawnRequestResult::Success, RespawnPointName);

    UE_LOG(LogTemp, Log, TEXT("Nios respawn: Player=%s Pawn=%s Point=%s Location=%s Health=%.1f/%.1f"),
        *GetNameSafe(this),
        *GetNameSafe(DeadPawn),
        *RespawnPointName.ToString(),
        *RespawnLocation.ToString(),
        DesiredHealth,
        MaxHealth);
}

bool ANiosCore_PlayerController::FindNearestRespawnTransform(APawn* DeadPawn, FTransform& OutTransform, FName& OutRespawnPointName)
{
    UWorld* World = GetWorld();
    if (!World || !DeadPawn)
    {
        return false;
    }

    const FVector Origin = DeadPawn->GetActorLocation();

    if (bPreferGraveyardRegistry && GraveyardRegistry)
    {
        if (GraveyardRegistry->FindNearestGraveyard(this, Origin, DeadPawn, GraveyardSearchRadius, OutTransform, OutRespawnPointName))
        {
            return true;
        }
    }

    if (bAllowLoadedGraveyardActorFallback)
    {
        const float SearchRadiusSq = GraveyardSearchRadius > 0.f ? FMath::Square(GraveyardSearchRadius) : TNumericLimits<float>::Max();

        AActor* BestActor = nullptr;
        float BestDistSq = TNumericLimits<float>::Max();
        FTransform BestTransform;

        auto ConsiderActor = [&](AActor* Candidate, const FTransform& CandidateTransform)
        {
            if (!Candidate || Candidate->IsPendingKillPending())
            {
                return;
            }

            const float DistSq = FVector::DistSquared(Origin, CandidateTransform.GetLocation());
            if (DistSq > SearchRadiusSq || DistSq >= BestDistSq)
            {
                return;
            }

            BestDistSq = DistSq;
            BestActor = Candidate;
            BestTransform = CandidateTransform;
        };

        TArray<AActor*> GraveyardActors;
        UGameplayStatics::GetAllActorsOfClass(World, ANiosCore_GraveyardActor::StaticClass(), GraveyardActors);
        for (AActor* Actor : GraveyardActors)
        {
            ANiosCore_GraveyardActor* Graveyard = Cast<ANiosCore_GraveyardActor>(Actor);
            if (!Graveyard || !Graveyard->IsRespawnEnabled())
            {
                continue;
            }

            ConsiderActor(Graveyard, Graveyard->GetRespawnTransform(DeadPawn));
        }

        if (!GraveyardActorTag.IsNone())
        {
            TArray<AActor*> TaggedActors;
            UGameplayStatics::GetAllActorsWithTag(World, GraveyardActorTag, TaggedActors);
            for (AActor* Actor : TaggedActors)
            {
                if (!Actor || Actor->IsA<ANiosCore_GraveyardActor>())
                {
                    continue;
                }

                ConsiderActor(Actor, Actor->GetActorTransform());
            }
        }

        if (BestActor)
        {
            OutTransform = BestTransform;
            OutRespawnPointName = Nios_GetRespawnPointName(BestActor);
            return true;
        }
    }

    if (bFallbackToPlayerStartWhenNoGraveyard)
    {
        const float SearchRadiusSq = GraveyardSearchRadius > 0.f ? FMath::Square(GraveyardSearchRadius) : TNumericLimits<float>::Max();

        APlayerStart* BestPlayerStart = nullptr;
        float BestDistSq = TNumericLimits<float>::Max();

        TArray<AActor*> PlayerStarts;
        UGameplayStatics::GetAllActorsOfClass(World, APlayerStart::StaticClass(), PlayerStarts);
        for (AActor* Actor : PlayerStarts)
        {
            APlayerStart* PlayerStart = Cast<APlayerStart>(Actor);
            if (!PlayerStart || PlayerStart->IsPendingKillPending())
            {
                continue;
            }

            const float DistSq = FVector::DistSquared(Origin, PlayerStart->GetActorLocation());
            if (DistSq > SearchRadiusSq || DistSq >= BestDistSq)
            {
                continue;
            }

            BestDistSq = DistSq;
            BestPlayerStart = PlayerStart;
        }

        if (BestPlayerStart)
        {
            OutTransform = BestPlayerStart->GetActorTransform();
            OutRespawnPointName = FName(*BestPlayerStart->GetName());
            return true;
        }

        // Last resort: preserve vanilla GameMode behavior for projects that override FindPlayerStart.
        if (AGameModeBase* GameMode = World->GetAuthGameMode<AGameModeBase>())
        {
            if (AActor* FallbackPlayerStart = GameMode->FindPlayerStart(this))
            {
                OutTransform = FallbackPlayerStart->GetActorTransform();
                OutRespawnPointName = FName(*FallbackPlayerStart->GetName());
                return true;
            }
        }
    }

    return false;
}

void ANiosCore_PlayerController::Client_ReceiveRespawnAtGraveyardResult_Implementation(bool bSuccess, ENiosCoreRespawnRequestResult Result, FName RespawnPointName)
{
    BroadcastRespawnAtGraveyardResult(bSuccess, Result, RespawnPointName);
}

void ANiosCore_PlayerController::BroadcastRespawnAtGraveyardResult(bool bSuccess, ENiosCoreRespawnRequestResult Result, FName RespawnPointName)
{
    OnRespawnAtGraveyardResult.Broadcast(bSuccess, Result, RespawnPointName);
}

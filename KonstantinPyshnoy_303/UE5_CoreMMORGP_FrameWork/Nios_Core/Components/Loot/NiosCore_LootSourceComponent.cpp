#include "Components/Loot/NiosCore_LootSourceComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Components/Inventory/NiosCore_InventoryComponent.h"
#include "Data/Loot/NiosCore_LootRulesDataAsset.h"
#include "Data/Loot/NiosCore_LootTableDataAsset.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

namespace
{
    bool Nios_Loot_RarityAtLeast(ENiosCoreLootItemRarity Value, ENiosCoreLootItemRarity Threshold)
    {
        return static_cast<uint8>(Value) >= static_cast<uint8>(Threshold);
    }
}

UNiosCore_LootSourceComponent::UNiosCore_LootSourceComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UNiosCore_LootSourceComponent::BeginPlay()
{
    Super::BeginPlay();

    if (bGenerateLootOnBeginPlay && GetOwner() && GetOwner()->HasAuthority())
    {
        GenerateLoot(false);
    }
}

void UNiosCore_LootSourceComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UNiosCore_LootSourceComponent, LootSlots);
    DOREPLIFETIME(UNiosCore_LootSourceComponent, bLootGenerated);
    DOREPLIFETIME(UNiosCore_LootSourceComponent, bLootOwnershipResolved);
    DOREPLIFETIME(UNiosCore_LootSourceComponent, bLootResolvedAsPublic);
    DOREPLIFETIME(UNiosCore_LootSourceComponent, ResolvedLootOwnerUnit);
}

void UNiosCore_LootSourceComponent::ReportDamageForLoot(AActor* DamagedActor, AActor* DamageSourceActor, float DamageAmount)
{
    if (!DamagedActor || DamageAmount <= 0.f)
    {
        return;
    }

    if (UNiosCore_LootSourceComponent* LootSource = DamagedActor->FindComponentByClass<UNiosCore_LootSourceComponent>())
    {
        LootSource->RecordDamageContribution(DamageSourceActor, DamageAmount);
    }
}

void UNiosCore_LootSourceComponent::NotifyActorDiedForLoot(AActor* DeadActor)
{
    if (!DeadActor)
    {
        return;
    }

    if (UNiosCore_LootSourceComponent* LootSource = DeadActor->FindComponentByClass<UNiosCore_LootSourceComponent>())
    {
        LootSource->FinalizeLootOnDeath();
    }
}

FNiosCoreLootRules UNiosCore_LootSourceComponent::GetEffectiveLootRules() const
{
    if (LootRulesAsset)
    {
        return LootRulesAsset->Rules;
    }

    return InlineLootRules;
}

FName UNiosCore_LootSourceComponent::BP_GetLootUnitID_Implementation(APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return NAME_None;
    }

    const int32 PlayerID = PlayerState->GetPlayerId();
    if (PlayerID >= 0)
    {
        return FName(*FString::Printf(TEXT("Solo_%d"), PlayerID));
    }

    return FName(*FString::Printf(TEXT("Solo_%s"), *GetNameSafe(PlayerState)));
}

ENiosCoreLootUnitType UNiosCore_LootSourceComponent::BP_GetLootUnitType_Implementation(APlayerState* PlayerState) const
{
    return ENiosCoreLootUnitType::Solo;
}

APlayerState* UNiosCore_LootSourceComponent::BP_GetLootUnitLeader_Implementation(APlayerState* PlayerState) const
{
    return PlayerState;
}

TArray<APlayerState*> UNiosCore_LootSourceComponent::BP_GetLootUnitMembers_Implementation(APlayerState* PlayerState) const
{
    TArray<APlayerState*> Members;
    if (PlayerState)
    {
        Members.Add(PlayerState);
    }
    return Members;
}

bool UNiosCore_LootSourceComponent::TryResolvePlayerState(AActor* Actor, APlayerState*& OutPlayerState) const
{
    OutPlayerState = nullptr;

    if (!Actor)
    {
        return false;
    }

    if (APlayerState* DirectPlayerState = Cast<APlayerState>(Actor))
    {
        OutPlayerState = DirectPlayerState;
        return true;
    }

    if (APawn* Pawn = Cast<APawn>(Actor))
    {
        if (APlayerState* PawnPlayerState = Pawn->GetPlayerState())
        {
            OutPlayerState = PawnPlayerState;
            return true;
        }

        if (AController* PawnController = Pawn->GetController())
        {
            if (APlayerState* ControllerPlayerState = PawnController->PlayerState)
            {
                OutPlayerState = ControllerPlayerState;
                return true;
            }
        }
    }

    if (AController* Controller = Cast<AController>(Actor))
    {
        if (APlayerState* ControllerPlayerState = Controller->PlayerState)
        {
            OutPlayerState = ControllerPlayerState;
            return true;
        }
    }

    if (AController* InstigatorController = Actor->GetInstigatorController())
    {
        if (APlayerState* ControllerPlayerState = InstigatorController->PlayerState)
        {
            OutPlayerState = ControllerPlayerState;
            return true;
        }
    }

    if (APawn* InstigatorPawn = Actor->GetInstigator())
    {
        if (APlayerState* InstigatorPlayerState = InstigatorPawn->GetPlayerState())
        {
            OutPlayerState = InstigatorPlayerState;
            return true;
        }
    }

    return false;
}

bool UNiosCore_LootSourceComponent::ResolveLootUnitForPlayerState(APlayerState* PlayerState, FNiosCoreLootUnitSnapshot& OutUnit) const
{
    OutUnit = FNiosCoreLootUnitSnapshot();

    if (!PlayerState)
    {
        return false;
    }

    OutUnit.UnitID = BP_GetLootUnitID(PlayerState);
    OutUnit.UnitType = BP_GetLootUnitType(PlayerState);
    OutUnit.Leader = BP_GetLootUnitLeader(PlayerState);

    const TArray<APlayerState*> RawMembers = BP_GetLootUnitMembers(PlayerState);
    for (APlayerState* Member : RawMembers)
    {
        if (!Member)
        {
            continue;
        }

        bool bAlreadyAdded = false;
        for (APlayerState* ExistingMember : OutUnit.Members)
        {
            if (ExistingMember == Member)
            {
                bAlreadyAdded = true;
                break;
            }
        }

        if (!bAlreadyAdded)
        {
            OutUnit.Members.Add(Member);
        }
    }

    if (OutUnit.Members.Num() <= 0)
    {
        OutUnit.Members.Add(PlayerState);
    }

    if (!OutUnit.Leader)
    {
        OutUnit.Leader = PlayerState;
    }

    if (OutUnit.UnitID.IsNone())
    {
        OutUnit.UnitID = BP_GetLootUnitID(PlayerState);
    }

    return OutUnit.IsValidUnit();
}

void UNiosCore_LootSourceComponent::RecordDamageContribution(AActor* DamageSourceActor, float DamageAmount)
{
    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority() || DamageAmount <= 0.f)
    {
        return;
    }

    APlayerState* DamagePlayerState = nullptr;
    if (!TryResolvePlayerState(DamageSourceActor, DamagePlayerState) || !DamagePlayerState)
    {
        return;
    }

    FNiosCoreLootUnitSnapshot DamageUnit;
    if (!ResolveLootUnitForPlayerState(DamagePlayerState, DamageUnit))
    {
        return;
    }

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

    FNiosCoreLootContributionRecord* ExistingRecord = ContributionRecords.FindByPredicate(
        [DamagePlayerState, &DamageUnit](const FNiosCoreLootContributionRecord& Candidate)
        {
            return Candidate.PlayerState == DamagePlayerState && Candidate.LootUnit.UnitID == DamageUnit.UnitID;
        });

    if (ExistingRecord)
    {
        ExistingRecord->DamageAmount += DamageAmount;
        ExistingRecord->LastDamageTime = CurrentTime;
        ExistingRecord->LootUnit = DamageUnit;
        return;
    }

    FNiosCoreLootContributionRecord NewRecord;
    NewRecord.PlayerState = DamagePlayerState;
    NewRecord.LootUnit = DamageUnit;
    NewRecord.DamageAmount = DamageAmount;
    NewRecord.FirstDamageTime = CurrentTime;
    NewRecord.LastDamageTime = CurrentTime;
    ContributionRecords.Add(NewRecord);
}

AActor* UNiosCore_LootSourceComponent::ResolvePlayerPhysicalActor(APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return nullptr;
    }

    if (AController* Controller = Cast<AController>(PlayerState->GetOwner()))
    {
        if (APawn* ControlledPawn = Controller->GetPawn())
        {
            return ControlledPawn;
        }
    }

    return PlayerState;
}

AActor* UNiosCore_LootSourceComponent::ResolveLootingPhysicalActor(AActor* InventoryOwnerActor) const
{
    if (!InventoryOwnerActor)
    {
        return nullptr;
    }

    if (APawn* Pawn = Cast<APawn>(InventoryOwnerActor))
    {
        return Pawn;
    }

    if (APlayerState* PlayerState = Cast<APlayerState>(InventoryOwnerActor))
    {
        return ResolvePlayerPhysicalActor(PlayerState);
    }

    if (AController* Controller = Cast<AController>(InventoryOwnerActor))
    {
        return Controller->GetPawn();
    }

    return InventoryOwnerActor;
}

bool UNiosCore_LootSourceComponent::IsOwnerAliveForLootInteraction() const
{
    const AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
    {
        return false;
    }

    return INiosGASActorInterface::Execute_IsAlive(const_cast<AActor*>(SourceOwner));
}

bool UNiosCore_LootSourceComponent::CanReachLootSource(AActor* LootingActor) const
{
    const AActor* SourceOwner = GetOwner();
    const AActor* PhysicalLootingActor = ResolveLootingPhysicalActor(LootingActor);

    if (!SourceOwner || !PhysicalLootingActor)
    {
        return false;
    }

    if (LootInteractionRange <= 0.f)
    {
        return true;
    }

    return FVector::DistSquared(SourceOwner->GetActorLocation(), PhysicalLootingActor->GetActorLocation()) <= FMath::Square(LootInteractionRange);
}

bool UNiosCore_LootSourceComponent::IsPlayerStateCurrentlyValidForOwnership(APlayerState* PlayerState, const FNiosCoreLootRules& Rules) const
{
    if (!PlayerState)
    {
        return false;
    }

    AActor* PhysicalActor = ResolvePlayerPhysicalActor(PlayerState);
    if (!PhysicalActor)
    {
        return false;
    }

    if (Rules.bSkipDeadOwnershipUnits)
    {
        bool bAlive = true;
        if (PhysicalActor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
        {
            bAlive = INiosGASActorInterface::Execute_IsAlive(PhysicalActor);
        }
        else if (PlayerState->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
        {
            bAlive = INiosGASActorInterface::Execute_IsAlive(PlayerState);
        }

        if (!bAlive)
        {
            return false;
        }
    }

    if (Rules.bSkipOutOfRangeOwnershipUnits && Rules.OwnershipPresenceRadius > 0.f)
    {
        const AActor* SourceOwner = GetOwner();
        if (!SourceOwner)
        {
            return false;
        }

        if (FVector::DistSquared(SourceOwner->GetActorLocation(), PhysicalActor->GetActorLocation()) > FMath::Square(Rules.OwnershipPresenceRadius))
        {
            return false;
        }
    }

    return true;
}

bool UNiosCore_LootSourceComponent::IsLootUnitCurrentlyEligible(const FNiosCoreLootUnitSnapshot& Unit, const FNiosCoreLootRules& Rules) const
{
    if (!Unit.IsValidUnit())
    {
        return false;
    }

    for (APlayerState* Member : Unit.Members)
    {
        if (IsPlayerStateCurrentlyValidForOwnership(Member, Rules))
        {
            return true;
        }
    }

    return false;
}

bool UNiosCore_LootSourceComponent::ResolveLootOwnership(AActor* OptionalInteractor, bool bForceReResolve)
{
    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority())
    {
        return false;
    }

    const FNiosCoreLootRules Rules = GetEffectiveLootRules();
    if (bLootOwnershipResolved && Rules.bLockOwnershipWhenResolved && !bForceReResolve)
    {
        return true;
    }

    ResolvedLootOwnerUnit = FNiosCoreLootUnitSnapshot();
    bLootOwnershipResolved = false;
    bLootResolvedAsPublic = false;

    if (Rules.OwnershipRule == ENiosCoreLootOwnershipRule::Public)
    {
        bLootOwnershipResolved = true;
        bLootResolvedAsPublic = true;
        MarkChanged();
        return true;
    }

    if (Rules.OwnershipRule == ENiosCoreLootOwnershipRule::FirstInteractorUnit)
    {
        APlayerState* InteractorPlayerState = nullptr;
        FNiosCoreLootUnitSnapshot InteractorUnit;
        if (TryResolvePlayerState(OptionalInteractor, InteractorPlayerState) && ResolveLootUnitForPlayerState(InteractorPlayerState, InteractorUnit) &&
            IsLootUnitCurrentlyEligible(InteractorUnit, Rules))
        {
            ResolvedLootOwnerUnit = InteractorUnit;
            bLootOwnershipResolved = true;
            if (bLootGenerated)
            {
                AssignDistributionReservations();
            }
            MarkChanged();
            return true;
        }

        if (Rules.bFallbackToPublicWhenNoValidOwner)
        {
            bLootOwnershipResolved = true;
            bLootResolvedAsPublic = true;
            MarkChanged();
            return true;
        }

        return false;
    }

    struct FNiosLootCandidate
    {
        FNiosCoreLootUnitSnapshot Unit;
        float Damage = 0.f;
        float FirstDamageTime = 0.f;
    };

    TArray<FNiosLootCandidate> Candidates;

    for (const FNiosCoreLootContributionRecord& Record : ContributionRecords)
    {
        if (!Record.LootUnit.IsValidUnit() || Record.DamageAmount <= 0.f)
        {
            continue;
        }

        FNiosLootCandidate* ExistingCandidate = Candidates.FindByPredicate(
            [&Record](const FNiosLootCandidate& Candidate)
            {
                return Candidate.Unit.UnitID == Record.LootUnit.UnitID;
            });

        if (ExistingCandidate)
        {
            ExistingCandidate->Damage += Record.DamageAmount;
            ExistingCandidate->FirstDamageTime = FMath::Min(ExistingCandidate->FirstDamageTime, Record.FirstDamageTime);
            ExistingCandidate->Unit = Record.LootUnit;
        }
        else
        {
            FNiosLootCandidate NewCandidate;
            NewCandidate.Unit = Record.LootUnit;
            NewCandidate.Damage = Record.DamageAmount;
            NewCandidate.FirstDamageTime = Record.FirstDamageTime;
            Candidates.Add(NewCandidate);
        }
    }

    if (Rules.OwnershipRule == ENiosCoreLootOwnershipRule::FirstDamageUnit)
    {
        Candidates.Sort(
            [](const FNiosLootCandidate& A, const FNiosLootCandidate& B)
            {
                return A.FirstDamageTime < B.FirstDamageTime;
            });
    }
    else
    {
        Candidates.Sort(
            [](const FNiosLootCandidate& A, const FNiosLootCandidate& B)
            {
                if (!FMath::IsNearlyEqual(A.Damage, B.Damage))
                {
                    return A.Damage > B.Damage;
                }
                return A.FirstDamageTime < B.FirstDamageTime;
            });
    }

    for (const FNiosLootCandidate& Candidate : Candidates)
    {
        if (IsLootUnitCurrentlyEligible(Candidate.Unit, Rules))
        {
            ResolvedLootOwnerUnit = Candidate.Unit;
            bLootOwnershipResolved = true;
            if (bLootGenerated)
            {
                AssignDistributionReservations();
            }
            MarkChanged();
            return true;
        }
    }

    if (Rules.bFallbackToPublicWhenNoValidOwner)
    {
        bLootOwnershipResolved = true;
        bLootResolvedAsPublic = true;
        MarkChanged();
        return true;
    }

    return false;
}

void UNiosCore_LootSourceComponent::FinalizeLootOnDeath()
{
    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority())
    {
        return;
    }

    ResolveLootOwnership(nullptr, false);

    if (!bLootGenerated)
    {
        GenerateLoot(false);
    }
}

void UNiosCore_LootSourceComponent::BuildEntries(TArray<FNiosCoreLootEntry>& OutEntries) const
{
    OutEntries.Reset();

    const bool bUseDataSettingsLoot =
        LootItemsMode == ENiosCoreLootItemsMode::DataSettingsLoot ||
        LootItemsMode == ENiosCoreLootItemsMode::DataSettingsAndCustomLoot;

    const bool bUseCustomLoot =
        LootItemsMode == ENiosCoreLootItemsMode::CustomLoot ||
        LootItemsMode == ENiosCoreLootItemsMode::DataSettingsAndCustomLoot;

    if (bUseDataSettingsLoot && LootTable)
    {
        OutEntries.Append(LootTable->Entries);
    }

    if (bUseCustomLoot)
    {
        OutEntries.Append(InlineLootEntries);
    }
}

void UNiosCore_LootSourceComponent::GenerateLoot(bool bForceRegenerate)
{
    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority())
    {
        return;
    }

    if (bLootGenerated && bGenerateOnlyOnce && !bForceRegenerate)
    {
        return;
    }

    TArray<FNiosCoreLootEntry> Entries;
    BuildEntries(Entries);

    LootSlots.Reset();

    int32 NextSlotIndex = 0;
    for (const FNiosCoreLootEntry& Entry : Entries)
    {
        if (!Entry.IsValidEntry())
        {
            continue;
        }

        const int32 SafeRolls = FMath::Max(1, Entry.Rolls);
        const int32 SafeMin = FMath::Max(1, Entry.MinCount);
        const int32 SafeMax = FMath::Max(SafeMin, Entry.MaxCount);
        const float SafeChance = FMath::Clamp(Entry.DropChance, 0.f, 1.f);

        for (int32 RollIndex = 0; RollIndex < SafeRolls; ++RollIndex)
        {
            if (FMath::FRand() > SafeChance)
            {
                continue;
            }

            FNiosCoreLootSlot NewSlot;
            NewSlot.LootSlotIndex = NextSlotIndex++;
            NewSlot.ItemID = Entry.ItemID;
            NewSlot.Count = FMath::RandRange(SafeMin, SafeMax);
            NewSlot.Rarity = Entry.Rarity;
            NewSlot.bLooted = false;
            LootSlots.Add(NewSlot);
        }
    }

    bLootGenerated = true;
    AssignDistributionReservations();
    MarkChanged();
}

void UNiosCore_LootSourceComponent::AssignDistributionReservations()
{
    const FNiosCoreLootRules Rules = GetEffectiveLootRules();

    if (bLootResolvedAsPublic)
    {
        return;
    }

    if (Rules.OwnershipRule != ENiosCoreLootOwnershipRule::Public && !ResolvedLootOwnerUnit.IsValidUnit())
    {
        return;
    }

    if (Rules.DistributionRule == ENiosCoreLootDistributionRule::FreeForAllInsideOwnerUnit)
    {
        return;
    }

    if (Rules.DistributionRule == ENiosCoreLootDistributionRule::LeaderOnly)
    {
        for (FNiosCoreLootSlot& LootSlot : LootSlots)
        {
            if (!LootSlot.IsEmpty())
            {
                LootSlot.ReservedForPlayerState = ResolvedLootOwnerUnit.Leader;
            }
        }
        return;
    }

    for (FNiosCoreLootSlot& LootSlot : LootSlots)
    {
        if (LootSlot.IsEmpty())
        {
            continue;
        }

        if (Rules.DistributionRule == ENiosCoreLootDistributionRule::RoundRobinWithVoteThreshold &&
            Nios_Loot_RarityAtLeast(LootSlot.Rarity, Rules.VoteRarityThreshold))
        {
            LootSlot.VoteState = ENiosCoreLootVoteState::PendingVote;
            LootSlot.ReservedForPlayerState = Rules.bFallbackVoteItemsToLeaderUntilVoteSystem ? ResolvedLootOwnerUnit.Leader : nullptr;
            continue;
        }

        LootSlot.ReservedForPlayerState = GetNextEligibleRoundRobinMember();
    }
}

APlayerState* UNiosCore_LootSourceComponent::GetNextEligibleRoundRobinMember()
{
    const FNiosCoreLootRules Rules = GetEffectiveLootRules();
    if (!ResolvedLootOwnerUnit.IsValidUnit())
    {
        return nullptr;
    }

    const int32 MemberCount = ResolvedLootOwnerUnit.Members.Num();
    if (MemberCount <= 0)
    {
        return nullptr;
    }

    for (int32 Attempt = 0; Attempt < MemberCount; ++Attempt)
    {
        const int32 MemberIndex = (RoundRobinCursor + Attempt) % MemberCount;
        APlayerState* Candidate = ResolvedLootOwnerUnit.Members[MemberIndex];
        if (IsPlayerStateCurrentlyValidForOwnership(Candidate, Rules))
        {
            RoundRobinCursor = (MemberIndex + 1) % MemberCount;
            return Candidate;
        }
    }

    return ResolvedLootOwnerUnit.Leader;
}

APlayerState* UNiosCore_LootSourceComponent::ResolveSlotReservation(FNiosCoreLootSlot& LootSlot)
{
    const FNiosCoreLootRules Rules = GetEffectiveLootRules();

    if (!LootSlot.ReservedForPlayerState)
    {
        return nullptr;
    }

    if (IsPlayerStateCurrentlyValidForOwnership(LootSlot.ReservedForPlayerState, Rules))
    {
        return LootSlot.ReservedForPlayerState;
    }

    APlayerState* Replacement = GetNextEligibleRoundRobinMember();
    LootSlot.ReservedForPlayerState = Replacement;
    return Replacement;
}

bool UNiosCore_LootSourceComponent::HasLoot() const
{
    for (const FNiosCoreLootSlot& Slot : LootSlots)
    {
        if (!Slot.IsEmpty())
        {
            return true;
        }
    }

    return false;
}

void UNiosCore_LootSourceComponent::EnsureLootReadyForLooting(AActor* OptionalInteractor)
{
    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority())
    {
        return;
    }

    FText AttemptReason;
    if (!CanAttemptLoot(OptionalInteractor, AttemptReason))
    {
        return;
    }

    if (!bLootOwnershipResolved)
    {
        ResolveLootOwnership(OptionalInteractor, false);
    }

    if (!bLootGenerated && bGenerateOnFirstLootRequest)
    {
        GenerateLoot(false);
    }
}

bool UNiosCore_LootSourceComponent::IsPlayerStateInResolvedOwnerUnit(APlayerState* PlayerState) const
{
    if (!PlayerState)
    {
        return false;
    }

    if (!ResolvedLootOwnerUnit.IsValidUnit())
    {
        return false;
    }

    for (APlayerState* Member : ResolvedLootOwnerUnit.Members)
    {
        if (Member == PlayerState)
        {
            return true;
        }
    }

    return false;
}

bool UNiosCore_LootSourceComponent::CanAttemptLoot(AActor* LootingActor, FText& OutReasonText) const
{
    OutReasonText = FText::GetEmpty();

    const AActor* SourceOwner = GetOwner();
    if (!SourceOwner)
    {
        OutReasonText = NSLOCTEXT("NiosLoot", "AttemptNoSourceOwner", "No loot source owner");
        return false;
    }

    if (!LootingActor)
    {
        OutReasonText = NSLOCTEXT("NiosLoot", "AttemptNoLootingActor", "No looting actor");
        return false;
    }

    if (bBlockLootWhileOwnerAlive && IsOwnerAliveForLootInteraction())
    {
        OutReasonText = NSLOCTEXT("NiosLoot", "AttemptOwnerAlive", "Target is alive");
        return false;
    }

    if (!CanReachLootSource(LootingActor))
    {
        OutReasonText = NSLOCTEXT("NiosLoot", "AttemptTooFar", "Too far");
        return false;
    }

    if (bLootGenerated && !HasLoot())
    {
        OutReasonText = NSLOCTEXT("NiosLoot", "AttemptEmpty", "Empty");
        return false;
    }

    return true;
}

bool UNiosCore_LootSourceComponent::CanAttemptLootActor(AActor* LootSourceActor, AActor* LootingActor, FText& OutReasonText)
{
    OutReasonText = FText::GetEmpty();

    if (!LootSourceActor)
    {
        OutReasonText = NSLOCTEXT("NiosLoot", "AttemptNoActor", "No loot actor");
        return false;
    }

    UNiosCore_LootSourceComponent* LootSource = LootSourceActor->FindComponentByClass<UNiosCore_LootSourceComponent>();
    if (!LootSource)
    {
        OutReasonText = NSLOCTEXT("NiosLoot", "AttemptNoLootComponent", "No loot source");
        return false;
    }

    return LootSource->CanAttemptLoot(LootingActor, OutReasonText);
}

bool UNiosCore_LootSourceComponent::CanLootSlot(AActor* LootingActor, FNiosCoreLootSlot& LootSlot)
{
    FText AttemptReason;
    if (!CanAttemptLoot(LootingActor, AttemptReason))
    {
        return false;
    }

    if (LootSlot.IsEmpty())
    {
        return false;
    }

    if (!CanReachLootSource(LootingActor))
    {
        return false;
    }

    const FNiosCoreLootRules Rules = GetEffectiveLootRules();

    if (Rules.OwnershipRule == ENiosCoreLootOwnershipRule::Public || bLootResolvedAsPublic)
    {
        return true;
    }

    if (!bLootOwnershipResolved || !ResolvedLootOwnerUnit.IsValidUnit())
    {
        return false;
    }

    APlayerState* LootingPlayerState = nullptr;
    if (!TryResolvePlayerState(LootingActor, LootingPlayerState) || !LootingPlayerState)
    {
        return false;
    }

    if (!IsPlayerStateInResolvedOwnerUnit(LootingPlayerState))
    {
        return false;
    }

    switch (Rules.DistributionRule)
    {
    case ENiosCoreLootDistributionRule::LeaderOnly:
        return LootingPlayerState == ResolvedLootOwnerUnit.Leader;

    case ENiosCoreLootDistributionRule::RoundRobin:
    {
        APlayerState* Reservation = ResolveSlotReservation(LootSlot);
        return !Reservation || Reservation == LootingPlayerState;
    }

    case ENiosCoreLootDistributionRule::RoundRobinWithVoteThreshold:
    {
        if (LootSlot.VoteState == ENiosCoreLootVoteState::PendingVote)
        {
            if (!Rules.bFallbackVoteItemsToLeaderUntilVoteSystem)
            {
                BP_OnLootVoteRequested(LootSlot.LootSlotIndex, LootSlot.ItemID, LootSlot.Rarity);
                return false;
            }

            return LootingPlayerState == ResolvedLootOwnerUnit.Leader;
        }

        if (LootSlot.VoteState == ENiosCoreLootVoteState::Awarded)
        {
            return LootSlot.ReservedForPlayerState == LootingPlayerState;
        }

        APlayerState* Reservation = ResolveSlotReservation(LootSlot);
        return !Reservation || Reservation == LootingPlayerState;
    }

    case ENiosCoreLootDistributionRule::FreeForAllInsideOwnerUnit:
    default:
        return true;
    }
}

bool UNiosCore_LootSourceComponent::CanLoot(AActor* LootingActor)
{
    FText AttemptReason;
    if (!CanAttemptLoot(LootingActor, AttemptReason))
    {
        return false;
    }

    EnsureLootReadyForLooting(LootingActor);

    for (FNiosCoreLootSlot& LootSlot : LootSlots)
    {
        if (CanLootSlot(LootingActor, LootSlot))
        {
            return true;
        }
    }

    return false;
}

bool UNiosCore_LootSourceComponent::GetLootSlotAccessInfo(AActor* LootingActor, int32 LootSlotIndex, FNiosCoreLootSlotAccessInfo& OutAccessInfo) const
{
    OutAccessInfo = FNiosCoreLootSlotAccessInfo();

    const FNiosCoreLootSlot* LootSlot = LootSlots.FindByPredicate(
        [LootSlotIndex](const FNiosCoreLootSlot& Candidate)
        {
            return Candidate.LootSlotIndex == LootSlotIndex;
        });

    if (!LootSlot || LootSlot->IsEmpty())
    {
        OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessEmpty", "Empty");
        return false;
    }

    if (!LootingActor)
    {
        OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessNoActor", "No looting actor");
        return true;
    }

    FText AttemptReason;
    if (!CanAttemptLoot(LootingActor, AttemptReason))
    {
        OutAccessInfo.ReasonText = AttemptReason;
        return true;
    }

    OutAccessInfo.bWithinRange = CanReachLootSource(LootingActor);
    if (!OutAccessInfo.bWithinRange)
    {
        OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessTooFar", "Too far");
        return true;
    }

    const FNiosCoreLootRules Rules = GetEffectiveLootRules();
    if (Rules.OwnershipRule == ENiosCoreLootOwnershipRule::Public || bLootResolvedAsPublic)
    {
        OutAccessInfo.bCanLoot = true;
        OutAccessInfo.bInOwnerUnit = true;
        return true;
    }

    if (!bLootOwnershipResolved || !ResolvedLootOwnerUnit.IsValidUnit())
    {
        OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessNoOwner", "No owner yet");
        return true;
    }

    APlayerState* LootingPlayerState = nullptr;
    if (!TryResolvePlayerState(LootingActor, LootingPlayerState) || !LootingPlayerState)
    {
        OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessNoPlayer", "No player state");
        return true;
    }

    OutAccessInfo.bInOwnerUnit = IsPlayerStateInResolvedOwnerUnit(LootingPlayerState);
    if (!OutAccessInfo.bInOwnerUnit)
    {
        OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessWrongOwner", "Not your loot");
        return true;
    }

    if (Rules.DistributionRule == ENiosCoreLootDistributionRule::LeaderOnly)
    {
        OutAccessInfo.bCanLoot = LootingPlayerState == ResolvedLootOwnerUnit.Leader;
        if (!OutAccessInfo.bCanLoot)
        {
            OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessLeaderOnly", "Leader only");
        }
        return true;
    }

    if (Rules.DistributionRule == ENiosCoreLootDistributionRule::RoundRobinWithVoteThreshold &&
        LootSlot->VoteState == ENiosCoreLootVoteState::PendingVote)
    {
        OutAccessInfo.bPendingVote = true;
        if (!Rules.bFallbackVoteItemsToLeaderUntilVoteSystem)
        {
            OutAccessInfo.bCanLoot = false;
            OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessPendingVote", "Vote pending");
            return true;
        }

        OutAccessInfo.bCanLoot = LootingPlayerState == ResolvedLootOwnerUnit.Leader;
        if (!OutAccessInfo.bCanLoot)
        {
            OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessVoteLeader", "Vote item: leader only");
        }
        return true;
    }

    if (LootSlot->ReservedForPlayerState)
    {
        OutAccessInfo.bReservedForRequester = LootSlot->ReservedForPlayerState == LootingPlayerState;
        OutAccessInfo.bReservedForOther = !OutAccessInfo.bReservedForRequester;
        OutAccessInfo.bCanLoot = OutAccessInfo.bReservedForRequester;
        if (OutAccessInfo.bReservedForOther)
        {
            OutAccessInfo.ReasonText = NSLOCTEXT("NiosLoot", "AccessReserved", "Reserved for another player");
        }
        return true;
    }

    OutAccessInfo.bCanLoot = true;
    return true;
}

bool UNiosCore_LootSourceComponent::LootSlotToInventory(
    int32 LootSlotIndex,
    UNiosCore_InventoryComponent* InventoryComponent,
    int32 Count,
    int32& OutAddedCount)
{
    OutAddedCount = 0;

    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority() || !InventoryComponent)
    {
        return false;
    }

    EnsureLootReadyForLooting(InventoryComponent->GetOwner());

    FNiosCoreLootSlot* LootSlot = LootSlots.FindByPredicate(
        [LootSlotIndex](const FNiosCoreLootSlot& Candidate)
        {
            return Candidate.LootSlotIndex == LootSlotIndex;
        });

    if (!LootSlot || !CanLootSlot(InventoryComponent->GetOwner(), *LootSlot))
    {
        return false;
    }

    const int32 RequestedCount = Count <= 0 ? LootSlot->Count : Count;
    const int32 CountToTry = FMath::Clamp(RequestedCount, 1, LootSlot->Count);

    int32 AddedCount = 0;
    if (!InventoryComponent->AddItem(LootSlot->ItemID, CountToTry, AddedCount) || AddedCount <= 0)
    {
        return false;
    }

    LootSlot->Count -= AddedCount;
    OutAddedCount = AddedCount;

    if (LootSlot->Count <= 0)
    {
        LootSlot->Clear();
    }

    MarkChanged();

    if (bDestroyOwnerWhenEmpty && !HasLoot())
    {
        SourceOwner->Destroy();
    }

    return true;
}

bool UNiosCore_LootSourceComponent::LootAllToInventory(
    UNiosCore_InventoryComponent* InventoryComponent,
    int32& OutAddedCount)
{
    OutAddedCount = 0;

    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority() || !InventoryComponent)
    {
        return false;
    }

    EnsureLootReadyForLooting(InventoryComponent->GetOwner());

    bool bAnyLooted = false;
    for (FNiosCoreLootSlot& LootSlot : LootSlots)
    {
        if (!CanLootSlot(InventoryComponent->GetOwner(), LootSlot))
        {
            continue;
        }

        int32 AddedCount = 0;
        if (!InventoryComponent->AddItem(LootSlot.ItemID, LootSlot.Count, AddedCount) || AddedCount <= 0)
        {
            continue;
        }

        LootSlot.Count -= AddedCount;
        OutAddedCount += AddedCount;
        bAnyLooted = true;

        if (LootSlot.Count <= 0)
        {
            LootSlot.Clear();
        }
    }

    if (bAnyLooted)
    {
        MarkChanged();

        if (bDestroyOwnerWhenEmpty && !HasLoot())
        {
            SourceOwner->Destroy();
        }
    }

    return bAnyLooted;
}

bool UNiosCore_LootSourceComponent::AwardLootSlotToPlayer(int32 LootSlotIndex, APlayerState* WinnerPlayerState)
{
    AActor* SourceOwner = GetOwner();
    if (!SourceOwner || !SourceOwner->HasAuthority() || !WinnerPlayerState)
    {
        return false;
    }

    if (!IsPlayerStateInResolvedOwnerUnit(WinnerPlayerState))
    {
        return false;
    }

    FNiosCoreLootSlot* LootSlot = LootSlots.FindByPredicate(
        [LootSlotIndex](const FNiosCoreLootSlot& Candidate)
        {
            return Candidate.LootSlotIndex == LootSlotIndex;
        });

    if (!LootSlot || LootSlot->IsEmpty())
    {
        return false;
    }

    LootSlot->ReservedForPlayerState = WinnerPlayerState;
    LootSlot->VoteState = ENiosCoreLootVoteState::Awarded;
    MarkChanged();
    return true;
}

void UNiosCore_LootSourceComponent::MarkChanged()
{
    OnLootChanged.Broadcast();
}

void UNiosCore_LootSourceComponent::OnRep_LootSlots()
{
    OnLootChanged.Broadcast();
}

void UNiosCore_LootSourceComponent::OnRep_LootOwnership()
{
    OnLootChanged.Broadcast();
}

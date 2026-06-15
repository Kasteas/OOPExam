#include "Data/Respawn/NiosCore_GraveyardRegistryDataAsset.h"

#include "Components/CapsuleComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameplayTagAssetInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"

namespace
{
    static FName Nios_GetCurrentShortLevelName(const UWorld* World)
    {
        if (!World)
        {
            return NAME_None;
        }

        FString MapName = World->GetMapName();
        if (!World->StreamingLevelsPrefix.IsEmpty())
        {
            MapName.RemoveFromStart(World->StreamingLevelsPrefix);
        }

        return FName(*FPackageName::GetShortName(MapName));
    }

    static bool Nios_LevelNamesMatch(FName EntryLevelName, FName CurrentLevelName)
    {
        if (EntryLevelName.IsNone())
        {
            return true;
        }

        return EntryLevelName.ToString().Equals(CurrentLevelName.ToString(), ESearchCase::IgnoreCase);
    }

    static void Nios_AppendOwnedGameplayTags(const AActor* Actor, FGameplayTagContainer& OutTags)
    {
        if (!Actor)
        {
            return;
        }

        if (const IGameplayTagAssetInterface* TagAsset = Cast<IGameplayTagAssetInterface>(Actor))
        {
            FGameplayTagContainer ActorTags;
            TagAsset->GetOwnedGameplayTags(ActorTags);
            OutTags.AppendTags(ActorTags);
        }
    }

    static FName Nios_GetEntryName(const FNiosCoreGraveyardEntry& Entry)
    {
        return Entry.GraveyardID.IsNone() ? FName(TEXT("DataGraveyard")) : Entry.GraveyardID;
    }
}

bool FNiosCoreGraveyardEntry::IsValidForLevel(FName CurrentLevelName) const
{
    return bEnabled && Nios_LevelNamesMatch(LevelName, CurrentLevelName);
}

bool FNiosCoreGraveyardEntry::IsAllowedForPawn(const APawn* RespawningPawn) const
{
    if (AccessMode == ENiosCoreGraveyardAccessMode::Everyone || WhoCanReviveHere.IsEmpty())
    {
        return true;
    }

    FGameplayTagContainer OwnedTags;
    Nios_AppendOwnedGameplayTags(RespawningPawn, OwnedTags);

    if (const APlayerState* PlayerState = RespawningPawn ? RespawningPawn->GetPlayerState() : nullptr)
    {
        Nios_AppendOwnedGameplayTags(PlayerState, OwnedTags);
    }

    switch (AccessMode)
    {
    case ENiosCoreGraveyardAccessMode::RequiresAnyGameplayTag:
        return OwnedTags.HasAnyExact(WhoCanReviveHere);

    case ENiosCoreGraveyardAccessMode::RequiresAllGameplayTags:
        return OwnedTags.HasAllExact(WhoCanReviveHere);

    case ENiosCoreGraveyardAccessMode::Everyone:
    default:
        return true;
    }
}

FTransform FNiosCoreGraveyardEntry::BuildRespawnTransform(APawn* RespawningPawn) const
{
    FVector SpawnLocation = Location;

    if (SpawnRadius > KINDA_SMALL_NUMBER)
    {
        const float Angle = FMath::FRandRange(0.f, 2.f * PI);
        const float Radius = FMath::Sqrt(FMath::FRand()) * SpawnRadius;
        SpawnLocation.X += FMath::Cos(Angle) * Radius;
        SpawnLocation.Y += FMath::Sin(Angle) * Radius;
    }

    // Data coordinates are usually placed on the floor. Keep capsule center above the floor marker.
    if (const UCapsuleComponent* Capsule = RespawningPawn ? RespawningPawn->FindComponentByClass<UCapsuleComponent>() : nullptr)
    {
        SpawnLocation.Z += Capsule->GetScaledCapsuleHalfHeight();
    }

    return FTransform(Rotation, SpawnLocation);
}

bool UNiosCore_GraveyardRegistryDataAsset::FindNearestGraveyard(
    UObject* WorldContextObject,
    FVector Origin,
    APawn* RespawningPawn,
    float SearchRadius,
    FTransform& OutTransform,
    FName& OutGraveyardID) const
{
    const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
    if (!World)
    {
        return false;
    }

    const FName CurrentLevelName = Nios_GetCurrentShortLevelName(World);
    const float SearchRadiusSq = SearchRadius > 0.f ? FMath::Square(SearchRadius) : TNumericLimits<float>::Max();

    const FNiosCoreGraveyardEntry* BestEntry = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();

    for (const FNiosCoreGraveyardEntry& Entry : Graveyards)
    {
        if (!Entry.IsValidForLevel(CurrentLevelName) || !Entry.IsAllowedForPawn(RespawningPawn))
        {
            continue;
        }

        const float DistSq = FVector::DistSquared(Origin, Entry.Location);
        if (DistSq > SearchRadiusSq || DistSq >= BestDistSq)
        {
            continue;
        }

        BestEntry = &Entry;
        BestDistSq = DistSq;
    }

    if (!BestEntry)
    {
        return false;
    }

    OutTransform = BestEntry->BuildRespawnTransform(RespawningPawn);
    OutGraveyardID = Nios_GetEntryName(*BestEntry);
    return true;
}

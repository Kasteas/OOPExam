#include "Character/NiosCore_GraveyardActor.h"

#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/Pawn.h"

ANiosCore_GraveyardActor::ANiosCore_GraveyardActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    RespawnArrow = CreateDefaultSubobject<UArrowComponent>(TEXT("RespawnArrow"));
    RespawnArrow->SetupAttachment(Root);

    Tags.AddUnique(FName(TEXT("Nios.Graveyard")));
}

FTransform ANiosCore_GraveyardActor::GetRespawnTransform(const APawn* RespawningPawn) const
{
    const USceneComponent* Marker = RespawnArrow ? Cast<USceneComponent>(RespawnArrow) : GetRootComponent();

    FTransform Result = Marker ? Marker->GetComponentTransform() : GetActorTransform();
    FVector Location = Result.GetLocation() + Result.GetRotation().RotateVector(LocalRespawnOffset);

    if (SpawnRadius > KINDA_SMALL_NUMBER)
    {
        const float Angle = FMath::FRandRange(0.f, 2.f * PI);
        const float Radius = FMath::Sqrt(FMath::FRand()) * SpawnRadius;
        Location.X += FMath::Cos(Angle) * Radius;
        Location.Y += FMath::Sin(Angle) * Radius;
    }

    // Keep the pawn's vertical offset stable if designers use a marker at floor level.
    if (const UCapsuleComponent* Capsule = RespawningPawn ? RespawningPawn->FindComponentByClass<UCapsuleComponent>() : nullptr)
    {
        Location.Z += Capsule->GetScaledCapsuleHalfHeight();
    }

    Result.SetLocation(Location);
    return Result;
}

FNiosCoreGraveyardEntry ANiosCore_GraveyardActor::MakeRegistryEntry() const
{
    FNiosCoreGraveyardEntry Entry;
    Entry.GraveyardID = GraveyardID.IsNone() ? FName(*GetName()) : GraveyardID;
    Entry.Location = GetRespawnTransform(nullptr).GetLocation();
    Entry.Rotation = GetRespawnTransform(nullptr).Rotator();
    Entry.SpawnRadius = SpawnRadius;
    Entry.bEnabled = bRespawnEnabled;
    return Entry;
}

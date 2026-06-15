#include "Data/StatusEffects/NiosCore_CrowdControlPresentationCatalog.h"

#include "Engine/Engine.h"
#include "Data/Config/NiosCore_DatabaseSettings.h"
#include "Data/Resolvers/ActionSlotResolverSubsystem.h"

bool UNiosCore_CrowdControlPresentationCatalog::ResolvePresentation(FName PresentationProfileID, ENiosCrowdControlType CrowdControlType, FName PresentationID, FNiosCrowdControlPresentationEntry& OutPresentation) const
{
    OutPresentation = FNiosCrowdControlPresentationEntry();

    auto FindBest = [this, CrowdControlType, PresentationID](FName ProfileID, FNiosCrowdControlPresentationEntry& OutEntry) -> bool
    {
        const FNiosCrowdControlPresentationEntry* Best = nullptr;
        for (const FNiosCrowdControlPresentationEntry& Entry : Presentations)
        {
            if (!Entry.Matches(ProfileID, CrowdControlType, PresentationID))
            {
                continue;
            }

            if (!Best || Entry.Priority > Best->Priority)
            {
                Best = &Entry;
            }
        }

        if (Best)
        {
            OutEntry = *Best;
            return true;
        }

        return false;
    };

    if (!PresentationProfileID.IsNone() && FindBest(PresentationProfileID, OutPresentation))
    {
        return true;
    }

    if (!GenericPresentationProfileID.IsNone() && GenericPresentationProfileID != PresentationProfileID && FindBest(GenericPresentationProfileID, OutPresentation))
    {
        return true;
    }

    return false;
}

UNiosCore_CrowdControlPresentationCatalog* UNiosCore_CrowdControlPresentationLibrary::GetCrowdControlPresentationCatalog(const UObject* WorldContextObject)
{
    if (!WorldContextObject || !GEngine)
    {
        return nullptr;
    }

    const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
    if (!World || !World->GetGameInstance())
    {
        return nullptr;
    }

    UActionSlotResolverSubsystem* ResolverSubsystem = World->GetGameInstance()->GetSubsystem<UActionSlotResolverSubsystem>();
    return ResolverSubsystem ? ResolverSubsystem->GetCrowdControlPresentationCatalog() : nullptr;
}

bool UNiosCore_CrowdControlPresentationLibrary::ResolveCrowdControlPresentation(const UObject* WorldContextObject, FName PresentationProfileID, ENiosCrowdControlType CrowdControlType, FName PresentationID, FNiosCrowdControlPresentationEntry& OutPresentation)
{
    OutPresentation = FNiosCrowdControlPresentationEntry();

    const UNiosCore_CrowdControlPresentationCatalog* Catalog = GetCrowdControlPresentationCatalog(WorldContextObject);
    return Catalog ? Catalog->ResolvePresentation(PresentationProfileID, CrowdControlType, PresentationID, OutPresentation) : false;
}

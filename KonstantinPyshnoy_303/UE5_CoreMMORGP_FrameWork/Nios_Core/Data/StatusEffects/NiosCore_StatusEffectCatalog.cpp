#include "Data/StatusEffects/NiosCore_StatusEffectCatalog.h"

#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Data/Config/NiosCore_DatabaseSettings.h"
#include "Data/Resolvers/ActionSlotResolverSubsystem.h"
#include "Data/StatusEffects/NiosCore_StatusEffectDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UNiosCore_StatusEffectCatalog::HasStatusEffect(FName EffectID) const
{
    return FindStatusEffect(EffectID) != nullptr;
}

const FNiosStatusEffectCatalogEntry* UNiosCore_StatusEffectCatalog::FindStatusEffect(FName EffectID) const
{
    if (EffectID.IsNone())
    {
        return nullptr;
    }

    return Effects.FindByPredicate([EffectID](const FNiosStatusEffectCatalogEntry& Entry)
    {
        return Entry.EffectID == EffectID;
    });
}

FNiosStatusEffectCatalogEntry UNiosCore_StatusEffectCatalog::GetStatusEffect(FName EffectID, bool& bFound) const
{
    if (const FNiosStatusEffectCatalogEntry* Entry = FindStatusEffect(EffectID))
    {
        bFound = true;
        return *Entry;
    }

    bFound = false;
    return FNiosStatusEffectCatalogEntry();
}

UNiosCore_StatusEffectDataAsset* UNiosCore_StatusEffectCatalog::GetStatusEffectData(FName EffectID) const
{
    const FNiosStatusEffectCatalogEntry* Entry = FindStatusEffect(EffectID);
    return Entry ? Entry->EffectData.Get() : nullptr;
}

UTexture2D* UNiosCore_StatusEffectCatalog::LoadStatusEffectIconSynchronous(FName EffectID) const
{
    const FNiosStatusEffectCatalogEntry* Entry = FindStatusEffect(EffectID);
    return Entry ? Entry->Icon.LoadSynchronous() : nullptr;
}

#if WITH_EDITOR
EDataValidationResult UNiosCore_StatusEffectCatalog::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = EDataValidationResult::Valid;
    TSet<FName> UsedIDs;

    for (int32 Index = 0; Index < Effects.Num(); ++Index)
    {
        const FNiosStatusEffectCatalogEntry& Entry = Effects[Index];

        if (Entry.EffectID.IsNone())
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("StatusEffect entry [%d] has empty EffectID."), Index)));
            Result = EDataValidationResult::Invalid;
        }
        else if (UsedIDs.Contains(Entry.EffectID))
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("Duplicate StatusEffectID '%s' in entry [%d]."), *Entry.EffectID.ToString(), Index)));
            Result = EDataValidationResult::Invalid;
        }

        UsedIDs.Add(Entry.EffectID);

        if (!Entry.EffectData)
        {
            Context.AddError(FText::FromString(FString::Printf(TEXT("StatusEffect entry '%s' has no EffectData."), *Entry.EffectID.ToString())));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.Name.IsEmpty())
        {
            Context.AddWarning(FText::FromString(FString::Printf(TEXT("StatusEffect entry '%s' has empty Name."), *Entry.EffectID.ToString())));
        }
    }

    return Result;
}
#endif

UNiosCore_StatusEffectCatalog* UNiosCore_StatusEffectCatalogLibrary::GetStatusEffectCatalog(const UObject* WorldContextObject)
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
    if (!ResolverSubsystem)
    {
        return nullptr;
    }

    return ResolverSubsystem->GetStatusEffectCatalog();
}

bool UNiosCore_StatusEffectCatalogLibrary::ResolveStatusEffectByID(const UObject* WorldContextObject, FName EffectID, FNiosStatusEffectCatalogEntry& OutEntry)
{
    OutEntry = FNiosStatusEffectCatalogEntry();

    const UNiosCore_StatusEffectCatalog* Catalog = GetStatusEffectCatalog(WorldContextObject);
    if (!Catalog)
    {
        return false;
    }

    if (const FNiosStatusEffectCatalogEntry* Entry = Catalog->FindStatusEffect(EffectID))
    {
        OutEntry = *Entry;
        return Entry->EffectData != nullptr;
    }

    return false;
}

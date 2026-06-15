#include "AbilitySystem/Skills/NiosCore_SkillCatalog.h"

#include "Engine/Texture2D.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif

bool UNiosCore_SkillCatalog::HasSkill(FName SkillID) const
{
    return FindSkill(SkillID) != nullptr;
}

const FNiosCoreSkillCatalogEntry* UNiosCore_SkillCatalog::FindSkill(FName SkillID) const
{
    if (SkillID.IsNone())
    {
        return nullptr;
    }

    return Skills.FindByPredicate([SkillID](const FNiosCoreSkillCatalogEntry& Entry)
        {
            return Entry.SkillID == SkillID;
        });
}

FNiosCoreSkillCatalogEntry UNiosCore_SkillCatalog::GetSkill(FName SkillID, bool& bFound) const
{
    if (const FNiosCoreSkillCatalogEntry* Entry = FindSkill(SkillID))
    {
        bFound = true;
        return *Entry;
    }

    bFound = false;
    return FNiosCoreSkillCatalogEntry();
}

UNiosCore_SkillDataAsset* UNiosCore_SkillCatalog::GetSkillData(FName SkillID) const
{
    const FNiosCoreSkillCatalogEntry* Entry = FindSkill(SkillID);
    return Entry ? Entry->SkillData.Get() : nullptr;
}

UTexture2D* UNiosCore_SkillCatalog::LoadSkillIconSynchronous(FName SkillID) const
{
    const FNiosCoreSkillCatalogEntry* Entry = FindSkill(SkillID);
    if (!Entry)
    {
        return nullptr;
    }

    return Entry->Icon.LoadSynchronous();
}

#if WITH_EDITOR
EDataValidationResult UNiosCore_SkillCatalog::IsDataValid(FDataValidationContext& Context) const
{
    EDataValidationResult Result = EDataValidationResult::Valid;

    TSet<FName> UsedIDs;

    for (int32 Index = 0; Index < Skills.Num(); ++Index)
    {
        const FNiosCoreSkillCatalogEntry& Entry = Skills[Index];

        if (Entry.SkillID.IsNone())
        {
            Context.AddError(FText::FromString(
                FString::Printf(TEXT("Skill entry [%d] has empty SkillID."), Index)
            ));
            Result = EDataValidationResult::Invalid;
        }

        if (UsedIDs.Contains(Entry.SkillID))
        {
            Context.AddError(FText::FromString(
                FString::Printf(TEXT("Duplicate SkillID '%s' in entry [%d]."),
                    *Entry.SkillID.ToString(),
                    Index
                )
            ));
            Result = EDataValidationResult::Invalid;
        }

        UsedIDs.Add(Entry.SkillID);

        if (!Entry.SkillData)
        {
            Context.AddError(FText::FromString(
                FString::Printf(TEXT("Skill entry '%s' has no SkillData."),
                    *Entry.SkillID.ToString()
                )
            ));
            Result = EDataValidationResult::Invalid;
        }

        if (Entry.Name.IsEmpty())
        {
            Context.AddWarning(FText::FromString(
                FString::Printf(TEXT("Skill entry '%s' has empty Name."),
                    *Entry.SkillID.ToString()
                )
            ));
        }
    }

    return Result;
}
#endif
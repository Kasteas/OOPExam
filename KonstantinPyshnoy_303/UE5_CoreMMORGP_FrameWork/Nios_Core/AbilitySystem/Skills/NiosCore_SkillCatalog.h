#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NiosCore_SkillCatalog.generated.h"

class UTexture2D;
class UNiosCore_SkillDataAsset;

USTRUCT(BlueprintType)
struct FNiosCoreSkillCatalogEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName SkillID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (MultiLine = true))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    TObjectPtr<UNiosCore_SkillDataAsset> SkillData = nullptr;

    bool IsValid() const
    {
        return !SkillID.IsNone() && SkillData != nullptr;
    }
};

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_SkillCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Skills")
    TArray<FNiosCoreSkillCatalogEntry> Skills;

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    bool HasSkill(FName SkillID) const;

    const FNiosCoreSkillCatalogEntry* FindSkill(FName SkillID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    FNiosCoreSkillCatalogEntry GetSkill(FName SkillID, bool& bFound) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    UNiosCore_SkillDataAsset* GetSkillData(FName SkillID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    UTexture2D* LoadSkillIconSynchronous(FName SkillID) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
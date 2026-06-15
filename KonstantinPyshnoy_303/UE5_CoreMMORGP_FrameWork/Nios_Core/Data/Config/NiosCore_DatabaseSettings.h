#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "NiosCore_DatabaseSettings.generated.h"

class UDataTable;
class UNiosCore_SkillCatalog;
class UNiosCore_StatusEffectCatalog;
class UNiosCore_CrowdControlPresentationCatalog;

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_DatabaseSettings : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    //  
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalogs")
    TSoftObjectPtr<UNiosCore_SkillCatalog> SkillCatalog;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalogs")
    TSoftObjectPtr<UNiosCore_StatusEffectCatalog> StatusEffectCatalog;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Catalogs")
    TSoftObjectPtr<UNiosCore_CrowdControlPresentationCatalog> CrowdControlPresentationCatalog;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items")
    TSoftObjectPtr<UDataTable> DT_Items;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Items")
    TSoftObjectPtr<UDataTable> DT_Usable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    TSoftObjectPtr<UDataTable> DT_Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
    TSoftObjectPtr<UDataTable> DT_Armor;


};
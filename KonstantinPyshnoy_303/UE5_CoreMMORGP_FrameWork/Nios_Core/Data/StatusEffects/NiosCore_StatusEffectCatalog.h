#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "NiosCore_StatusEffectCatalog.generated.h"

class UTexture2D;
class UNiosCore_StatusEffectDataAsset;

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosStatusEffectCatalogEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName EffectID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (MultiLine = true))
    FText Description;

    /** One soft UI icon. UI decides final size. Dedicated Server does not load it. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
    TSoftObjectPtr<UTexture2D> Icon;

    /** Gameplay configuration for this status effect. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Config")
    TObjectPtr<UNiosCore_StatusEffectDataAsset> EffectData = nullptr;

    bool IsValid() const
    {
        return !EffectID.IsNone() && EffectData != nullptr;
    }
};

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_StatusEffectCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Status Effects")
    TArray<FNiosStatusEffectCatalogEntry> Effects;

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    bool HasStatusEffect(FName EffectID) const;

    const FNiosStatusEffectCatalogEntry* FindStatusEffect(FName EffectID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    FNiosStatusEffectCatalogEntry GetStatusEffect(FName EffectID, bool& bFound) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    UNiosCore_StatusEffectDataAsset* GetStatusEffectData(FName EffectID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    UTexture2D* LoadStatusEffectIconSynchronous(FName EffectID) const;

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};

UCLASS()
class NIOS_CORE_API UNiosCore_StatusEffectCatalogLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Resolve configured Effect Catalog through ActionSlotResolverSubsystem / DatabaseSettings. */
    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects", meta = (WorldContext = "WorldContextObject"))
    static UNiosCore_StatusEffectCatalog* GetStatusEffectCatalog(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects", meta = (WorldContext = "WorldContextObject"))
    static bool ResolveStatusEffectByID(const UObject* WorldContextObject, FName EffectID, FNiosStatusEffectCatalogEntry& OutEntry);
};

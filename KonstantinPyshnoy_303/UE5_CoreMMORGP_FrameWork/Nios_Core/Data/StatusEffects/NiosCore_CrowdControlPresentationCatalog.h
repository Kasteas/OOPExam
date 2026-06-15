#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Data/StatusEffects/NiosCore_StatusEffectTypes.h"
#include "NiosCore_CrowdControlPresentationCatalog.generated.h"

class UAnimMontage;
class USoundBase;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCrowdControlPresentationEntry
{
    GENERATED_BODY()

    /** Visual behavior profile, not race. Examples: Humanoid, Quadruped, Giant, Spider. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName PresentationProfileID = FName(TEXT("Humanoid"));

    /** Gameplay CC type this visual represents. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    ENiosCrowdControlType CrowdControlType = ENiosCrowdControlType::None;

    /** Optional explicit key from status operation. Empty means match by CrowdControlType. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
    FName PresentationID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> EnterMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> LoopMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Animation")
    TSoftObjectPtr<UAnimMontage> ExitMontage;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> StartSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Audio")
    TSoftObjectPtr<USoundBase> LoopSound;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "VFX")
    TSoftObjectPtr<UNiagaraSystem> LoopNiagara;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Rules")
    int32 Priority = 0;

    bool Matches(FName InProfileID, ENiosCrowdControlType InType, FName InPresentationID) const
    {
        const bool bProfileMatches = PresentationProfileID == InProfileID;
        const bool bPresentationMatches = !InPresentationID.IsNone() && PresentationID == InPresentationID;
        const bool bTypeMatches = InType != ENiosCrowdControlType::None && CrowdControlType == InType;
        return bProfileMatches && (bPresentationMatches || (PresentationID.IsNone() && bTypeMatches));
    }
};

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_CrowdControlPresentationCatalog : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crowd Control")
    TArray<FNiosCrowdControlPresentationEntry> Presentations;

    /** Fallback profile used when exact profile has no visual for the CC. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Fallback")
    FName GenericPresentationProfileID = FName(TEXT("Generic"));

    UFUNCTION(BlueprintCallable, Category = "Nios|CrowdControl|Presentation")
    bool ResolvePresentation(FName PresentationProfileID, ENiosCrowdControlType CrowdControlType, FName PresentationID, FNiosCrowdControlPresentationEntry& OutPresentation) const;
};

UCLASS()
class NIOS_CORE_API UNiosCore_CrowdControlPresentationLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|CrowdControl|Presentation", meta = (WorldContext = "WorldContextObject"))
    static UNiosCore_CrowdControlPresentationCatalog* GetCrowdControlPresentationCatalog(const UObject* WorldContextObject);

    UFUNCTION(BlueprintCallable, Category = "Nios|CrowdControl|Presentation", meta = (WorldContext = "WorldContextObject"))
    static bool ResolveCrowdControlPresentation(const UObject* WorldContextObject, FName PresentationProfileID, ENiosCrowdControlType CrowdControlType, FName PresentationID, FNiosCrowdControlPresentationEntry& OutPresentation);
};

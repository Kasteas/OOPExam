#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Data/UI/NiosCore_UIDataTypes.h"
#include "NiosCore_UIDataSubsystem.generated.h"

class UNiosCore_UIDataAsset;

UCLASS(BlueprintType)
class NIOS_CORE_API UNiosCore_UIDataSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|UI")
    void SetUIDataAsset(UNiosCore_UIDataAsset* InUIDataAsset);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    UNiosCore_UIDataAsset* GetUIDataAsset() const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FText GetStatDisplayName(ENiosStatType StatType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FText GetStatDescription(ENiosStatType StatType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FLinearColor GetStatColor(ENiosStatType StatType) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FText GetItemTypeDisplayName(FName ItemTypeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FText GetWeaponTypeDisplayName(FName WeaponTypeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FText GetArmorTypeDisplayName(FName ArmorTypeID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FText GetArmorSlotDisplayName(FName ArmorSlotID) const;

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|UI")
    FText GetTooltipLabel(FName LabelID) const;

protected:
    FText FindNamedDisplayName(const TArray<FNiosNamedDisplayData>& Entries, FName ID) const;
    FText FindNamedDescription(const TArray<FNiosNamedDisplayData>& Entries, FName ID) const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_UIDataAsset> UIDataAsset = nullptr;
};

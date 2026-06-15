#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Core/Nios_CoreStructers.h"
#include "Data/Items/NiosCore_ItemDataTypes.h"
#include "UI/Tooltip/NiosCore_TooltipTypes.h"
#include "Data/Stats/NiosCore_StatTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Components/Abilities/NiosCore_AbilityLoadoutComponent.h"

#include "ActionSlotResolverSubsystem.generated.h"

class UDataTable;
class UNiosCore_SkillCatalog;
class UNiosCore_StatusEffectCatalog;
class UNiosCore_CrowdControlPresentationCatalog;
class UTexture2D;
class UNiosCore_DatabaseSettings;
class UNiosCore_UIDataSubsystem;


/**
 * BP payload for ActionSlot -> GroundTargetingComponent.
 * UI should ask the resolver for this instead of manually opening SkillCatalog/DataAssets.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreGroundTargetingPayload
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Ground Targeting")
    bool bShouldStartGroundTargeting = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Ground Targeting")
    FName SkillID = NAME_None;

    /** Max targeting distance/range used by GroundTargetingComponent preview and confirm. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Ground Targeting", meta = (ClampMin = "0"))
    float Distance = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Ground Targeting")
    ENiosAbilityTargetType TargetType = ENiosAbilityTargetType::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Ground Targeting")
    UNiosCore_SkillDataAsset* SkillData = nullptr;
};

UCLASS(BlueprintType)
class NIOS_CORE_API UActionSlotResolverSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    bool ResolveVisualData(
        const FNiosActionSlotData& SlotData,
        FNiosActionSlotVisualData& OutVisualData
    );
    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    bool UIsItemUsable(const FNiosActionSlotData& SlotData);

    UFUNCTION(BlueprintCallable, Category = "Nios|Inventory")
    bool ResolveInventoryItemData(
        FName ItemID,
        FNiosResolvedInventoryItemData& OutItemData
    );



    /** Returns SkillDataAsset from the configured SkillCatalog by semantic SkillID. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Skill")
    bool ResolveSkillDataAsset(FName SkillID, UNiosCore_SkillDataAsset*& OutSkillData);

    /** Returns SkillDataAsset when Request.Kind == Skill and Request.ID is a valid SkillID. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Skill")
    bool ResolveSkillDataAssetFromActivationRequest(const FNiosCoreActivationRequest& Request, UNiosCore_SkillDataAsset*& OutSkillData);

    /** Returns TargetType for a SkillID without forcing UI to manually open SkillCatalog. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Skill")
    bool GetSkillTargetType(FName SkillID, ENiosAbilityTargetType& OutTargetType);

    /** Returns TargetType when Request.Kind == Skill. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Skill")
    bool GetSkillTargetTypeFromActivationRequest(const FNiosCoreActivationRequest& Request, ENiosAbilityTargetType& OutTargetType);

    /** True when Request.Kind == Skill and its SkillDataAsset.TargetType == GroundPoint. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Skill")
    bool IsGroundTargetedSkill(const FNiosCoreActivationRequest& Request);

    /** True when SkillDataAsset.TargetType == GroundPoint. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Skill")
    bool IsGroundTargetedSkillByID(FName SkillID);

    /** Convenience helper for BP ActionSlot. Returns false if Request is not a skill or SkillID is unknown. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Skill")
    bool ShouldStartGroundTargetingForRequest(const FNiosCoreActivationRequest& Request, FName& OutSkillID);

    /** Same as ShouldStartGroundTargetingForRequest, but also returns targeting distance for StartGroundTargeting. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Ground Targeting")
    bool ShouldStartGroundTargetingForRequestWithDistance(const FNiosCoreActivationRequest& Request, FName& OutSkillID, float& OutDistance);

    /** Returns the configured/default ground targeting distance for a skill. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Ground Targeting")
    bool GetSkillGroundTargetingDistance(FName SkillID, float& OutDistance);

    /** One-call BP helper for ActionSlot. Use Payload.SkillID and Payload.Distance to start GroundTargetingComponent. */
    UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Nios|ActionSlot|Ground Targeting")
    bool ResolveGroundTargetingPayloadForRequest(const FNiosCoreActivationRequest& Request, FNiosCoreGroundTargetingPayload& OutPayload);

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    bool ResolveTooltipData(
        const FNiosActionSlotData& SlotData,
        FNiosTooltipData& OutTooltipData
    );

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    bool ResolveItemTooltipData(
        FName ItemID,
        int32 Count,
        FNiosTooltipData& OutTooltipData
    );

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    bool ResolveSkillTooltipData(
        FName SkillID,
        FNiosTooltipData& OutTooltipData
    );

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    bool ResolveStatTooltipData(
        const FNiosCalculatedStat& Stat,
        FNiosTooltipData& OutTooltipData
    );

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void ClearCache();

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void SetDatabaseSettings(UNiosCore_DatabaseSettings* InDatabaseSettings);

    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_DatabaseSettings> DatabaseSettings;

    UFUNCTION(BlueprintCallable, Category = "Nios|StatusEffects")
    UNiosCore_StatusEffectCatalog* GetStatusEffectCatalog();

    UFUNCTION(BlueprintCallable, Category = "Nios|CrowdControl|Presentation")
    UNiosCore_CrowdControlPresentationCatalog* GetCrowdControlPresentationCatalog();

protected:
    bool ResolveSkillVisualData(
        const FNiosActionSlotData& SlotData,
        FNiosActionSlotVisualData& OutVisualData
    );

    bool ResolveItemVisualData(
        const FNiosActionSlotData& SlotData,
        FNiosActionSlotVisualData& OutVisualData
    );

    UNiosCore_SkillCatalog* GetSkillCatalog();

    UDataTable* GetItemsTable() const;

    UDataTable* GetUsableTable() const;

    UDataTable* GetWeaponTable() const;

    UDataTable* GetArmorTable() const;

    UTexture2D* LoadIconCached(TSoftObjectPtr<UTexture2D> Icon);

    UNiosCore_UIDataSubsystem* GetUIDataSubsystem() const;

    FText GetTooltipLabel(FName LabelID) const;
    FText GetTooltipLabelWithFallback(FName LabelID, const FText& FallbackText) const;
    FText GetStatDisplayName(ENiosStatType StatType) const;
    FText GetGameplayAttributeDisplayName(const FGameplayAttribute& Attribute) const;
    FText GetItemTypeDisplayName(ENiosItemType ItemType) const;
    FText GetWeaponTypeDisplayName(ENiosWeaponType WeaponType) const;
    FText GetArmorTypeDisplayName(ENiosArmorType ArmorType) const;
    FText GetArmorSlotDisplayName(ENiosArmorSlot ArmorSlot) const;
private:
    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_SkillCatalog> CachedSkillCatalog;

    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_StatusEffectCatalog> CachedStatusEffectCatalog;

    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_CrowdControlPresentationCatalog> CachedCrowdControlPresentationCatalog;

    UPROPERTY(Transient)
    TMap<FSoftObjectPath, TObjectPtr<UTexture2D>> LoadedIconCache;
};
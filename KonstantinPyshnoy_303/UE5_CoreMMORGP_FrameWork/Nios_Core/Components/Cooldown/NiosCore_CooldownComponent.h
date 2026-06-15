#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/Abilities/NiosCore_AbilityLoadoutComponent.h"
#include "NiosCore_CooldownComponent.generated.h"

UENUM(BlueprintType)
enum class ENiosCoreCooldownDomain : uint8
{
    Skill UMETA(DisplayName = "Skill"),
    Item UMETA(DisplayName = "Item"),
    GlobalSkill UMETA(DisplayName = "Global Skill")
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreCooldownEntry
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    FName ID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    float StartTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    float EndTime = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    float Duration = 0.f;

    bool IsActive(float Now) const
    {
        return ID != NAME_None && EndTime > Now;
    }
};


/**
 * Blueprint/UI friendly cooldown snapshot for ActionSlot widgets.
 *
 * ActionSlot should not tick or know gameplay rules. It asks CooldownComponent for this state,
 * then locally renders Remaining / Duration every frame. Server still validates cooldowns on use.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreCooldownState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    bool bOnCooldown = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    float Remaining = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    float Duration = 0.f;

    /** 0..1 value suitable for radial cooldown fill. 1 means full cooldown remaining. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    float Normalized = 0.f;

    /** Which cooldown won when multiple cooldowns apply, e.g. GlobalSkill vs Skill. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    ENiosCoreCooldownDomain DominantDomain = ENiosCoreCooldownDomain::Skill;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    FName DominantID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    bool bGlobalCooldownActive = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    bool bLocalCooldownActive = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNiosCooldownStartedSignature, ENiosCoreCooldownDomain, Domain, FName, ID, float, Duration);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNiosCooldownClearedSignature, ENiosCoreCooldownDomain, Domain, FName, ID);

/**
 * Timestamp-based cooldown storage for player actions.
 *
 * Intended owner: PlayerState.
 *
 * Important MMO rule:
 * - This component does NOT tick cooldown remaining time on the server.
 * - Server stores authoritative absolute end timestamps and checks them on use requests.
 * - Owning client/UI can locally compute remaining time from EndTime for ActionSlot visuals.
 *
 * Current integration:
 * - Global skill cooldown.
 * - Local skill cooldown from SkillDataAsset.
 *
 * Prepared but not yet wired to usable item logic:
 * - Item cooldowns, including DefaultItemCooldown.
 */
UCLASS(ClassGroup = (NiosCore), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_CooldownComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_CooldownComponent();

    /**
     * Owning-client notification used by ActionSlot UI.
     * The server remains authoritative and stores end timestamps; the client receives one event and
     * locally stores a matching timestamp so widgets can draw cooldown without replicated ticking.
     */
    UFUNCTION(Client, Reliable)
    void Client_ReceiveCooldownStarted(ENiosCoreCooldownDomain Domain, FName ID, float Duration);


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Cooldown|Skill", meta = (ClampMin = "0"))
    float GlobalSkillCooldown = 0.75f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Cooldown|Skill")
    bool bUseGlobalSkillCooldown = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Cooldown|Item", meta = (ClampMin = "0"))
    float DefaultItemCooldown = 10.f;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Cooldown")
    FNiosCooldownStartedSignature OnCooldownStarted;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Cooldown")
    FNiosCooldownClearedSignature OnCooldownCleared;

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown")
    bool IsGlobalSkillCooldownReady() const;

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown")
    float GetGlobalSkillCooldownRemaining() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cooldown")
    void StartGlobalSkillCooldown(float DurationOverride = -1.f);

    UFUNCTION(BlueprintCallable, Category = "Nios|Cooldown")
    void ClearGlobalSkillCooldown();

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Skill")
    bool IsSkillCooldownReady(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Skill")
    float GetSkillCooldownRemaining(FName SkillID) const;

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Skill")
    float GetSkillCooldownDuration(FName SkillID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cooldown|Skill")
    void StartSkillCooldown(FName SkillID, float Duration);

    UFUNCTION(BlueprintCallable, Category = "Nios|Cooldown|Skill")
    void ClearSkillCooldown(FName SkillID);

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Item")
    bool IsItemCooldownReady(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Item")
    float GetItemCooldownRemaining(FName ItemID) const;

    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Item")
    float GetItemCooldownDuration(FName ItemID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cooldown|Item")
    void StartItemCooldown(FName ItemID, float Duration = -1.f);

    UFUNCTION(BlueprintCallable, Category = "Nios|Cooldown|Item")
    void ClearItemCooldown(FName ItemID);

    /**
     * ActionSlot helper for skill slots.
     * Combines GlobalSkillCooldown and local SkillCooldown[SkillID].
     * The returned state uses whichever remaining time is currently larger.
     * This is what makes all skill slots show the quick global visual cooldown.
     */
    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Action Slot")
    FNiosCoreCooldownState GetSkillCooldownState(FName SkillID) const;

    /** ActionSlot helper for item slots. Usable item execution is not wired here yet. */
    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Action Slot")
    FNiosCoreCooldownState GetItemCooldownState(FName ItemID) const;

    /**
     * Universal ActionSlot helper.
     * Skill -> GlobalSkill + local skill cooldown.
     * Item  -> local item cooldown for now.
     * Other kinds currently return no cooldown unless future systems add one.
     */
    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown|Action Slot")
    FNiosCoreCooldownState GetCooldownStateForActivationRequest(const FNiosCoreActivationRequest& Request) const;

    /** Generic helper for ActionSlot UI. */
    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown")
    float GetCooldownRemaining(ENiosCoreCooldownDomain Domain, FName ID) const;

    /** Generic helper for ActionSlot UI. */
    UFUNCTION(BlueprintPure, Category = "Nios|Cooldown")
    float GetCooldownDuration(ENiosCoreCooldownDomain Domain, FName ID) const;

protected:
    float GetNow() const;
    static float GetRemainingForEndTime(float Now, float EndTime);

    void ApplyCooldownStartedLocal(ENiosCoreCooldownDomain Domain, FName ID, float Duration, bool bBroadcast);
    void NotifyOwningClientCooldownStarted(ENiosCoreCooldownDomain Domain, FName ID, float Duration);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    FNiosCoreCooldownEntry GlobalSkillCooldownEntry;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    TMap<FName, FNiosCoreCooldownEntry> SkillCooldowns;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Cooldown")
    TMap<FName, FNiosCoreCooldownEntry> ItemCooldowns;
};

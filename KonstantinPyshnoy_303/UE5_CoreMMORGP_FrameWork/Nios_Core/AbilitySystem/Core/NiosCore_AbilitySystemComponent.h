#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Components/Feedback/NiosCore_ActionFeedbackTypes.h"
#include "NiosCore_AbilitySystemComponent.generated.h"

class AActor;
class UGameplayAbility;
class UAnimMontage;
class USoundBase;
struct FStreamableHandle;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNiosCastStateChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNiosGrantedSkillsChanged);

UENUM(BlueprintType)
enum class ENiosCoreSkillActivationFeedbackState : uint8
{
    None UMETA(DisplayName = "None"),
    Requested UMETA(DisplayName = "Requested"),
    Accepted UMETA(DisplayName = "Accepted"),
    Executing UMETA(DisplayName = "Executing"),
    Executed UMETA(DisplayName = "Executed"),
    Rejected UMETA(DisplayName = "Rejected"),
    Interrupted UMETA(DisplayName = "Interrupted"),
    Failed UMETA(DisplayName = "Failed")
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreSkillActivationFeedback
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    FName SkillID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    TObjectPtr<UNiosCore_SkillDataAsset> Skill = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    ENiosCoreSkillActivationFeedbackState State = ENiosCoreSkillActivationFeedbackState::None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    ENiosCoreActionFailReason FailureReason = ENiosCoreActionFailReason::None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    float RequiredValue = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    float CurrentValue = 0.f;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|Skills|Feedback")
    FName DebugReason = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosSkillActivationFeedbackSignature, const FNiosCoreSkillActivationFeedback&, Feedback);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNiosSelectedTargetChangedSignature, AActor*, Target, bool, bServerConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosSelectedTargetRejectedSignature, AActor*, RejectedTarget);

USTRUCT(BlueprintType)
struct FNiosCoreGrantedSkill
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName SkillID = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    FGameplayAbilitySpecHandle SpecHandle;
};

USTRUCT(BlueprintType)
struct FNiosLoadedSkillAssets
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UAnimMontage> CastLoopMontage;

    UPROPERTY()
    TObjectPtr<UAnimMontage> ExecuteMontage;

    UPROPERTY()
    TObjectPtr<USoundBase> CastLoopSound;

    UPROPERTY()
    TObjectPtr<USoundBase> ExecuteSound;
};

UCLASS(Blueprintable, BlueprintType)
class NIOS_CORE_API UNiosCore_AbilitySystemComponent : public UAbilitySystemComponent
{
    GENERATED_BODY()

public:
    UNiosCore_AbilitySystemComponent();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable)
    void InitAvatar(AActor* Pawn);

    /**
     * Initializes ASC owner/avatar and guarantees a Nios AttributeSet is registered.
     * Player-owned ASC should pass InOwnerActor = PlayerState, InAvatarActor = Pawn, AttributeSetOverride = PlayerState AttributeSet.
     */
    UFUNCTION(BlueprintCallable)
    void InitOwnerAvatar(AActor* InOwnerActor, AActor* InAvatarActor, UNiosCore_AttributeSet* AttributeSetOverride = nullptr);

    UFUNCTION(BlueprintCallable, Category = "Nios|Attributes")
    bool GetAttributeValue(FGameplayAttribute Attribute, float& OutValue) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Attributes")
    bool SetAttributeValue(FGameplayAttribute Attribute, float NewValue);

    UFUNCTION(BlueprintCallable, Category = "Nios|Attributes")
    bool AddAttributeValue(FGameplayAttribute Attribute, float DeltaValue);

    UFUNCTION(BlueprintCallable)
    UNiosCore_AttributeSet* GetStatsAttributeSet() const;

    UFUNCTION(BlueprintCallable)
    FGameplayAbilitySpecHandle GiveAbilityWithSkill(
        FName SkillID,
        TSubclassOf<UGameplayAbility> AbilityClass,
        UNiosCore_SkillDataAsset* Skill,
        int32 Level = 1
    );

    UPROPERTY(BlueprintAssignable, Category = "Nios|Skills")
    FNiosGrantedSkillsChanged OnGrantedSkillsChanged;

    /** Local/client UX hook: Requested fires immediately on the owning client, Accepted/Rejected comes from server validation. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Skills|Feedback")
    FNiosSkillActivationFeedbackSignature OnSkillActivationFeedback;

    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    bool TryActivateSkillByID(FName SkillID);

    /** Activates a granted SkillID using an authoritative ground/world target location. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    bool TryActivateSkillByIDAtLocation(FName SkillID, FVector TargetLocation);

    /** Stores one-shot ground location for the next GroundPoint skill activation. Server-authoritative, not replicated. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Ground Target")
    void SetPendingGroundTargetLocation(const FVector& Location);

    /** Consumes one-shot ground location for skill pipeline. Returns false if no location was supplied. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Ground Target")
    bool ConsumePendingGroundTargetLocation(FVector& OutLocation);

    /** Peeks current one-shot ground location without clearing it. */
    UFUNCTION(BlueprintPure, Category = "Nios|Ground Target")
    bool PeekPendingGroundTargetLocation(FVector& OutLocation) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    bool HasSkillByID(FName SkillID) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    FName GetSkillIDBySpecHandle(FGameplayAbilitySpecHandle SpecHandle) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    const TArray<FNiosCoreGrantedSkill>& GetGrantedSkills() const;

    /*
     * Preloads visual/audio assets referenced by SkillDataAsset.
     * Runtime skill activation should use cached assets and avoid synchronous loads.
     */
    UFUNCTION(BlueprintCallable, Category = "Nios|Skills")
    void PreloadSkillAssets(UNiosCore_SkillDataAsset* Skill);

    /* Returns cached loaded assets for a skill, or nullptr if not loaded. */
    FNiosLoadedSkillAssets* GetLoadedSkillAssets(UNiosCore_SkillDataAsset* Skill);

    UFUNCTION(BlueprintCallable, Category = "Nios|Target")
    void SetSelectedTarget(AActor* NewTarget);

    UFUNCTION(BlueprintCallable, Category = "Nios|Target")
    AActor* GetSelectedTarget() const;

    /** Returns immediate local preview on owning clients, otherwise the replicated server-confirmed target. UI should prefer this. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Target")
    AActor* GetDisplayedSelectedTarget() const;

    /** Local target preview used to hide 100-150ms ping while the server confirms selection. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Target")
    AActor* GetLocalSelectedTargetPreview() const;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Target")
    FNiosSelectedTargetChangedSignature OnSelectedTargetChanged;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Target")
    FNiosSelectedTargetRejectedSignature OnSelectedTargetRejected;

    UFUNCTION(BlueprintCallable, Category = "Nios|Target")
    void ClearSelectedTarget();

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    bool IsCasting() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    UNiosCore_SkillDataAsset* GetCurrentCastSkill() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    AActor* GetCastLockedTarget() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Target")
    AActor* GetCurrentLookAtTarget() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    float GetCastStartTime() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    float GetCastEndTime() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    float GetCastDuration() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    float GetCastRemainingTime() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Cast")
    float GetCastProgress01() const;

    void BeginCastState(UNiosCore_SkillDataAsset* Skill, float CastTime, AActor* LockedTarget = nullptr);
    void EndCastState();

    /** Called by the server-side skill pipeline after validation. Routes Accepted to the owning client. */
    void NotifySkillActivationAccepted(FName SkillID, UNiosCore_SkillDataAsset* Skill, AActor* TargetActor = nullptr, FName DebugReason = NAME_None);

    /** Called when the skill is past commit/cost/cooldown and begins authoritative execution. */
    void NotifySkillActivationExecuting(FName SkillID, UNiosCore_SkillDataAsset* Skill, AActor* TargetActor = nullptr, FName DebugReason = NAME_None);

    /** Called when all gameplay effects/deliveries for the activation have finished. */
    void NotifySkillActivationExecuted(FName SkillID, UNiosCore_SkillDataAsset* Skill, AActor* TargetActor = nullptr, FName DebugReason = NAME_None);

    /** Called when an already accepted cast/activation is interrupted before execute. */
    void NotifySkillActivationInterrupted(FName SkillID, UNiosCore_SkillDataAsset* Skill, ENiosCoreActionFailReason FailureReason, AActor* TargetActor = nullptr, FName DebugReason = NAME_None, float RequiredValue = 0.f, float CurrentValue = 0.f);

    /** Called when execution failed after validation/acceptance, for example CommitAbility failed. */
    void NotifySkillActivationFailed(FName SkillID, UNiosCore_SkillDataAsset* Skill, ENiosCoreActionFailReason FailureReason, AActor* TargetActor = nullptr, FName DebugReason = NAME_None, float RequiredValue = 0.f, float CurrentValue = 0.f);

    /** Called by the server-side skill pipeline or ASC request layer when activation fails before acceptance. Routes Rejected to the owning client. */
    void NotifySkillActivationRejected(FName SkillID, UNiosCore_SkillDataAsset* Skill, ENiosCoreActionFailReason FailureReason, FName DebugReason = NAME_None);

    UPROPERTY(BlueprintAssignable, Category = "Nios|Cast")
    FNiosCastStateChanged OnCastStateChanged;

protected:

    UFUNCTION(Server, Reliable)
    void Server_TryActivateSkillByID(FName SkillID);

    UFUNCTION(Server, Reliable)
    void Server_TryActivateSkillByIDAtLocation(FName SkillID, FVector_NetQuantize TargetLocation);

    UFUNCTION(Server, Reliable)
    void Server_SetSelectedTarget(AActor* NewTarget);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveSelectedTargetRejected(AActor* RejectedTarget);

    UFUNCTION(Client, Reliable)
    void Client_ReceiveSkillActivationFeedback(FNiosCoreSkillActivationFeedback Feedback);

    UFUNCTION()
    void OnRep_SelectedTarget();

    UFUNCTION()
    void OnRep_CastState();

    bool IsValidTargetActor(AActor* TargetActor) const;
    bool ShouldDispatchLocalTargetFeedback() const;
    void NotifyLocalDisplayedTargetChanged(AActor* NewDisplayedTarget, bool bServerConfirmed, FName Reason);

    UFUNCTION()
    void OnRep_GrantedSkills();

    void RebuildSkillMaps();
    void PreloadGrantedSkillAssetsFromSpecs();
    bool FindGrantedSkillByID(FName SkillID, FGameplayAbilitySpecHandle& OutHandle, UNiosCore_SkillDataAsset*& OutSkill);
    void BroadcastSkillActivationFeedback(FName SkillID, UNiosCore_SkillDataAsset* Skill, ENiosCoreSkillActivationFeedbackState State, ENiosCoreActionFailReason FailureReason = ENiosCoreActionFailReason::None, FName DebugReason = NAME_None, AActor* TargetActor = nullptr, float RequiredValue = 0.f, float CurrentValue = 0.f);
    void RouteSkillActivationFeedbackToOwningClient(FNiosCoreSkillActivationFeedback Feedback);

protected:
    UPROPERTY(ReplicatedUsing = OnRep_GrantedSkills)
    TArray<FNiosCoreGrantedSkill> GrantedSkills;

    TMap<FName, FGameplayAbilitySpecHandle> SkillIDToSpecHandle;
    TMap<FGameplayAbilitySpecHandle, FName> SpecHandleToSkillID;

private:
    UPROPERTY()
    TObjectPtr<UNiosCore_AttributeSet> StatsAttributeSet;

    UPROPERTY()
    TMap<TObjectPtr<UNiosCore_SkillDataAsset>, FNiosLoadedSkillAssets> LoadedSkillAssets;

    /* FStreamableHandle is not UObject; keep it outside UPROPERTY. */
    TMap<TObjectPtr<UNiosCore_SkillDataAsset>, TSharedPtr<FStreamableHandle>> LoadingSkillAssetHandles;

    UPROPERTY(ReplicatedUsing = OnRep_SelectedTarget)
    TObjectPtr<AActor> SelectedTarget;

    /** Client-only optimistic target used by UI until OnRep_SelectedTarget confirms or server rejects it. */
    UPROPERTY(Transient)
    TObjectPtr<AActor> LocalSelectedTargetPreview;

    /** Client/listen-server only: last actor that received local target feedback from GetDisplayedSelectedTarget(). */
    UPROPERTY(Transient)
    TObjectPtr<AActor> LastLocallyNotifiedDisplayedTarget;

    UPROPERTY(ReplicatedUsing = OnRep_CastState)
    bool bIsCasting = false;

    UPROPERTY(ReplicatedUsing = OnRep_CastState)
    TObjectPtr<UNiosCore_SkillDataAsset> CurrentCastSkill;

    UPROPERTY(ReplicatedUsing = OnRep_CastState)
    TObjectPtr<AActor> CastLockedTarget;

    UPROPERTY(ReplicatedUsing = OnRep_CastState)
    float CastStartTime = 0.f;

    UPROPERTY(ReplicatedUsing = OnRep_CastState)
    float CastEndTime = 0.f;

    /** One-shot server-side location supplied by ground targeting before activating a GroundPoint skill. */
    UPROPERTY()
    FVector PendingGroundTargetLocation = FVector::ZeroVector;

    /** Whether PendingGroundTargetLocation contains valid one-shot data for the next GroundPoint skill. */
    UPROPERTY()
    bool bHasPendingGroundTargetLocation = false;
};

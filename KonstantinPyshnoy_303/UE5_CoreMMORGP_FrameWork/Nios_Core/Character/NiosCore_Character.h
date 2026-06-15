#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Interfaces/NiosMissionManagerInterface.h"
#include "NiagaraSystem.h"
#include "NiosCore_Character.generated.h"

class UNiosCore_AbilitySystemComponent;
class UNiosCore_AttributeSet;
class UNiosCore_StatsComponent;
class UAbilitySystemComponent;
class UNiosCore_SkillDataAsset;
class UNiosCore_TargetFeedbackComponent;
class UMOMissionsManager;

UCLASS()
class NIOS_CORE_API ANiosCore_Character
    : public ACharacter
    , public IAbilitySystemInterface
    , public INiosGASActorInterface
    , public INiosMissionManagerInterface
{
    GENERATED_BODY()

public:
    ANiosCore_Character();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    virtual void PossessedBy(AController* NewController) override;
    virtual void OnRep_PlayerState() override;
    virtual void UnPossessed() override;

    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintCallable, Category = "Nios|Ability")
    UNiosCore_AbilitySystemComponent* GetNiosAbilitySystemComponent() const;

    /** Local-only feedback component used by client-side target selection visuals. */
    UFUNCTION(BlueprintPure, Category = "Nios|Targeting|Feedback")
    UNiosCore_TargetFeedbackComponent* GetTargetFeedbackComponent() const { return TargetFeedbackComponent; }

    // INiosGASActorInterface
    virtual UNiosCore_AbilitySystemComponent* GetNiosASC_Implementation() const override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponentGeneric_Implementation() const override;
    virtual UNiosCore_AttributeSet* GetNiosAttributeSet_Implementation() const override;
    virtual UNiosCore_StatsComponent* GetNiosStatsComponent_Implementation() const override;
    virtual UNiosCore_StatusEffectComponent* GetStatusEffectComponentGeneric_Implementation() const override;
    virtual bool IsAlive_Implementation() const override;
    virtual bool CanBeTargetedBy_Implementation(AActor* SourceActor) const override;
    virtual FName GetTeamID_Implementation() const override;
    virtual void HandleDeath_Implementation(AActor* Killer) override;

    /** Clears the local dead state after an authoritative resurrection/heal raised Health above zero. */
    UFUNCTION(BlueprintCallable, Category = "Nios|State")
    void HandleRevive();

    UFUNCTION(BlueprintPure, Category = "Nios|State")
    bool IsDead() const { return bDead; }

    // INiosMissionManagerInterface. Character is a convenience proxy to PlayerState.
    virtual UMOMissionsManager* GetNiosMissionManager_Implementation() const override;

    UFUNCTION(BlueprintCallable, Category = "Nios|Missions")
    UMOMissionsManager* GetNiosMissionsManager() const;

    /** Designer-defined combat radius used by skill edge-to-edge range checks. <= 0 falls back to bounds/collision. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Targeting|Range")
    void SetCombatRangeRadiusOverride(float NewRadius);

    UFUNCTION(BlueprintPure, Category = "Nios|Targeting|Range")
    float GetCombatRangeRadiusOverride() const { return CombatRangeRadiusOverride; }

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Ability")
    void BP_OnAbilitySystemInitialized();

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|State")
    void BP_OnDeath(AActor* Killer);

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|State")
    void BP_OnRevive();

    UFUNCTION(BlueprintCallable, Category = "Nios|Ability")
    void InitAbilitySystemFromPlayerState();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Targeting|Range", meta = (ClampMin = "0", UIMin = "0"))
    float CombatRangeRadiusOverride = 0.f;

    /** Present on all base targetable player characters so BP can drive decals/outline/nameplate feedback. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Targeting|Feedback")
    TObjectPtr<UNiosCore_TargetFeedbackComponent> TargetFeedbackComponent;

public:
    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySkillExecuteVisuals(UNiosCore_SkillDataAsset* Skill);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySkillCastLoopVisuals(UNiosCore_SkillDataAsset* Skill, bool bStart);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlayDeliveryNiagara(
        const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem,
        FVector Location,
        FRotator Rotation,
        FName DebugName);

protected:
    void PlaySkillExecuteVisualsLocal(UNiosCore_SkillDataAsset* Skill);
    void PlaySkillCastLoopVisualsLocal(UNiosCore_SkillDataAsset* Skill, bool bStart);
    void PlayDeliveryNiagaraLocal(
        const TSoftObjectPtr<UNiagaraSystem>& NiagaraSystem,
        FVector Location,
        FRotator Rotation,
        FName DebugName);

private:
    UFUNCTION()
    void OnRep_Dead();

    void ApplyDeathMovementState(bool bInDeadState);

    bool bAbilitySystemInitialized = false;

    /** Tracks whether this pawn already pushed Controller::SetIgnoreMoveInput(true) for death lock. */
    UPROPERTY(Transient)
    bool bDeathMoveInputLocked = false;

    UPROPERTY(ReplicatedUsing = OnRep_Dead)
    bool bDead = false;
};

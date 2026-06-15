#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "NiosCore_GASAICharacter.generated.h"

class UNiosCore_AbilitySystemComponent;
class UNiosCore_AttributeSet;
class UNiosCore_StatsComponent;
class UNiosCore_StatusEffectComponent;
class UNiosCore_TargetFeedbackComponent;
class UNiosCore_LootSourceComponent;
class UAbilitySystemComponent;
class UAudioComponent;
class UNiosCore_SkillDataAsset;

UCLASS(Blueprintable)
class NIOS_CORE_API ANiosCore_GASAICharacter
    : public ACharacter
    , public IAbilitySystemInterface
    , public INiosGASActorInterface
{
    GENERATED_BODY()

public:
    ANiosCore_GASAICharacter();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintCallable, Category = "Nios|Ability")
    UNiosCore_AbilitySystemComponent* GetNiosAbilitySystemComponent() const;

    /** Local-only feedback component used by client-side target selection visuals. */
    UFUNCTION(BlueprintPure, Category = "Nios|Targeting|Feedback")
    UNiosCore_TargetFeedbackComponent* GetTargetFeedbackComponent() const { return TargetFeedbackComponent; }

    /** Server-authoritative loot source present by default on AI pawns; EnemyDefinitionComponent configures it from the enemy row. */
    UFUNCTION(BlueprintPure, Category = "Nios|Loot")
    UNiosCore_LootSourceComponent* GetLootSourceComponent() const { return LootSourceComponent; }

    UFUNCTION(BlueprintCallable, Category = "Nios|Targeting")
    void SetRelationToPlayer(ENiosCoreTargetRelation NewRelation);

    UFUNCTION(BlueprintPure, Category = "Nios|Targeting")
    ENiosCoreTargetRelation GetRelationToPlayer() const { return RelationToPlayer; }

    UFUNCTION(BlueprintCallable, Category = "Nios|Combat")
    void SetImmortal(bool bNewImmortal);

    UFUNCTION(BlueprintCallable, Category = "Nios|Combat")
    void SetCanBeTargeted(bool bNewCanBeTargeted);

    UFUNCTION(BlueprintPure, Category = "Nios|Combat")
    bool IsImmortal() const { return bImmortal; }

    UFUNCTION(BlueprintCallable, Category = "Nios|Combat")
    void SetCanDealDamage(bool bNewCanDealDamage);

    UFUNCTION(BlueprintPure, Category = "Nios|Combat")
    bool CanDealDamage() const { return bCanDealDamage; }

    /** Designer-defined combat radius used by skill edge-to-edge range checks. <= 0 falls back to bounds/collision. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Targeting|Range")
    void SetCombatRangeRadiusOverride(float NewRadius);

    UFUNCTION(BlueprintPure, Category = "Nios|Targeting|Range")
    float GetCombatRangeRadiusOverride() const { return CombatRangeRadiusOverride; }

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

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySkillExecuteVisuals(UNiosCore_SkillDataAsset* Skill);

    UFUNCTION(NetMulticast, Unreliable)
    void Multicast_PlaySkillCastLoopVisuals(UNiosCore_SkillDataAsset* Skill, bool bStart);

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Ability")
    TObjectPtr<UNiosCore_AbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Ability")
    TObjectPtr<UNiosCore_AttributeSet> AttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    TObjectPtr<UNiosCore_StatsComponent> StatsComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffects")
    TObjectPtr<UNiosCore_StatusEffectComponent> StatusEffectComponent;

    /** Present on all base targetable AI/NPC actors so BP can drive decals/outline/nameplate feedback. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Targeting|Feedback")
    TObjectPtr<UNiosCore_TargetFeedbackComponent> TargetFeedbackComponent;

    /** Passive by default; configured from FNiosEnemyRow Loot settings when this pawn represents a lootable mob. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Loot")
    TObjectPtr<UNiosCore_LootSourceComponent> LootSourceComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Targeting")
    ENiosCoreTargetRelation RelationToPlayer = ENiosCoreTargetRelation::Enemy;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Targeting")
    bool bCanBeTargeted = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Combat")
    bool bImmortal = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Combat")
    bool bCanDealDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Targeting|Range", meta = (ClampMin = "0", UIMin = "0"))
    float CombatRangeRadiusOverride = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Testing")
    bool bDisableMovementOnBeginPlay = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Testing")
    bool bTreatZeroHealthAsAliveForTesting = true;

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|State")
    void BP_OnDeath(AActor* Killer);

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|State")
    void BP_OnRevive();

private:
    UFUNCTION()
    void OnRep_Dead();

    void ApplyDeathMovementState(bool bInDeadState);

    FName ResolveTeamIDFromRelation() const;

    /** Tracks whether this pawn already pushed Controller::SetIgnoreMoveInput(true) for death lock. */
    UPROPERTY(Transient)
    bool bDeathMoveInputLocked = false;

    UPROPERTY(Transient)
    TObjectPtr<UAudioComponent> ActiveCastLoopAudioComponent;

    void PlaySkillExecuteVisualsLocal(UNiosCore_SkillDataAsset* Skill);
    void PlaySkillCastLoopVisualsLocal(UNiosCore_SkillDataAsset* Skill, bool bStart);

    UPROPERTY(ReplicatedUsing = OnRep_Dead)
    bool bDead = false;
};

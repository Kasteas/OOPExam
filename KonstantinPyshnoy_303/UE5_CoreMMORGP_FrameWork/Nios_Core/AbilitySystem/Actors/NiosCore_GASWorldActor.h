#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Rules/Targeting/NiosCore_TargetRelationTypes.h"
#include "NiosCore_GASWorldActor.generated.h"

class UNiosCore_AbilitySystemComponent;
class UNiosCore_AttributeSet;
class UNiosCore_StatsComponent;
class UNiosCore_StatusEffectComponent;
class UNiosCore_TargetFeedbackComponent;
class UAbilitySystemComponent;

UCLASS(Blueprintable)
class NIOS_CORE_API ANiosCore_GASWorldActor
    : public AActor
    , public IAbilitySystemInterface
    , public INiosGASActorInterface
{
    GENERATED_BODY()

public:
    ANiosCore_GASWorldActor();

    virtual void BeginPlay() override;
    virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintCallable, Category = "Nios|Ability")
    UNiosCore_AbilitySystemComponent* GetNiosAbilitySystemComponent() const;

    /** Local-only feedback component used by client-side target selection visuals. */
    UFUNCTION(BlueprintPure, Category = "Nios|Targeting|Feedback")
    UNiosCore_TargetFeedbackComponent* GetTargetFeedbackComponent() const { return TargetFeedbackComponent; }

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

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Ability")
    TObjectPtr<UNiosCore_AbilitySystemComponent> AbilitySystemComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Ability")
    TObjectPtr<UNiosCore_AttributeSet> AttributeSet;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Stats")
    TObjectPtr<UNiosCore_StatsComponent> StatsComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|StatusEffects")
    TObjectPtr<UNiosCore_StatusEffectComponent> StatusEffectComponent;

    /** Present on all base targetable world actors so BP can drive decals/outline/nameplate feedback. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Nios|Targeting|Feedback")
    TObjectPtr<UNiosCore_TargetFeedbackComponent> TargetFeedbackComponent;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Targeting")
    ENiosCoreTargetRelation RelationToPlayer = ENiosCoreTargetRelation::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Targeting")
    bool bCanBeTargeted = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Combat")
    bool bImmortal = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Combat")
    bool bCanDealDamage = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Testing")
    bool bTreatZeroHealthAsAliveForTesting = true;

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|State")
    void BP_OnDeath(AActor* Killer);

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|State")
    void BP_OnRevive();

private:
    FName ResolveTeamIDFromRelation() const;

    bool bDead = false;
};

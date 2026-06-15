#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/StatusEffects/NiosCore_StatusEffectTypes.h"
#include "Data/StatusEffects/NiosCore_CrowdControlPresentationCatalog.h"
#include "NiosCore_CrowdControlVisualComponent.generated.h"

class UNiosCore_StatusEffectComponent;
class UAnimMontage;
class UAnimInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNiosCrowdControlVisualChanged, const FNiosReplicatedStatusEffect&, CrowdControlEffect, const FNiosCrowdControlPresentationEntry&, Presentation);

/**
 * Client-side visual layer for CC loop animations.
 * Gameplay state remains in StatusEffectComponent; this component only resolves and plays presentation assets.
 */
UCLASS(ClassGroup = (Nios), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_CrowdControlVisualComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_CrowdControlVisualComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|CrowdControl|Visual")
    bool bAutoBindStatusEffectComponent = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|CrowdControl|Visual")
    bool bPlayNativeMontages = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|CrowdControl|Debug")
    bool bLogDebug = false;

    UPROPERTY(BlueprintAssignable, Category = "Nios|CrowdControl|Visual")
    FNiosCrowdControlVisualChanged OnCrowdControlVisualChanged;

    UFUNCTION(BlueprintCallable, Category = "Nios|CrowdControl|Visual")
    void InitializeCrowdControlVisuals(UNiosCore_StatusEffectComponent* InStatusEffectComponent);

    UFUNCTION(BlueprintCallable, Category = "Nios|CrowdControl|Visual")
    void StopCrowdControlVisuals(float BlendOutTime = 0.15f);

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|CrowdControl|Visual")
    ENiosCrowdControlType GetActiveCrowdControlType() const { return ActiveCrowdControlType; }

    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Nios|CrowdControl|Visual")
    FName GetActiveCrowdControlPresentationID() const { return ActiveCrowdControlPresentationID; }

protected:
    UFUNCTION()
    void HandleActiveCrowdControlChanged(const FNiosReplicatedStatusEffect& CrowdControlEffect);

    void ApplyPresentation(const FNiosReplicatedStatusEffect& CrowdControlEffect, const FNiosCrowdControlPresentationEntry& Presentation);
    void StartLoopMontage();
    UAnimInstance* ResolveAnimInstance() const;

protected:
    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_StatusEffectComponent> StatusEffectComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveLoopMontage = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> ActiveExitMontage = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UAnimMontage> PendingLoopMontage = nullptr;

    UPROPERTY(Transient)
    ENiosCrowdControlType ActiveCrowdControlType = ENiosCrowdControlType::None;

    UPROPERTY(Transient)
    FName ActiveCrowdControlPresentationID = NAME_None;

    FTimerHandle StartLoopTimerHandle;
};

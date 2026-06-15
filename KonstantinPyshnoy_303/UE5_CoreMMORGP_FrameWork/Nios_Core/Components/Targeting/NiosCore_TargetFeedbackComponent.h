#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NiosCore_TargetFeedbackComponent.generated.h"

class APlayerController;
class UDecalComponent;
class UNiosCore_AbilitySystemComponent;

UENUM(BlueprintType)
enum class ENiosCoreTargetFeedbackEventType : uint8
{
    Targeted UMETA(DisplayName = "Targeted"),
    Untargeted UMETA(DisplayName = "Untargeted")
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreTargetFeedbackContext
{
    GENERATED_BODY()

    /** Actor that received local target feedback. Usually this component owner. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    TObjectPtr<AActor> TargetActor = nullptr;

    /** Actor that selected this target, usually the local player pawn/avatar. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    TObjectPtr<AActor> TargetingActor = nullptr;

    /** Local player controller that owns this visual feedback. Not replicated. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    TObjectPtr<APlayerController> TargetingController = nullptr;

    /** Ability system that produced the target change, if available. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    TObjectPtr<UNiosCore_AbilitySystemComponent> TargetingAbilitySystem = nullptr;

    /** Previous locally displayed target before the change. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    TObjectPtr<AActor> PreviousTarget = nullptr;

    /** New locally displayed target after the change. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    TObjectPtr<AActor> NewTarget = nullptr;

    /** True when the change came from replicated/server-confirmed state. False for optimistic local preview. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    bool bServerConfirmed = false;

    /** Extensible reason for BP/VFX branching, for example PreviewSelected, ServerConfirmed, Rejected, Cleared. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|Target Feedback")
    FName Reason = NAME_None;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNiosCoreTargetFeedbackEventSignature, ENiosCoreTargetFeedbackEventType, EventType, const FNiosCoreTargetFeedbackContext&, Context);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosCoreTargetFeedbackContextSignature, const FNiosCoreTargetFeedbackContext&, Context);

/**
 * Local-only visual feedback receiver for actors that can be selected as a target.
 *
 * Add this component to mobs/NPCs/players that should react when the local player selects or deselects them.
 * It does not replicate and does not make gameplay decisions. Server target validation remains in ASC/rules.
 *
 * Typical BP usage:
 * - Add a DecalComponent under the actor, tag it with Nios.TargetDecal, keep it hidden.
 * - Add this component.
 * - Use BP_OnLocallyTargeted / BP_OnLocallyUntargeted to toggle outline, nameplate highlight, sound, decals, etc.
 */
UCLASS(ClassGroup=(NiosCore), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_TargetFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_TargetFeedbackComponent();

    virtual void BeginPlay() override;

    /** Dispatches local target feedback to TargetActor's UNiosCore_TargetFeedbackComponent, if present. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Target Feedback")
    static bool DispatchLocalTargetFeedback(AActor* TargetActor, ENiosCoreTargetFeedbackEventType EventType, const FNiosCoreTargetFeedbackContext& Context);

    /** Manually handles a local feedback event. Useful for custom BP/tool tests. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Target Feedback")
    void HandleLocalTargetFeedback(ENiosCoreTargetFeedbackEventType EventType, const FNiosCoreTargetFeedbackContext& Context);

    UFUNCTION(BlueprintPure, Category = "Nios|Target Feedback")
    bool IsLocallyTargeted() const { return bLocallyTargeted; }

    /** Re-finds decal component by configured tag/name. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Target Feedback")
    UDecalComponent* ResolveTargetDecalComponent();

    /** Directly toggles the configured target decal, if one is found. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Target Feedback")
    void SetTargetDecalVisible(bool bVisible);

    UPROPERTY(BlueprintAssignable, Category = "Nios|Target Feedback")
    FNiosCoreTargetFeedbackEventSignature OnLocalTargetFeedback;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Target Feedback")
    FNiosCoreTargetFeedbackContextSignature OnLocallyTargeted;

    UPROPERTY(BlueprintAssignable, Category = "Nios|Target Feedback")
    FNiosCoreTargetFeedbackContextSignature OnLocallyUntargeted;

protected:
    /** If true, component tries to hide target decal on BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Target Feedback|Decal")
    bool bHideTargetDecalOnBeginPlay = true;

    /** If true, targeted/untargeted events automatically show/hide the configured decal component. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Target Feedback|Decal")
    bool bAutoToggleTargetDecal = true;

    /** Preferred component tag for the decal that should be toggled by this component. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Target Feedback|Decal")
    FName TargetDecalComponentTag = TEXT("Nios.TargetDecal");

    /** Fallback component name if no component with TargetDecalComponentTag is found. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Target Feedback|Decal")
    FName TargetDecalComponentName = TEXT("TargetDecal");

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Target Feedback", meta = (DisplayName = "On Local Target Feedback"))
    void BP_OnLocalTargetFeedback(ENiosCoreTargetFeedbackEventType EventType, const FNiosCoreTargetFeedbackContext& Context);

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Target Feedback", meta = (DisplayName = "On Locally Targeted"))
    void BP_OnLocallyTargeted(const FNiosCoreTargetFeedbackContext& Context);

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Target Feedback", meta = (DisplayName = "On Locally Untargeted"))
    void BP_OnLocallyUntargeted(const FNiosCoreTargetFeedbackContext& Context);

private:
    UPROPERTY(Transient)
    bool bLocallyTargeted = false;

    UPROPERTY(Transient)
    TObjectPtr<UDecalComponent> CachedTargetDecalComponent = nullptr;
};

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/Feedback/NiosCore_ActionFeedbackTypes.h"
#include "NiosCore_ActionFeedbackComponent.generated.h"

class APlayerController;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FNiosCoreActionFailedSignature, FNiosCoreActionFailResult, Result);

/**
 * Local player feedback bridge for rejected gameplay actions.
 *
 * Intended owner: PlayerController.
 *
 * Server gameplay systems emit FNiosCoreActionFailResult via NotifyActionFailedForActor().
 * The result is delivered only to the owning client, then exposed to BP/UI/audio/VO through
 * OnActionFailed and BP_OnActionFailed.
 *
 * This component should NOT decide gameplay. It only presents feedback:
 * - floating error text;
 * - combat log line;
 * - resource bar highlight;
 * - error sound / voice line;
 * - hotbar button shake.
 */
UCLASS(ClassGroup = (NiosCore), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_ActionFeedbackComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_ActionFeedbackComponent();

    /** BP/UI can bind here to show localized messages, sounds, VO, highlights. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Action Feedback")
    FNiosCoreActionFailedSignature OnActionFailed;

    /** Ensures a PlayerController has this component. Useful while project BP controller is not yet parented to a C++ class. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Action Feedback")
    static UNiosCore_ActionFeedbackComponent* EnsureOnPlayerController(APlayerController* PlayerController);

    /** Finds the owning PlayerController for SourceActor and sends the failure to its owning client. Server-side entry point. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Action Feedback")
    static bool NotifyActionFailedForActor(AActor* SourceActor, FNiosCoreActionFailResult Result);

    /** Local/client handler. Safe to call manually from BP for debug. */
    UFUNCTION(BlueprintCallable, Category = "Nios|Action Feedback")
    void HandleActionFailed(FNiosCoreActionFailResult Result);

protected:
    UFUNCTION(Client, Reliable)
    void Client_ReceiveActionFailed(FNiosCoreActionFailResult Result);

    /** Override in BP_PlayerController component for UI/audio/VO. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|Action Feedback")
    void BP_OnActionFailed(FNiosCoreActionFailResult Result);
};

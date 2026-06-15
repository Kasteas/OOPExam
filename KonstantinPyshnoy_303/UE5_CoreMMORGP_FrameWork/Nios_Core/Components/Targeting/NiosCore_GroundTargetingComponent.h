#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "NiosCore_GroundTargetingComponent.generated.h"

UENUM(BlueprintType)
enum class ENiosCoreGroundTargetingFinishReason : uint8
{
    Confirmed UMETA(DisplayName = "Confirmed"),
    Cancelled UMETA(DisplayName = "Cancelled"),
    Invalid UMETA(DisplayName = "Invalid")
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FNiosGroundTargetingStateChangedSignature, bool, bActive, FName, SkillID);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNiosGroundTargetingUpdatedSignature, FName, SkillID, FVector, Location, bool, bValid);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FNiosGroundTargetingFinishedSignature, ENiosCoreGroundTargetingFinishReason, Reason, FName, SkillID, FVector, Location);

/**
 * Local PlayerController component for ground-targeting UX.
 *
 * Responsibility:
 * - Owns local cursor/world trace targeting mode.
 * - Provides Blueprint events for decals/previews/debug UI.
 * - Confirms/cancels pending ground skill selection.
 * - On confirm, asks ANiosCore_PlayerController to send the authoritative server request.
 *
 * Not responsible for:
 * - Applying effects.
 * - Validating final server combat rules.
 * - Replicating cooldowns/resources.
 * - Storing persistent gameplay state.
 */
UCLASS(ClassGroup=(NiosCore), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_GroundTargetingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_GroundTargetingComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    /** Starts local ground targeting for a pending skill. Usually called by BP/input when a GroundPoint skill is pressed. */
    UFUNCTION(BlueprintCallable, Category="Nios|Ground Targeting")
    void StartGroundTargeting(FName InSkillID, float InMaxRange = 0.f);

    /** Cancels local ground targeting. Bind this to RMB/ESC in BP. */
    UFUNCTION(BlueprintCallable, Category="Nios|Ground Targeting")
    void CancelGroundTargeting();

    /** Confirms current cursor location. Bind this to LMB in BP. Returns false if no valid targeting session/location exists. */
    UFUNCTION(BlueprintCallable, Category="Nios|Ground Targeting")
    bool ConfirmGroundTargeting();

    UFUNCTION(BlueprintPure, Category="Nios|Ground Targeting")
    bool IsGroundTargeting() const { return bGroundTargetingActive; }

    UFUNCTION(BlueprintPure, Category="Nios|Ground Targeting")
    FName GetPendingSkillID() const { return PendingSkillID; }

    UFUNCTION(BlueprintPure, Category="Nios|Ground Targeting")
    FVector GetCurrentTargetLocation() const { return CurrentTargetLocation; }

    UFUNCTION(BlueprintPure, Category="Nios|Ground Targeting")
    bool IsCurrentTargetValid() const { return bCurrentTargetValid; }

    UFUNCTION(BlueprintPure, Category="Nios|Ground Targeting")
    float GetMaxRange() const { return MaxRange; }

    UPROPERTY(BlueprintAssignable, Category="Nios|Ground Targeting")
    FNiosGroundTargetingStateChangedSignature OnGroundTargetingStateChanged;

    UPROPERTY(BlueprintAssignable, Category="Nios|Ground Targeting")
    FNiosGroundTargetingUpdatedSignature OnGroundTargetingUpdated;

    UPROPERTY(BlueprintAssignable, Category="Nios|Ground Targeting")
    FNiosGroundTargetingFinishedSignature OnGroundTargetingFinished;

protected:
    /** Trace channel used for cursor-to-world targeting. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nios|Ground Targeting")
    TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

    /** If true, target points farther than MaxRange are clamped from pawn location toward cursor hit. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nios|Ground Targeting")
    bool bClampToMaxRange = true;

    /** If true, local preview range is clamped using XY distance only. Server validation has the same default. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nios|Ground Targeting", meta=(EditCondition="bClampToMaxRange", EditConditionHides))
    bool bClampRangeIn2D = true;

    /** If true, no hit under cursor is treated as invalid unless a previous valid point can be reused on confirm. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nios|Ground Targeting")
    bool bRequireCursorHit = true;

    /**
     * Prevents accidental client-side cancel when the cursor trace is temporarily lost on click
     * (for example because the cursor is over UI, a decal, or a transient non-ground object).
     * The last valid clamped preview point is sent to the server instead; server validation remains authoritative.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Nios|Ground Targeting", meta=(EditCondition="bRequireCursorHit", EditConditionHides))
    bool bConfirmWithLastValidLocationOnTraceMiss = true;

    UFUNCTION(BlueprintImplementableEvent, Category="Nios|Ground Targeting", meta=(DisplayName="On Ground Targeting State Changed"))
    void BP_OnGroundTargetingStateChanged(bool bActive, FName SkillID);

    UFUNCTION(BlueprintImplementableEvent, Category="Nios|Ground Targeting", meta=(DisplayName="On Ground Targeting Updated"))
    void BP_OnGroundTargetingUpdated(FName SkillID, FVector Location, bool bValid);

    UFUNCTION(BlueprintImplementableEvent, Category="Nios|Ground Targeting", meta=(DisplayName="On Ground Targeting Finished"))
    void BP_OnGroundTargetingFinished(ENiosCoreGroundTargetingFinishReason Reason, FName SkillID, FVector Location);

private:
    bool UpdateCurrentTargetLocation();
    bool GetCursorGroundHit(FHitResult& OutHit) const;
    FVector GetSourceLocationForRange() const;
    void FinishGroundTargeting(ENiosCoreGroundTargetingFinishReason Reason, bool bSendToServer);

private:
    UPROPERTY(Transient)
    bool bGroundTargetingActive = false;

    UPROPERTY(Transient)
    bool bCurrentTargetValid = false;

    UPROPERTY(Transient)
    FName PendingSkillID = NAME_None;

    UPROPERTY(Transient)
    FVector CurrentTargetLocation = FVector::ZeroVector;

    UPROPERTY(Transient)
    bool bHasCurrentTargetLocation = false;

    UPROPERTY(Transient)
    float MaxRange = 0.f;
};

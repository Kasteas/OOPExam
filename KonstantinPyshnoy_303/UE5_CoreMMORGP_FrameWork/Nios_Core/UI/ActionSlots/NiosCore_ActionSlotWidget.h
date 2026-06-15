// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "Core/Nios_CoreStructers.h"
#include "UI/Tooltip/NiosCore_TooltipTypes.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "Components/Feedback/NiosCore_ActionFeedbackComponent.h"
#include "NiosCore_ActionSlotWidget.generated.h"


UENUM(BlueprintType)
enum class ENiosActionSlotRuntimeVisualState : uint8
{
    Ready UMETA(DisplayName = "Ready"),
    Pending UMETA(DisplayName = "Pending"),
    Accepted UMETA(DisplayName = "Accepted"),
    Executing UMETA(DisplayName = "Executing"),
    Success UMETA(DisplayName = "Success"),
    Rejected UMETA(DisplayName = "Rejected"),
    Interrupted UMETA(DisplayName = "Interrupted"),
    Failed UMETA(DisplayName = "Failed"),
    Cooldown UMETA(DisplayName = "Cooldown"),
    Disabled UMETA(DisplayName = "Disabled")
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosActionSlotRuntimeFeedback
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    ENiosActionSlotRuntimeVisualState State = ENiosActionSlotRuntimeVisualState::Ready;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    ENiosCoreActionFailReason FailureReason = ENiosCoreActionFailReason::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    FName ContextId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    TObjectPtr<AActor> TargetActor = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    float RequiredValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    float CurrentValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    float CooldownRemaining = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    float CooldownDuration = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    FName DebugReason = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    bool bPending = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    bool bExecuting = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    bool bHasFailure = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    bool bOnCooldown = false;

    bool IsTerminal() const
    {
        return State == ENiosActionSlotRuntimeVisualState::Success
            || State == ENiosActionSlotRuntimeVisualState::Rejected
            || State == ENiosActionSlotRuntimeVisualState::Interrupted
            || State == ENiosActionSlotRuntimeVisualState::Failed;
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosActionSlotRuntimeFeedbackChanged,
    int32, SlotIndex,
    FNiosActionSlotRuntimeFeedback, RuntimeFeedback
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosActionSlotWidgetEvent,
    int32, SlotIndex,
    FNiosActionSlotData, SlotData
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FNiosActionSlotHoverEvent,
    int32, SlotIndex,
    FNiosActionSlotData, SlotData,
    FNiosActionSlotVisualData, VisualData
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosActionSlotRightClicked,
    FNiosActionSlotData,
    SlotData
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRequestDrop, const FNiosActionSlotData&, SlotData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosActionSlotTooltipRequested,
    int32, SlotIndex,
    FNiosTooltipData, TooltipData
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FNiosActionSlotTooltipClosed,
    int32, SlotIndex
);

class UDragDropOperation;
class UNiosCore_DragVisualWidget;
class UActionSlotResolverSubsystem;
class USizeBox;

UCLASS(Blueprintable)
class NIOS_CORE_API UNiosCore_ActionSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativePreConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void SetSlotIndex(int32 InSlotIndex);

    void RefreshVisualDataFromResolver(UActionSlotResolverSubsystem* Resolver);

    UPROPERTY(BlueprintAssignable, Category = "Nios|ActionSlot")
    FOnRequestDrop OnRequestDrop;

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void SetSlotData(const FNiosActionSlotData& InSlotData);

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void SetVisualData(const FNiosActionSlotVisualData& InVisualData);

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void SetSlotAndVisualData(
        int32 InSlotIndex,
        const FNiosActionSlotData& InSlotData,
        const FNiosActionSlotVisualData& InVisualData
    );

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void SetEquippedState(bool bInEquipped);

    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot")
    bool IsEquipped() const;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    bool bEquipped = false;

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|ActionSlot")
    void BP_OnEquippedStateChanged(bool bNewEquipped);


    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Drag")
    TSubclassOf<UNiosCore_DragVisualWidget> DragVisualWidgetClass;

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void UpdateVisual(
        const FText& InName,
        const FText& InDescription,
        TSoftObjectPtr<UTexture2D> InIcon,
        int32 InCount,
        bool bInShowCount,
        bool bInEnabled,
        bool bInValid
    );

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    int32 GetSlotIndex() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    FNiosActionSlotData GetSlotData() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    FNiosActionSlotVisualData GetVisualData() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    bool BuildTooltipData(FNiosTooltipData& OutTooltipData);

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    void RequestTooltip();

    UFUNCTION(BlueprintCallable, Category = "Nios|Tooltip")
    void CloseTooltip();

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    bool IsEmpty() const;

    virtual FReply NativeOnMouseButtonDown(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent
    ) override;

    virtual void NativeOnDragDetected(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent,
        UDragDropOperation*& OutOperation
    ) override;

    UFUNCTION(BlueprintCallable, Category = "Nios|Slot")
    bool CanAcceptSlotData(
        const FNiosActionSlotData& InSlotData
    ) const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Slot")
    void ClearSlot();

    UFUNCTION(BlueprintCallable, Category = "Nios|Slot")
    bool IsEmptySlot() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|Slot")
    void SetSlotSize(FVector2D NewSize);
    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    void SetRuntimeFeedback(const FNiosActionSlotRuntimeFeedback& InRuntimeFeedback);

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    void ClearRuntimeFeedback();

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    void SetCooldownRuntimeState(float InCooldownRemaining, float InCooldownDuration, FName InDebugReason = NAME_None);

    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot|Runtime")
    FNiosActionSlotRuntimeFeedback GetRuntimeFeedback() const;

    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot|Runtime")
    bool IsRuntimePending() const;

    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot|Runtime")
    bool IsRuntimeExecuting() const;

    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot|Runtime")
    bool HasRuntimeFailure() const;

    /** Runtime execution feedback is meant for executable action slots. Inventory/equipment slots stay passive by default. */
    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot|Runtime")
    bool ShouldConsumeRuntimeFeedback() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    void ApplySkillActivationFeedback(const FNiosCoreSkillActivationFeedback& Feedback);

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    void ApplyActionFailResult(FNiosCoreActionFailResult Result);

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    bool BindToAbilitySystemFeedback(UNiosCore_AbilitySystemComponent* AbilitySystemComponent);

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    void UnbindFromAbilitySystemFeedback();

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    bool BindToActionFeedbackComponent(UNiosCore_ActionFeedbackComponent* ActionFeedbackComponent);

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    void UnbindFromActionFeedbackComponent();

    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot|Runtime")
    bool BindFeedbackFromOwningPlayer(bool bEnsureActionFeedbackComponent = true);

    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot|Runtime")
    bool MatchesActionContext(ENiosActionSlotType InActionType, FName InActionID) const;

    UFUNCTION(BlueprintPure, Category = "Nios|ActionSlot|Runtime")
    bool MatchesCurrentSlotContext(FName ContextId) const;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot")
    FVector2D DefaultSlotSize = FVector2D(100.f, 100.f);
    
    UPROPERTY(BlueprintAssignable, Category = "Nios|ActionSlot")
    FNiosActionSlotWidgetEvent OnSlotClicked;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<USizeBox> SizeBox_20 = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Slot")
    ENiosSlotInteractionProfile InteractionProfile = ENiosSlotInteractionProfile::Inventory;

    UPROPERTY(BlueprintAssignable)
    FNiosActionSlotRightClicked OnSlotRightClicked;

    UPROPERTY(BlueprintAssignable, Category = "Nios|ActionSlot")
    FNiosActionSlotWidgetEvent OnSlotShiftClicked;

    UPROPERTY(BlueprintAssignable, Category = "Nios|ActionSlot")
    FNiosActionSlotHoverEvent OnSlotHovered;

    UPROPERTY(BlueprintAssignable, Category = "Nios|ActionSlot")
    FNiosActionSlotWidgetEvent OnSlotUnhovered;

    /** Fired when this slot wants an external tooltip panel to open. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Tooltip")
    FNiosActionSlotTooltipRequested OnTooltipRequested;

    /** Fired when this slot wants an external tooltip panel to close. */
    UPROPERTY(BlueprintAssignable, Category = "Nios|Tooltip")
    FNiosActionSlotTooltipClosed OnTooltipClosed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bEnabled = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bValid = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    bool bDragDetected = false;

    /** Generic visual/runtime state for any action type: skills, items, emotes, future actions. */
    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot|Runtime")
    FNiosActionSlotRuntimeFeedback RuntimeFeedback;

    UPROPERTY(BlueprintAssignable, Category = "Nios|ActionSlot|Runtime")
    FNiosActionSlotRuntimeFeedbackChanged OnRuntimeFeedbackChanged;

    /** Inventory/equipment slots should not flash from skill/action feedback unless explicitly enabled. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    bool bEnableRuntimeFeedback = true;

    /** Safety default: only Hotbar slots consume execution feedback. Disable for special executable non-hotbar widgets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Runtime")
    bool bRuntimeFeedbackHotbarOnly = true;

    /** Safety default: generic C++ drag/drop handling is only for Hotbar. Inventory/equipment can keep their own BP/container logic. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Policy")
    bool bHandleDropInCppForNonHotbar = false;

    /** Hotbar policy: Item slots must resolve to usable items unless this is disabled or AllowedTypes overrides the intended profile. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Policy")
    bool bRequireUsableItemsOnHotbar = true;

    /** Disable only for decorative/preview slots. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot|Policy")
    bool bAllowDropOnThisSlot = true;
protected:
    virtual bool NativeOnDrop(
        const FGeometry& InGeometry,
        const FDragDropEvent& InDragDropEvent,
        UDragDropOperation* InOperation
    ) override;

    virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation);

    virtual FReply NativeOnMouseButtonUp(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent
    ) override;

    virtual void NativeOnMouseEnter(
        const FGeometry& InGeometry,
        const FPointerEvent& InMouseEvent
    ) override;

    virtual void NativeOnMouseLeave(
        const FPointerEvent& InMouseEvent
    ) override;

protected:
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|ActionSlot")
    void BP_OnSlotDataChanged();

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|ActionSlot")
    void BP_OnVisualDataChanged();

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|ActionSlot")
    void BP_OnPressedStateChanged(bool bPressed);

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|ActionSlot")
    void BP_OnHoveredStateChanged(bool bHovered);

    /** Override in WBP_ActionSlot to drive pending pulse, accepted flash, error shake, cooldown overlay, etc. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|ActionSlot|Runtime")
    void BP_OnRuntimeFeedbackChanged(FNiosActionSlotRuntimeFeedback NewRuntimeFeedback);

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    int32 SlotIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    FNiosActionSlotData SlotData;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    FNiosActionSlotVisualData VisualData;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    bool bPressedInside = false;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    bool bIsHovered = false;

    public:
        UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
        void SetAllowedTypes(const TArray<ENiosActionSlotType>& InAllowedTypes);



protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|ActionSlot")
    TArray<ENiosActionSlotType> AllowedTypes;


    public:

    void AddCount(int32 Delta);

    // � UNiosCore_ActionSlotWidget.h
protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|ActionSlot")
    UActionSlotResolverSubsystem* ResolverSubsystem = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_AbilitySystemComponent> BoundAbilitySystemComponent = nullptr;

    UPROPERTY(Transient)
    TObjectPtr<UNiosCore_ActionFeedbackComponent> BoundActionFeedbackComponent = nullptr;

    UFUNCTION()
    void HandleBoundSkillActivationFeedback(const FNiosCoreSkillActivationFeedback& Feedback);

    UFUNCTION()
    void HandleBoundActionFailed(FNiosCoreActionFailResult Result);

    bool CanAcceptSlotDataByProfile(const FNiosActionSlotData& InSlotData) const;

public:
    // ������� ��� ���������/������������� ����������
    UFUNCTION(BlueprintCallable, Category = "Nios|ActionSlot")
    void InitResolverSubsystem();

    UFUNCTION(BlueprintCallable, Category = "Slot")
    void InitSlotSize();

    UFUNCTION(BlueprintCallable, Category = "Slot")
    void InitSlotData(const FNiosActionSlotData& InSlotData);
};

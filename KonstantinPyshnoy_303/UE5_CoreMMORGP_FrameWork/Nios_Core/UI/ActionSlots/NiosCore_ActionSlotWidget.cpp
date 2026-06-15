#include "UI/ActionSlots/NiosCore_ActionSlotWidget.h"

#include "UI/ActionSlots/NiosCore_DragVisualWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Data/Resolvers/ActionSlotResolverSubsystem.h"
#include "UI/ActionSlots/Operations/NiosActionSlotDragDropOperation.h"
#include "Components/SizeBox.h"
#include "InputCoreTypes.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UNiosCore_ActionSlotWidget::NativePreConstruct()
{
    Super::NativePreConstruct();

    // ������������� ������ �� ��������� ��� ����������
    InitSlotSize();

}

void UNiosCore_ActionSlotWidget::NativeDestruct()
{
    UnbindFromAbilitySystemFeedback();
    UnbindFromActionFeedbackComponent();
    Super::NativeDestruct();
}

void UNiosCore_ActionSlotWidget::SetSlotIndex(int32 InSlotIndex)
{
    SlotIndex = InSlotIndex;
}

void UNiosCore_ActionSlotWidget::SetSlotData(const FNiosActionSlotData& InSlotData)
{
    if (InteractionProfile == ENiosSlotInteractionProfile::SkillBook)
    {
        // ����������� ���� � �� ������ ������
        return;
    }
    const bool bActionChanged = SlotData.Type != InSlotData.Type || SlotData.ActionID != InSlotData.ActionID;
    SlotData = InSlotData;
    if (bActionChanged)
    {
        ClearRuntimeFeedback();
    }
    BP_OnSlotDataChanged();
}

void UNiosCore_ActionSlotWidget::SetVisualData(const FNiosActionSlotVisualData& InVisualData)
{
    VisualData = InVisualData;
    BP_OnVisualDataChanged();
}

void UNiosCore_ActionSlotWidget::SetSlotSize(FVector2D NewSize)
{
    if (!SizeBox_20) return;

    SizeBox_20->SetWidthOverride(NewSize.X);
    SizeBox_20->SetHeightOverride(NewSize.Y);
}

void UNiosCore_ActionSlotWidget::SetSlotAndVisualData(
    int32 InSlotIndex,
    const FNiosActionSlotData& InSlotData,
    const FNiosActionSlotVisualData& InVisualData)
{
    const bool bActionChanged = SlotData.Type != InSlotData.Type || SlotData.ActionID != InSlotData.ActionID;
    SlotIndex = InSlotIndex;
    SlotData = InSlotData;
    VisualData = InVisualData;

    if (bActionChanged)
    {
        ClearRuntimeFeedback();
    }

    BP_OnSlotDataChanged();
    BP_OnVisualDataChanged();
}

void UNiosCore_ActionSlotWidget::SetEquippedState(bool bInEquipped)
{
    bEquipped = bInEquipped;
    BP_OnEquippedStateChanged(bEquipped);
}

bool UNiosCore_ActionSlotWidget::IsEquipped() const
{
    return bEquipped;
}

void UNiosCore_ActionSlotWidget::ClearSlot()
{
    SlotData = FNiosActionSlotData();
    VisualData = FNiosActionSlotVisualData();

    bPressedInside = false;
    bIsHovered = false;
    bDragDetected = false;
    SetEquippedState(false);
    ClearRuntimeFeedback();
    CloseTooltip();
    BP_OnPressedStateChanged(false);
    BP_OnHoveredStateChanged(false);
    BP_OnSlotDataChanged();
    BP_OnVisualDataChanged();
}

void UNiosCore_ActionSlotWidget::UpdateVisual(
    const FText& InName,
    const FText& InDescription,
    TSoftObjectPtr<UTexture2D> InIcon,
    int32 InCount,
    bool bInShowCount,
    bool bInEnabled,
    bool bInValid)
{
    VisualData.Name = InName;
    VisualData.Description = InDescription;
    VisualData.Icon = InIcon;
    VisualData.Count = InCount;
    VisualData.bShowCount = bInShowCount;
    VisualData.bEnabled = bInEnabled;
    VisualData.bValid = bInValid;

    BP_OnVisualDataChanged();
}

int32 UNiosCore_ActionSlotWidget::GetSlotIndex() const
{
    return SlotIndex;
}

FNiosActionSlotData UNiosCore_ActionSlotWidget::GetSlotData() const
{
    return SlotData;
}

FNiosActionSlotVisualData UNiosCore_ActionSlotWidget::GetVisualData() const
{
    return VisualData;
}


bool UNiosCore_ActionSlotWidget::BuildTooltipData(FNiosTooltipData& OutTooltipData)
{
    OutTooltipData.Reset();

    if (SlotData.IsEmpty())
    {
        return false;
    }

    if (!ResolverSubsystem)
    {
        InitResolverSubsystem();
    }

    if (ResolverSubsystem && ResolverSubsystem->ResolveTooltipData(SlotData, OutTooltipData))
    {
        return true;
    }

    // Safe fallback: if resolver cannot build a rich tooltip, still expose current visual data.
    if (VisualData.bValid)
    {
        OutTooltipData.bValid = true;
        OutTooltipData.SourceID = SlotData.ActionID;
        OutTooltipData.Title = VisualData.Name;
        OutTooltipData.Description = VisualData.Description;
        OutTooltipData.Icon = VisualData.Icon;
        OutTooltipData.Count = VisualData.Count;
        OutTooltipData.bShowCount = VisualData.bShowCount;

        switch (SlotData.Type)
        {
        case ENiosActionSlotType::Skill:
            OutTooltipData.Type = ENiosTooltipType::Skill;
            OutTooltipData.Subtitle = FText::FromString(TEXT("Skill"));
            break;
        case ENiosActionSlotType::Item:
            OutTooltipData.Type = ENiosTooltipType::Item;
            OutTooltipData.Subtitle = FText::FromString(TEXT("Item"));
            break;
        default:
            OutTooltipData.Type = ENiosTooltipType::Debug;
            break;
        }

        return true;
    }

    return false;
}

void UNiosCore_ActionSlotWidget::RequestTooltip()
{
    FNiosTooltipData TooltipData;
    if (BuildTooltipData(TooltipData) && TooltipData.bValid)
    {
        OnTooltipRequested.Broadcast(SlotIndex, TooltipData);
    }
}

void UNiosCore_ActionSlotWidget::CloseTooltip()
{
    OnTooltipClosed.Broadcast(SlotIndex);
}

bool UNiosCore_ActionSlotWidget::IsEmpty() const
{
    return SlotData.IsEmpty();
}



bool UNiosCore_ActionSlotWidget::IsEmptySlot() const
{
    return SlotData.IsEmpty();
}



void UNiosCore_ActionSlotWidget::SetAllowedTypes(
    const TArray<ENiosActionSlotType>& InAllowedTypes)
{
    AllowedTypes = InAllowedTypes;
}

void UNiosCore_ActionSlotWidget::AddCount(int32 Delta)
{
    SlotData.Count = FMath::Max(0, SlotData.Count + Delta);
    VisualData.Count = SlotData.Count;

    BP_OnSlotDataChanged();
    BP_OnVisualDataChanged();
}


void UNiosCore_ActionSlotWidget::SetRuntimeFeedback(const FNiosActionSlotRuntimeFeedback& InRuntimeFeedback)
{
    RuntimeFeedback = InRuntimeFeedback;
    OnRuntimeFeedbackChanged.Broadcast(SlotIndex, RuntimeFeedback);
    BP_OnRuntimeFeedbackChanged(RuntimeFeedback);
}

void UNiosCore_ActionSlotWidget::ClearRuntimeFeedback()
{
    FNiosActionSlotRuntimeFeedback NewFeedback;
    NewFeedback.ContextId = SlotData.ActionID;
    NewFeedback.State = VisualData.bEnabled ? ENiosActionSlotRuntimeVisualState::Ready : ENiosActionSlotRuntimeVisualState::Disabled;
    SetRuntimeFeedback(NewFeedback);
}

void UNiosCore_ActionSlotWidget::SetCooldownRuntimeState(float InCooldownRemaining, float InCooldownDuration, FName InDebugReason)
{
    FNiosActionSlotRuntimeFeedback NewFeedback;
    NewFeedback.State = ENiosActionSlotRuntimeVisualState::Cooldown;
    NewFeedback.ContextId = SlotData.ActionID;
    NewFeedback.CooldownRemaining = FMath::Max(0.f, InCooldownRemaining);
    NewFeedback.CooldownDuration = FMath::Max(0.f, InCooldownDuration);
    NewFeedback.bOnCooldown = NewFeedback.CooldownDuration > 0.f && NewFeedback.CooldownRemaining > 0.f;
    NewFeedback.DebugReason = InDebugReason;
    SetRuntimeFeedback(NewFeedback);
}

FNiosActionSlotRuntimeFeedback UNiosCore_ActionSlotWidget::GetRuntimeFeedback() const
{
    return RuntimeFeedback;
}

bool UNiosCore_ActionSlotWidget::IsRuntimePending() const
{
    return RuntimeFeedback.bPending;
}

bool UNiosCore_ActionSlotWidget::IsRuntimeExecuting() const
{
    return RuntimeFeedback.bExecuting;
}

bool UNiosCore_ActionSlotWidget::HasRuntimeFailure() const
{
    return RuntimeFeedback.bHasFailure;
}

bool UNiosCore_ActionSlotWidget::ShouldConsumeRuntimeFeedback() const
{
    if (!bEnableRuntimeFeedback)
    {
        return false;
    }

    return !bRuntimeFeedbackHotbarOnly || InteractionProfile == ENiosSlotInteractionProfile::Hotbar;
}

bool UNiosCore_ActionSlotWidget::MatchesActionContext(ENiosActionSlotType InActionType, FName InActionID) const
{
    if (SlotData.IsEmpty() || InActionID.IsNone())
    {
        return false;
    }

    return SlotData.Type == InActionType && SlotData.ActionID == InActionID;
}

bool UNiosCore_ActionSlotWidget::MatchesCurrentSlotContext(FName ContextId) const
{
    if (SlotData.IsEmpty() || ContextId.IsNone())
    {
        return false;
    }

    return SlotData.ActionID == ContextId;
}

void UNiosCore_ActionSlotWidget::ApplySkillActivationFeedback(const FNiosCoreSkillActivationFeedback& Feedback)
{
    if (!ShouldConsumeRuntimeFeedback())
    {
        return;
    }

    if (!MatchesActionContext(ENiosActionSlotType::Skill, Feedback.SkillID))
    {
        return;
    }

    FNiosActionSlotRuntimeFeedback NewFeedback;
    NewFeedback.ContextId = Feedback.SkillID;
    NewFeedback.FailureReason = Feedback.FailureReason;
    NewFeedback.TargetActor = Feedback.TargetActor;
    NewFeedback.RequiredValue = Feedback.RequiredValue;
    NewFeedback.CurrentValue = Feedback.CurrentValue;
    NewFeedback.DebugReason = Feedback.DebugReason;

    switch (Feedback.State)
    {
    case ENiosCoreSkillActivationFeedbackState::Requested:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Pending;
        NewFeedback.bPending = true;
        break;

    case ENiosCoreSkillActivationFeedbackState::Accepted:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Accepted;
        NewFeedback.bPending = true;
        break;

    case ENiosCoreSkillActivationFeedbackState::Executing:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Executing;
        NewFeedback.bExecuting = true;
        break;

    case ENiosCoreSkillActivationFeedbackState::Executed:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Success;
        break;

    case ENiosCoreSkillActivationFeedbackState::Rejected:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Rejected;
        NewFeedback.bHasFailure = true;
        break;

    case ENiosCoreSkillActivationFeedbackState::Interrupted:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Interrupted;
        NewFeedback.bHasFailure = true;
        break;

    case ENiosCoreSkillActivationFeedbackState::Failed:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Failed;
        NewFeedback.bHasFailure = true;
        break;

    default:
        NewFeedback.State = ENiosActionSlotRuntimeVisualState::Ready;
        break;
    }

    SetRuntimeFeedback(NewFeedback);
}

void UNiosCore_ActionSlotWidget::ApplyActionFailResult(FNiosCoreActionFailResult Result)
{
    if (!ShouldConsumeRuntimeFeedback())
    {
        return;
    }

    if (!MatchesCurrentSlotContext(Result.ContextId))
    {
        return;
    }

    FNiosActionSlotRuntimeFeedback NewFeedback;
    NewFeedback.State = ENiosActionSlotRuntimeVisualState::Rejected;
    NewFeedback.FailureReason = Result.Reason;
    NewFeedback.ContextId = Result.ContextId;
    NewFeedback.TargetActor = Result.TargetActor;
    NewFeedback.RequiredValue = Result.RequiredValue;
    NewFeedback.CurrentValue = Result.CurrentValue;
    NewFeedback.DebugReason = Result.DebugReason;
    NewFeedback.bHasFailure = true;

    SetRuntimeFeedback(NewFeedback);
}

bool UNiosCore_ActionSlotWidget::BindToAbilitySystemFeedback(UNiosCore_AbilitySystemComponent* AbilitySystemComponent)
{
    if (BoundAbilitySystemComponent == AbilitySystemComponent && BoundAbilitySystemComponent)
    {
        return true;
    }

    UnbindFromAbilitySystemFeedback();

    if (!AbilitySystemComponent)
    {
        return false;
    }

    BoundAbilitySystemComponent = AbilitySystemComponent;
    BoundAbilitySystemComponent->OnSkillActivationFeedback.AddDynamic(this, &UNiosCore_ActionSlotWidget::HandleBoundSkillActivationFeedback);
    return true;
}

void UNiosCore_ActionSlotWidget::UnbindFromAbilitySystemFeedback()
{
    if (BoundAbilitySystemComponent)
    {
        BoundAbilitySystemComponent->OnSkillActivationFeedback.RemoveDynamic(this, &UNiosCore_ActionSlotWidget::HandleBoundSkillActivationFeedback);
        BoundAbilitySystemComponent = nullptr;
    }
}

bool UNiosCore_ActionSlotWidget::BindToActionFeedbackComponent(UNiosCore_ActionFeedbackComponent* ActionFeedbackComponent)
{
    if (BoundActionFeedbackComponent == ActionFeedbackComponent && BoundActionFeedbackComponent)
    {
        return true;
    }

    UnbindFromActionFeedbackComponent();

    if (!ActionFeedbackComponent)
    {
        return false;
    }

    BoundActionFeedbackComponent = ActionFeedbackComponent;
    BoundActionFeedbackComponent->OnActionFailed.AddDynamic(this, &UNiosCore_ActionSlotWidget::HandleBoundActionFailed);
    return true;
}

void UNiosCore_ActionSlotWidget::UnbindFromActionFeedbackComponent()
{
    if (BoundActionFeedbackComponent)
    {
        BoundActionFeedbackComponent->OnActionFailed.RemoveDynamic(this, &UNiosCore_ActionSlotWidget::HandleBoundActionFailed);
        BoundActionFeedbackComponent = nullptr;
    }
}

bool UNiosCore_ActionSlotWidget::BindFeedbackFromOwningPlayer(bool bEnsureActionFeedbackComponent)
{
    if (!ShouldConsumeRuntimeFeedback())
    {
        return false;
    }

    bool bBoundAny = false;

    if (APawn* OwningPawn = GetOwningPlayerPawn())
    {
        if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningPawn))
        {
            bBoundAny |= BindToAbilitySystemFeedback(Cast<UNiosCore_AbilitySystemComponent>(ASC));
        }
    }

    if (APlayerController* OwningPC = GetOwningPlayer())
    {
        UNiosCore_ActionFeedbackComponent* FeedbackComponent = OwningPC->FindComponentByClass<UNiosCore_ActionFeedbackComponent>();
        if (!FeedbackComponent && bEnsureActionFeedbackComponent)
        {
            FeedbackComponent = UNiosCore_ActionFeedbackComponent::EnsureOnPlayerController(OwningPC);
        }

        bBoundAny |= BindToActionFeedbackComponent(FeedbackComponent);
    }

    return bBoundAny;
}

void UNiosCore_ActionSlotWidget::HandleBoundSkillActivationFeedback(const FNiosCoreSkillActivationFeedback& Feedback)
{
    ApplySkillActivationFeedback(Feedback);
}

void UNiosCore_ActionSlotWidget::HandleBoundActionFailed(FNiosCoreActionFailResult Result)
{
    ApplyActionFailResult(Result);
}


bool UNiosCore_ActionSlotWidget::CanAcceptSlotData(
    const FNiosActionSlotData& InSlotData) const
{
    if (InSlotData.IsEmpty())
    {
        return true;
    }

    if (AllowedTypes.Num() > 0 && !AllowedTypes.Contains(InSlotData.Type))
    {
        return false;
    }

    return CanAcceptSlotDataByProfile(InSlotData);
}

bool UNiosCore_ActionSlotWidget::CanAcceptSlotDataByProfile(const FNiosActionSlotData& InSlotData) const
{
    if (InSlotData.IsEmpty())
    {
        return true;
    }

    switch (InteractionProfile)
    {
    case ENiosSlotInteractionProfile::Hotbar:
    {
        if (InSlotData.Type == ENiosActionSlotType::Skill || InSlotData.Type == ENiosActionSlotType::Emote)
        {
            return true;
        }

        if (InSlotData.Type == ENiosActionSlotType::Item)
        {
            if (!bRequireUsableItemsOnHotbar)
            {
                return true;
            }

            if (ResolverSubsystem)
            {
                return ResolverSubsystem->UIsItemUsable(InSlotData);
            }

            // If resolver is not initialized yet, allow BP/root to resolve later instead of blocking future action types forever.
            return true;
        }

        return false;
    }

    case ENiosSlotInteractionProfile::SkillBook:
        return InSlotData.Type == ENiosActionSlotType::Skill;

    case ENiosSlotInteractionProfile::Inventory:
        return InSlotData.Type == ENiosActionSlotType::Item;

    case ENiosSlotInteractionProfile::Equipment:
        return InSlotData.Type == ENiosActionSlotType::Item;

    default:
        return true;
    }
}

FReply UNiosCore_ActionSlotWidget::NativeOnMouseButtonDown(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (SlotData.IsEmpty())
    {
        return Super::NativeOnMouseButtonDown(
            InGeometry,
            InMouseEvent
        );
    }

    const FKey Button =
        InMouseEvent.GetEffectingButton();

    const bool bShiftDown =
        InMouseEvent.IsShiftDown();

    // RIGHT CLICK

    if (Button == EKeys::RightMouseButton)
    {
        OnSlotRightClicked.Broadcast(SlotData);

        return FReply::Handled();
    }

    // LEFT CLICK

    if (Button == EKeys::LeftMouseButton)
    {
        bDragDetected = false;

        bool bShouldStartDrag = false;

        switch (InteractionProfile)
        {
        case ENiosSlotInteractionProfile::SkillBook:
        {
            bShouldStartDrag = bShiftDown;
            break;
        }

        case ENiosSlotInteractionProfile::Hotbar:
        {
            bShouldStartDrag = bShiftDown;
            break;
        }

        case ENiosSlotInteractionProfile::Inventory:
        {
            bShouldStartDrag = true;
            break;
        }

        case ENiosSlotInteractionProfile::Equipment:
        {
            bShouldStartDrag = true;
            break;
        }

        default:
        {
            bShouldStartDrag = false;
            break;
        }
        }

        // START DRAG

        if (bShouldStartDrag)
        {
            return UWidgetBlueprintLibrary::DetectDragIfPressed(
                InMouseEvent,
                this,
                EKeys::LeftMouseButton
            ).NativeReply;
        }

        // NORMAL CLICK

        bPressedInside = true;

        BP_OnPressedStateChanged(true);

        return FReply::Handled()
            .CaptureMouse(TakeWidget());
    }

    return Super::NativeOnMouseButtonDown(
        InGeometry,
        InMouseEvent
    );
}

FReply UNiosCore_ActionSlotWidget::NativeOnMouseButtonUp(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() ==
        EKeys::LeftMouseButton)
    {
        const bool bWasPressedInside =
            bPressedInside;

        bPressedInside = false;

        BP_OnPressedStateChanged(false);

        // CLICK ON RELEASE

        if (
            bWasPressedInside &&
            !bDragDetected &&
            VisualData.bEnabled &&
            !SlotData.IsEmpty()
            )
        {
            if (InMouseEvent.IsShiftDown())
            {
                OnSlotShiftClicked.Broadcast(
                    SlotIndex,
                    SlotData
                );
            }
            else
            {
                OnSlotClicked.Broadcast(
                    SlotIndex,
                    SlotData
                );
            }
        }

        bDragDetected = false;

        return FReply::Handled()
            .ReleaseMouseCapture();
    }

    return Super::NativeOnMouseButtonUp(
        InGeometry,
        InMouseEvent
    );
}

void UNiosCore_ActionSlotWidget::NativeOnDragDetected(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent,
    UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

    if (SlotData.IsEmpty())
        return;

    bDragDetected = true;
    bPressedInside = false;
    BP_OnPressedStateChanged(false);

    UNiosActionSlotDragDropOperation* DragOp =
        NewObject<UNiosActionSlotDragDropOperation>();

    DragOp->SlotData = SlotData;
    DragOp->VisualData = VisualData;
    DragOp->SourceSlot = this;
    DragOp->bSourceIsStatic = (InteractionProfile == ENiosSlotInteractionProfile::SkillBook);

    // ������ ���������� ������ ��� ��������������
    if (DragVisualWidgetClass)
    {
        UNiosCore_DragVisualWidget* DragVisual =
            CreateWidget<UNiosCore_DragVisualWidget>(GetOwningPlayer(), DragVisualWidgetClass);

        if (DragVisual)
        {
            DragVisual->SetVisualData(VisualData);
            DragOp->DefaultDragVisual = DragVisual;
            DragOp->Pivot = EDragPivot::MouseDown;
        }
    }

    OutOperation = DragOp;
}

bool UNiosCore_ActionSlotWidget::NativeOnDrop(
    const FGeometry& InGeometry,
    const FDragDropEvent& InDragDropEvent,
    UDragDropOperation* InOperation)
{
    const UNiosActionSlotDragDropOperation* DragOp = Cast<UNiosActionSlotDragDropOperation>(InOperation);
    if (!DragOp || DragOp->SlotData.IsEmpty())
        return false;

    if (InteractionProfile != ENiosSlotInteractionProfile::Hotbar && !bHandleDropInCppForNonHotbar)
    {
        return Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);
    }

    if (!bAllowDropOnThisSlot)
        return false;

    if (!ResolverSubsystem)
    {
        const_cast<UNiosCore_ActionSlotWidget*>(this)->InitResolverSubsystem();
    }

    if (!CanAcceptSlotData(DragOp->SlotData))
        return false;



    // ���� ���� � ������� ����� Hotbar
    if (DragOp->SourceSlot && DragOp->SourceSlot != this)
    {
        // Swap ������
        FNiosActionSlotData OldData = SlotData;
        FNiosActionSlotVisualData OldVisual = VisualData;

        SetSlotAndVisualData(SlotIndex, DragOp->SlotData, DragOp->VisualData);

        // ���� �������� ���� ���� Hotbar � �� ����������� � ��������� ���
        if (!DragOp->bSourceIsStatic && DragOp->SourceSlot->InteractionProfile == ENiosSlotInteractionProfile::Hotbar)
        {
            if (OldData.IsEmpty()) // ���� � �������� ����� �����, ������ ������� ��������
                DragOp->SourceSlot->ClearSlot();
            else // ����� ����
                DragOp->SourceSlot->SetSlotAndVisualData(
                    DragOp->SourceSlot->GetSlotIndex(),
                    OldData,
                    OldVisual
                );
        }

        return true;
    }

    // �������������� � �������� ��������� (Inventory/SkillBook)
    SetSlotAndVisualData(SlotIndex, DragOp->SlotData, DragOp->VisualData);
    return true;
}

void UNiosCore_ActionSlotWidget::NativeOnMouseEnter(
    const FGeometry& InGeometry,
    const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

    bIsHovered = true;
    BP_OnHoveredStateChanged(true);

    if (!SlotData.IsEmpty() && VisualData.bValid)
    {
        OnSlotHovered.Broadcast(SlotIndex, SlotData, VisualData);
    }

    RequestTooltip();
}

void UNiosCore_ActionSlotWidget::RefreshVisualDataFromResolver(UActionSlotResolverSubsystem* Resolver)
{
    if (!Resolver || SlotData.IsEmpty())
        return;

    FNiosActionSlotVisualData Visual;
    if (Resolver->ResolveVisualData(SlotData, Visual))
    {
        SetVisualData(Visual);
    }
    else
    {
        ClearSlot();
    }
}

void UNiosCore_ActionSlotWidget::NativeOnMouseLeave(
    const FPointerEvent& InMouseEvent)
{
    bIsHovered = false;
    bPressedInside = false;

    BP_OnHoveredStateChanged(false);
    BP_OnPressedStateChanged(false);

    OnSlotUnhovered.Broadcast(SlotIndex, SlotData);
    CloseTooltip();

    Super::NativeOnMouseLeave(InMouseEvent);
}

void UNiosCore_ActionSlotWidget::InitSlotSize()
{
    if (SizeBox_20)
    {
        SizeBox_20->SetWidthOverride(DefaultSlotSize.X);
        SizeBox_20->SetHeightOverride(DefaultSlotSize.Y);
    }
}

void UNiosCore_ActionSlotWidget::InitSlotData(const FNiosActionSlotData& InSlotData)
{
    const bool bActionChanged = SlotData.Type != InSlotData.Type || SlotData.ActionID != InSlotData.ActionID;
    SlotData = InSlotData;
    if (bActionChanged)
    {
        ClearRuntimeFeedback();
    }

    // ��������� ����� ��������� ���������� ������ ����� ����������
    if (!ResolverSubsystem)
    {
        InitResolverSubsystem();
    }

    if (ResolverSubsystem && !SlotData.IsEmpty())
    {
        FNiosActionSlotVisualData OutVisual;
        if (ResolverSubsystem->ResolveVisualData(SlotData, OutVisual))
        {
            VisualData = OutVisual;
            BP_OnVisualDataChanged();
        }
    }

    BP_OnSlotDataChanged();
}

void UNiosCore_ActionSlotWidget::InitResolverSubsystem()
{
    if (ResolverSubsystem)
        return; // ��� ����

    if (UWorld* World = GetWorld())
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            ResolverSubsystem = GI->GetSubsystem<UActionSlotResolverSubsystem>();
        }
    }

    if (!ResolverSubsystem)
    {
        UE_LOG(LogTemp, Warning, TEXT("ActionSlotWidget: ResolverSubsystem not found!"));
    }
}

void UNiosCore_ActionSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    // ���� ����� ������� ������� ��� Inventory ��� ������ �� �����
    if (InteractionProfile == ENiosSlotInteractionProfile::Inventory)
    {
        if (OnRequestDrop.IsBound() && InOperation)
        {
            const UNiosActionSlotDragDropOperation* DragOp = Cast<UNiosActionSlotDragDropOperation>(InOperation);
            if (DragOp)
            {
                OnRequestDrop.Broadcast(DragOp->SlotData);
            }
        }
    }

    // �������� ������� ����������
    Super::NativeOnDragCancelled(InDragDropEvent, InOperation);
}
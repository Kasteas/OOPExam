#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Nios_CoreStructers.h"
#include "NiosCore_SkillBookEntryWidget.generated.h"

class UButton;
class UImage;
class UNiosCore_ActionSlotWidget;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FNiosSkillBookEntrySelected,
    FName,
    SkillID,
    FNiosActionSlotData,
    SlotData
);

UCLASS(Blueprintable)
class NIOS_CORE_API UNiosCore_SkillBookEntryWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    void SetSkillEntry(
        FName InSkillID,
        const FNiosActionSlotData& InSlotData,
        const FNiosActionSlotVisualData& InVisualData
    );

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    void SetSelected(bool bInSelected);

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    FName GetSkillID() const;

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    FNiosActionSlotData GetSlotData() const;

    UFUNCTION(BlueprintImplementableEvent, Category = "Nios|SkillBook")
    void BP_SetSkillName(const FText& InSkillName);

public:
    UPROPERTY(BlueprintAssignable, Category = "Nios|SkillBook")
    FNiosSkillBookEntrySelected OnSkillBookEntrySelected;

protected:
    UFUNCTION()
    void HandleInfoAreaClicked();

protected:
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    TObjectPtr<UButton> Button_InfoArea;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    TObjectPtr<UNiosCore_ActionSlotWidget> SkillIconSlot;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly)
    TObjectPtr<UImage> Image_SelectedGlow;

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Nios|SkillBook")
    FName SkillID = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|SkillBook")
    FNiosActionSlotData SlotData;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|SkillBook")
    FNiosActionSlotVisualData VisualData;
};
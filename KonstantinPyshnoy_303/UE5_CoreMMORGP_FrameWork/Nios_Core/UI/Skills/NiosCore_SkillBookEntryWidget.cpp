#include "UI/Skills/NiosCore_SkillBookEntryWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"

#include "UI/ActionSlots/NiosCore_ActionSlotWidget.h"

void UNiosCore_SkillBookEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Button_InfoArea)
    {
        Button_InfoArea->OnClicked.AddDynamic(
            this,
            &UNiosCore_SkillBookEntryWidget::HandleInfoAreaClicked
        );
    }

    SetSelected(false);
}

void UNiosCore_SkillBookEntryWidget::SetSkillEntry(
    FName InSkillID,
    const FNiosActionSlotData& InSlotData,
    const FNiosActionSlotVisualData& InVisualData)
{
    SkillID = InSkillID;
    SlotData = InSlotData;
    VisualData = InVisualData;
    BP_SetSkillName(VisualData.Name);

    if (SkillIconSlot)
    {
        TArray<ENiosActionSlotType> AllowedTypes;
        AllowedTypes.Add(ENiosActionSlotType::Skill);

        SkillIconSlot->SetAllowedTypes(AllowedTypes);

        SkillIconSlot->SetSlotAndVisualData(
            0,
            SlotData,
            VisualData
        );
    }
}

void UNiosCore_SkillBookEntryWidget::SetSelected(bool bInSelected)
{
    if (!Image_SelectedGlow)
        return;

    Image_SelectedGlow->SetVisibility(
        bInSelected
        ? ESlateVisibility::HitTestInvisible
        : ESlateVisibility::Collapsed
    );
}

FName UNiosCore_SkillBookEntryWidget::GetSkillID() const
{
    return SkillID;
}

FNiosActionSlotData UNiosCore_SkillBookEntryWidget::GetSlotData() const
{
    return SlotData;
}

void UNiosCore_SkillBookEntryWidget::HandleInfoAreaClicked()
{
    OnSkillBookEntrySelected.Broadcast(
        SkillID,
        SlotData
    );
}
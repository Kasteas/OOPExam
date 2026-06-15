#include "UI/Skills/NiosCore_SkillBookWidget.h"

#include "AbilitySystemGlobals.h"
#include "Components/PanelWidget.h"
#include "Components/EditableTextBox.h"

#include "UI/Skills/NiosCore_SkillBookEntryWidget.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"

void UNiosCore_SkillBookWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SearchTextBox)
    {
        SearchTextBox->OnTextChanged.AddDynamic(
            this,
            &UNiosCore_SkillBookWidget::HandleSearchTextChanged
        );
    }

    if (!CachedASC)
    {
        CachedASC = GetOwningNiosASC();
    }

    BindASCEvents();
    RefreshSkillBook();
}

void UNiosCore_SkillBookWidget::NativeDestruct()
{
    UnbindASCEvents();

    Super::NativeDestruct();
}

void UNiosCore_SkillBookWidget::InitializeSkillBook(
    UNiosCore_AbilitySystemComponent* InASC)
{
    if (CachedASC == InASC)
    {
        RefreshSkillBook();
        return;
    }

    UnbindASCEvents();

    CachedASC = InASC;

    BindASCEvents();
    RefreshSkillBook();
}

void UNiosCore_SkillBookWidget::BindASCEvents()
{
    if (!CachedASC)
        return;

    CachedASC->OnGrantedSkillsChanged.RemoveDynamic(
        this,
        &UNiosCore_SkillBookWidget::HandleGrantedSkillsChanged
    );

    CachedASC->OnGrantedSkillsChanged.AddDynamic(
        this,
        &UNiosCore_SkillBookWidget::HandleGrantedSkillsChanged
    );
}

void UNiosCore_SkillBookWidget::UnbindASCEvents()
{
    if (!CachedASC)
        return;

    CachedASC->OnGrantedSkillsChanged.RemoveDynamic(
        this,
        &UNiosCore_SkillBookWidget::HandleGrantedSkillsChanged
    );
}

void UNiosCore_SkillBookWidget::HandleGrantedSkillsChanged()
{
    RefreshSkillBook();
}

void UNiosCore_SkillBookWidget::RefreshSkillBook()
{
    CachedGrantedSkillIDs.Empty();

    UNiosCore_AbilitySystemComponent* ASC = GetOwningNiosASC();
    if (!ASC)
    {
        RebuildSkillList();
        return;
    }

    const TArray<FNiosCoreGrantedSkill>& GrantedSkills = ASC->GetGrantedSkills();

    for (const FNiosCoreGrantedSkill& GrantedSkill : GrantedSkills)
    {
        if (GrantedSkill.SkillID.IsNone())
            continue;

        CachedGrantedSkillIDs.AddUnique(GrantedSkill.SkillID);
    }

    RebuildSkillList();
}

void UNiosCore_SkillBookWidget::SetSearchText(const FText& InSearchText)
{
    CurrentSearchText = InSearchText;

    if (SearchTextBox)
    {
        SearchTextBox->SetText(InSearchText);
    }

    RebuildSkillList();
}

void UNiosCore_SkillBookWidget::ClearSearch()
{
    SetSearchText(FText::GetEmpty());
}

void UNiosCore_SkillBookWidget::HandleSearchTextChanged(const FText& InText)
{
    CurrentSearchText = InText;
    RebuildSkillList();
}

void UNiosCore_SkillBookWidget::RebuildSkillList()
{
    CreatedEntryWidgets.Empty();

    if (!SkillListContainer)
        return;

    SkillListContainer->ClearChildren();

    if (!SkillEntryWidgetClass || !SkillCatalog)
        return;

    for (const FName SkillID : CachedGrantedSkillIDs)
    {
        const FNiosCoreSkillCatalogEntry* Entry = SkillCatalog->FindSkill(SkillID);
        if (!Entry)
            continue;

        if (!DoesSkillPassSearch(*Entry))
            continue;

        UNiosCore_SkillBookEntryWidget* EntryWidget =
            CreateWidget<UNiosCore_SkillBookEntryWidget>(
                GetOwningPlayer(),
                SkillEntryWidgetClass
            );

        if (!EntryWidget)
            continue;

        FNiosActionSlotData SlotData;
        SlotData.Type = ENiosActionSlotType::Skill;
        SlotData.ActionID = SkillID;

        FNiosActionSlotVisualData VisualData =
            MakeVisualDataFromCatalogEntry(*Entry);

        EntryWidget->SetSkillEntry(
            SkillID,
            SlotData,
            VisualData
        );

        EntryWidget->SetSelected(SkillID == SelectedSkillID);

        EntryWidget->OnSkillBookEntrySelected.AddDynamic(
            this,
            &UNiosCore_SkillBookWidget::HandleSkillEntrySelected
        );

        SkillListContainer->AddChild(EntryWidget);
        CreatedEntryWidgets.Add(EntryWidget);
    }
}

void UNiosCore_SkillBookWidget::HandleSkillEntrySelected(
    FName SkillID,
    FNiosActionSlotData SlotData)
{
    if (!SlotData.IsSkill())
        return;

    SelectedSkillID = SkillID;

    for (UNiosCore_SkillBookEntryWidget* EntryWidget : CreatedEntryWidgets)
    {
        if (!EntryWidget)
            continue;

        EntryWidget->SetSelected(
            EntryWidget->GetSkillID() == SelectedSkillID
        );
    }

    //      .
}

UNiosCore_AbilitySystemComponent* UNiosCore_SkillBookWidget::GetOwningNiosASC() const
{
    if (CachedASC)
    {
        return CachedASC;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
        return nullptr;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn)
        return nullptr;

    UAbilitySystemComponent* ASC =
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);

    return Cast<UNiosCore_AbilitySystemComponent>(ASC);
}

bool UNiosCore_SkillBookWidget::DoesSkillPassSearch(
    const FNiosCoreSkillCatalogEntry& Entry) const
{
    const FString Search = CurrentSearchText.ToString().TrimStartAndEnd();

    if (Search.IsEmpty())
        return true;

    const FString SkillName = Entry.Name.ToString();

    return SkillName.Contains(Search, ESearchCase::IgnoreCase);
}

FNiosActionSlotVisualData UNiosCore_SkillBookWidget::MakeVisualDataFromCatalogEntry(
    const FNiosCoreSkillCatalogEntry& Entry) const
{
    FNiosActionSlotVisualData VisualData;

    VisualData.Name = Entry.Name;
    VisualData.Description = Entry.Description;
    VisualData.Icon = Entry.Icon;
    VisualData.Count = 0;
    VisualData.bShowCount = false;
    VisualData.bEnabled = true;
    VisualData.bValid = Entry.IsValid();

    return VisualData;
}
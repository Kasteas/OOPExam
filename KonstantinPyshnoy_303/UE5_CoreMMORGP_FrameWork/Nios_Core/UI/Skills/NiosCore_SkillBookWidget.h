#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Nios_CoreStructers.h"
#include "AbilitySystem/Skills/NiosCore_SkillCatalog.h"
#include "NiosCore_SkillBookWidget.generated.h"
#pragma once



class UPanelWidget;
class UEditableTextBox;
class UNiosCore_AbilitySystemComponent;
class UNiosCore_SkillBookEntryWidget;

UCLASS(Blueprintable)
class NIOS_CORE_API UNiosCore_SkillBookWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    void InitializeSkillBook(UNiosCore_AbilitySystemComponent* InASC);

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    void RefreshSkillBook();

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    void SetSearchText(const FText& InSearchText);

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    void ClearSearch();

    UFUNCTION(BlueprintCallable, Category = "Nios|SkillBook")
    void RebuildSkillList();

protected:
    UFUNCTION()
    void HandleSearchTextChanged(const FText& InText);

    UFUNCTION()
    void HandleSkillEntrySelected(FName SkillID, FNiosActionSlotData SlotData);

    UFUNCTION()
    void HandleGrantedSkillsChanged();

    void BindASCEvents();
    void UnbindASCEvents();

    UNiosCore_AbilitySystemComponent* GetOwningNiosASC() const;

    bool DoesSkillPassSearch(const FNiosCoreSkillCatalogEntry& Entry) const;

    FNiosActionSlotVisualData MakeVisualDataFromCatalogEntry(
        const FNiosCoreSkillCatalogEntry& Entry
    ) const;

protected:
    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|SkillBook")
    TObjectPtr<UPanelWidget> SkillListContainer;

    UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "Nios|SkillBook")
    TObjectPtr<UEditableTextBox> SearchTextBox;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|SkillBook")
    TSubclassOf<UNiosCore_SkillBookEntryWidget> SkillEntryWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|SkillBook")
    TObjectPtr<UNiosCore_SkillCatalog> SkillCatalog;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|SkillBook")
    TObjectPtr<UNiosCore_AbilitySystemComponent> CachedASC;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|SkillBook")
    FText CurrentSearchText;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|SkillBook")
    TArray<FName> CachedGrantedSkillIDs;

    UPROPERTY(BlueprintReadOnly, Category = "Nios|SkillBook")
    FName SelectedSkillID = NAME_None;

    UPROPERTY()
    TArray<TObjectPtr<UNiosCore_SkillBookEntryWidget>> CreatedEntryWidgets;
};
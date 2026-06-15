#include "Data/UI/NiosCore_UIDataSubsystem.h"
#include "Data/UI/NiosCore_UIDataAsset.h"

namespace
{
    FText Nios_FallbackName(FName ID)
    {
        return ID.IsNone() ? FText::GetEmpty() : FText::FromName(ID);
    }
}

void UNiosCore_UIDataSubsystem::SetUIDataAsset(UNiosCore_UIDataAsset* InUIDataAsset)
{
    UIDataAsset = InUIDataAsset;
}

UNiosCore_UIDataAsset* UNiosCore_UIDataSubsystem::GetUIDataAsset() const
{
    return UIDataAsset;
}

FText UNiosCore_UIDataSubsystem::FindNamedDisplayName(const TArray<FNiosNamedDisplayData>& Entries, FName ID) const
{
    for (const FNiosNamedDisplayData& Entry : Entries)
    {
        if (Entry.ID == ID)
        {
            return Entry.DisplayName.IsEmpty() ? Nios_FallbackName(ID) : Entry.DisplayName;
        }
    }

    return Nios_FallbackName(ID);
}

FText UNiosCore_UIDataSubsystem::FindNamedDescription(const TArray<FNiosNamedDisplayData>& Entries, FName ID) const
{
    for (const FNiosNamedDisplayData& Entry : Entries)
    {
        if (Entry.ID == ID)
        {
            return Entry.Description;
        }
    }

    return FText::GetEmpty();
}

FText UNiosCore_UIDataSubsystem::GetStatDisplayName(ENiosStatType StatType) const
{
    if (UIDataAsset)
    {
        for (const FNiosStatDisplayData& Entry : UIDataAsset->StatDisplayData)
        {
            if (Entry.StatType == StatType)
            {
                return Entry.DisplayName.IsEmpty()
                    ? (StaticEnum<ENiosStatType>()
                        ? StaticEnum<ENiosStatType>()->GetDisplayNameTextByValue(static_cast<int64>(StatType))
                        : FText::FromString(TEXT("Unknown")))
                    : Entry.DisplayName;
            }
        }
    }

    return StaticEnum<ENiosStatType>()
        ? StaticEnum<ENiosStatType>()->GetDisplayNameTextByValue(static_cast<int64>(StatType))
        : FText::FromString(TEXT("Unknown"));
}

FText UNiosCore_UIDataSubsystem::GetStatDescription(ENiosStatType StatType) const
{
    if (UIDataAsset)
    {
        for (const FNiosStatDisplayData& Entry : UIDataAsset->StatDisplayData)
        {
            if (Entry.StatType == StatType)
            {
                return Entry.Description;
            }
        }
    }

    return FText::GetEmpty();
}

FLinearColor UNiosCore_UIDataSubsystem::GetStatColor(ENiosStatType StatType) const
{
    if (UIDataAsset)
    {
        for (const FNiosStatDisplayData& Entry : UIDataAsset->StatDisplayData)
        {
            if (Entry.StatType == StatType)
            {
                return Entry.Color;
            }
        }
    }

    return FLinearColor::White;
}

FText UNiosCore_UIDataSubsystem::GetItemTypeDisplayName(FName ItemTypeID) const
{
    return UIDataAsset ? FindNamedDisplayName(UIDataAsset->ItemTypeDisplayData, ItemTypeID) : Nios_FallbackName(ItemTypeID);
}

FText UNiosCore_UIDataSubsystem::GetWeaponTypeDisplayName(FName WeaponTypeID) const
{
    return UIDataAsset ? FindNamedDisplayName(UIDataAsset->WeaponTypeDisplayData, WeaponTypeID) : Nios_FallbackName(WeaponTypeID);
}

FText UNiosCore_UIDataSubsystem::GetArmorTypeDisplayName(FName ArmorTypeID) const
{
    return UIDataAsset ? FindNamedDisplayName(UIDataAsset->ArmorTypeDisplayData, ArmorTypeID) : Nios_FallbackName(ArmorTypeID);
}

FText UNiosCore_UIDataSubsystem::GetArmorSlotDisplayName(FName ArmorSlotID) const
{
    return UIDataAsset ? FindNamedDisplayName(UIDataAsset->ArmorSlotDisplayData, ArmorSlotID) : Nios_FallbackName(ArmorSlotID);
}

FText UNiosCore_UIDataSubsystem::GetTooltipLabel(FName LabelID) const
{
    return UIDataAsset ? FindNamedDisplayName(UIDataAsset->TooltipLabelDisplayData, LabelID) : Nios_FallbackName(LabelID);
}

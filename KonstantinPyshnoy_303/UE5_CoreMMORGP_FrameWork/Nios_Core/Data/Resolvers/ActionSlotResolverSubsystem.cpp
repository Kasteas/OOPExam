#include "Data/Resolvers/ActionSlotResolverSubsystem.h"

#include "Data/Config/NiosCore_DatabaseSettings.h"
#include "AbilitySystem/Skills/NiosCore_SkillCatalog.h"
#include "Data/StatusEffects/NiosCore_StatusEffectCatalog.h"
#include "Data/StatusEffects/NiosCore_StatusEffectDataAsset.h"
#include "Data/StatusEffects/NiosCore_CrowdControlPresentationCatalog.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "UObject/EnumProperty.h"
#include "UObject/UnrealType.h"
#include "Data/UI/NiosCore_UIDataSubsystem.h"

void UActionSlotResolverSubsystem::SetDatabaseSettings(
    UNiosCore_DatabaseSettings* InDatabaseSettings)
{
    DatabaseSettings = InDatabaseSettings;

    CachedSkillCatalog = nullptr;
    CachedStatusEffectCatalog = nullptr;
    CachedCrowdControlPresentationCatalog = nullptr;
    LoadedIconCache.Empty();
}

bool UActionSlotResolverSubsystem::ResolveVisualData(
    const FNiosActionSlotData& SlotData,
    FNiosActionSlotVisualData& OutVisualData)
{
    OutVisualData = FNiosActionSlotVisualData();

    if (SlotData.IsEmpty())
    {
        return false;
    }

    switch (SlotData.Type)
    {
    case ENiosActionSlotType::Skill:
        return ResolveSkillVisualData(SlotData, OutVisualData);

    case ENiosActionSlotType::Item:
        return ResolveItemVisualData(SlotData, OutVisualData);

    case ENiosActionSlotType::Emote:
        return false;

    default:
        return false;
    }
}

bool UActionSlotResolverSubsystem::UIsItemUsable(const FNiosActionSlotData& SlotData) 
{
    FNiosResolvedInventoryItemData ItemData;

    if (!ResolveInventoryItemData(SlotData.ActionID, ItemData))
        return false;

    // ��������� ��� ��� ItemType
    return ItemData.ItemType == ENiosItemType::Usable;
}

bool UActionSlotResolverSubsystem::ResolveInventoryItemData(
    FName ItemID,
    FNiosResolvedInventoryItemData& OutItemData)
{
    OutItemData = FNiosResolvedInventoryItemData();

    if (ItemID.IsNone())
    {
        return false;
    }

    UDataTable* ItemsTable = GetItemsTable();

    if (!ItemsTable)
    {
        return false;
    }

    const FNiosItemBaseRow* BaseRow =
        ItemsTable->FindRow<FNiosItemBaseRow>(
            ItemID,
            TEXT("ResolveInventoryItemData")
        );

    if (!BaseRow)
    {
        return false;
    }

    OutItemData.bValid = true;
    OutItemData.ItemID = ItemID;
    OutItemData.ItemType = BaseRow->ItemType;
    OutItemData.bStackable = BaseRow->bStackable;
    OutItemData.MaxStack = BaseRow->bStackable
        ? FMath::Max(1, BaseRow->MaxStack)
        : 1;

    if (BaseRow->ItemType == ENiosItemType::Usable)
    {
        UDataTable* UsableTable = GetUsableTable();

        if (UsableTable)
        {
            const FNiosUsableRow* UsableRow =
                UsableTable->FindRow<FNiosUsableRow>(
                    ItemID,
                    TEXT("ResolveInventoryItemData Usable")
                );

            if (UsableRow)
            {
                OutItemData.SkillID = UsableRow->SkillID;
                OutItemData.AbilityData = UsableRow->AbilityData;
                OutItemData.bConsumeOnUse = UsableRow->bConsumeOnUse;
                OutItemData.ConsumeCount = FMath::Max(1, UsableRow->ConsumeCount);
                OutItemData.UseCooldown = UsableRow->UseCooldown;
                OutItemData.bUseGlobalCooldown = UsableRow->bUseGlobalCooldown;
                OutItemData.GlobalCooldownOverride = UsableRow->GlobalCooldownOverride;
            }
        }
    }

    return true;
}

bool UActionSlotResolverSubsystem::ResolveItemVisualData(
    const FNiosActionSlotData& SlotData,
    FNiosActionSlotVisualData& OutVisualData)
{
    if (SlotData.ActionID.IsNone())
    {
        return false;
    }

    UDataTable* ItemsTable = GetItemsTable();

    if (!ItemsTable)
    {
        return false;
    }

    const FNiosItemBaseRow* BaseRow =
        ItemsTable->FindRow<FNiosItemBaseRow>(
            SlotData.ActionID,
            TEXT("ResolveItemVisualData")
        );

    if (!BaseRow)
    {
        return false;
    }

    FNiosResolvedInventoryItemData ItemData;

    if (!ResolveInventoryItemData(SlotData.ActionID, ItemData))
    {
        return false;
    }

    OutVisualData.Name = BaseRow->Name;
    OutVisualData.Description = BaseRow->Description;
    OutVisualData.Icon = BaseRow->Icon;
    OutVisualData.Count = SlotData.Count;
    OutVisualData.bShowCount = ItemData.bStackable;
    OutVisualData.bEnabled = true;
    OutVisualData.bValid = true;

    return true;
}

bool UActionSlotResolverSubsystem::ResolveSkillVisualData(
    const FNiosActionSlotData& SlotData,
    FNiosActionSlotVisualData& OutVisualData)
{
    UNiosCore_SkillCatalog* SkillCatalog = GetSkillCatalog();

    if (!SkillCatalog)
    {
        return false;
    }

    const FNiosCoreSkillCatalogEntry* Entry =
        SkillCatalog->FindSkill(SlotData.ActionID);

    if (!Entry)
    {
        return false;
    }

    OutVisualData.Name = Entry->Name;
    OutVisualData.Description = Entry->Description;
    OutVisualData.Icon = Entry->Icon;
    OutVisualData.Count = 0;
    OutVisualData.bShowCount = false;
    OutVisualData.bEnabled = true;
    OutVisualData.bValid = true;

    return true;
}

UNiosCore_SkillCatalog* UActionSlotResolverSubsystem::GetSkillCatalog()
{
    if (CachedSkillCatalog)
    {
        return CachedSkillCatalog;
    }

    if (!DatabaseSettings || DatabaseSettings->SkillCatalog.IsNull())
    {
        return nullptr;
    }

    CachedSkillCatalog = DatabaseSettings->SkillCatalog.LoadSynchronous();

    return CachedSkillCatalog;
}


UNiosCore_StatusEffectCatalog* UActionSlotResolverSubsystem::GetStatusEffectCatalog()
{
    if (CachedStatusEffectCatalog)
    {
        return CachedStatusEffectCatalog;
    }

    if (!DatabaseSettings || DatabaseSettings->StatusEffectCatalog.IsNull())
    {
        return nullptr;
    }

    CachedStatusEffectCatalog = DatabaseSettings->StatusEffectCatalog.LoadSynchronous();
    return CachedStatusEffectCatalog;
}

UNiosCore_CrowdControlPresentationCatalog* UActionSlotResolverSubsystem::GetCrowdControlPresentationCatalog()
{
    if (CachedCrowdControlPresentationCatalog)
    {
        return CachedCrowdControlPresentationCatalog;
    }

    if (!DatabaseSettings || DatabaseSettings->CrowdControlPresentationCatalog.IsNull())
    {
        return nullptr;
    }

    CachedCrowdControlPresentationCatalog = DatabaseSettings->CrowdControlPresentationCatalog.LoadSynchronous();
    return CachedCrowdControlPresentationCatalog;
}

UDataTable* UActionSlotResolverSubsystem::GetItemsTable() const
{
    if (!DatabaseSettings || DatabaseSettings->DT_Items.IsNull())
    {
        return nullptr;
    }

    return DatabaseSettings->DT_Items.LoadSynchronous();
}

UDataTable* UActionSlotResolverSubsystem::GetUsableTable() const
{
    if (!DatabaseSettings || DatabaseSettings->DT_Usable.IsNull())
    {
        return nullptr;
    }

    return DatabaseSettings->DT_Usable.LoadSynchronous();
}

UDataTable* UActionSlotResolverSubsystem::GetWeaponTable() const
{
    if (!DatabaseSettings || DatabaseSettings->DT_Weapon.IsNull())
    {
        return nullptr;
    }

    return DatabaseSettings->DT_Weapon.LoadSynchronous();
}

UDataTable* UActionSlotResolverSubsystem::GetArmorTable() const
{
    if (!DatabaseSettings || DatabaseSettings->DT_Armor.IsNull())
    {
        return nullptr;
    }

    return DatabaseSettings->DT_Armor.LoadSynchronous();
}

UTexture2D* UActionSlotResolverSubsystem::LoadIconCached(
    TSoftObjectPtr<UTexture2D> Icon)
{
    if (Icon.IsNull())
    {
        return nullptr;
    }

    const FSoftObjectPath Path = Icon.ToSoftObjectPath();

    if (TObjectPtr<UTexture2D>* CachedIcon = LoadedIconCache.Find(Path))
    {
        return CachedIcon->Get();
    }

    UTexture2D* LoadedIcon = Icon.LoadSynchronous();

    if (LoadedIcon)
    {
        LoadedIconCache.Add(Path, LoadedIcon);
    }

    return LoadedIcon;
}

void UActionSlotResolverSubsystem::ClearCache()
{
    CachedSkillCatalog = nullptr;
    CachedStatusEffectCatalog = nullptr;
    CachedCrowdControlPresentationCatalog = nullptr;
    LoadedIconCache.Empty();
}

namespace
{
    FName Nios_EnumNameByValue(const UEnum* Enum, int64 Value);
    FText Nios_FallbackEnumDisplayName(const UEnum* Enum, int64 Value);
}

UNiosCore_UIDataSubsystem* UActionSlotResolverSubsystem::GetUIDataSubsystem() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    return GameInstance ? GameInstance->GetSubsystem<UNiosCore_UIDataSubsystem>() : nullptr;
}

FText UActionSlotResolverSubsystem::GetTooltipLabel(FName LabelID) const
{
    if (UNiosCore_UIDataSubsystem* UIData = GetUIDataSubsystem())
    {
        return UIData->GetTooltipLabel(LabelID);
    }

    return FText::FromName(LabelID);
}

FText UActionSlotResolverSubsystem::GetTooltipLabelWithFallback(FName LabelID, const FText& FallbackText) const
{
    const FText ResolvedText = GetTooltipLabel(LabelID);

    // If UI data has no entry, GetTooltipLabel usually falls back to the raw key.
    // In that case, use a readable fallback so production UI does not show internal ids.
    if (ResolvedText.IsEmpty() || ResolvedText.ToString().Equals(LabelID.ToString(), ESearchCase::IgnoreCase))
    {
        return FallbackText;
    }

    return ResolvedText;
}

FText UActionSlotResolverSubsystem::GetStatDisplayName(ENiosStatType StatType) const
{
    if (UNiosCore_UIDataSubsystem* UIData = GetUIDataSubsystem())
    {
        return UIData->GetStatDisplayName(StatType);
    }

    const UEnum* StatEnum = StaticEnum<ENiosStatType>();
    return StatEnum
        ? StatEnum->GetDisplayNameTextByValue(static_cast<int64>(StatType))
        : FText::FromString(TEXT("Unknown"));
}

FText UActionSlotResolverSubsystem::GetGameplayAttributeDisplayName(const FGameplayAttribute& Attribute) const
{
    if (!Attribute.IsValid())
    {
        return FText::GetEmpty();
    }

    const FString AttributeName = Attribute.GetName();

    struct FNiosAttributeToStatMapping
    {
        const TCHAR* AttributeName;
        ENiosStatType StatType;
    };

    static const FNiosAttributeToStatMapping Mappings[] =
    {
        { TEXT("MaxHealth"), ENiosStatType::MaxHealth },
        { TEXT("Health"), ENiosStatType::Health },
        { TEXT("MaxMana"), ENiosStatType::MaxMana },
        { TEXT("Mana"), ENiosStatType::Mana },
        { TEXT("MaxEnergy"), ENiosStatType::MaxEnergy },
        { TEXT("Energy"), ENiosStatType::Energy },
        { TEXT("MaxRage"), ENiosStatType::MaxRage },
        { TEXT("Rage"), ENiosStatType::Rage },

        { TEXT("Strength"), ENiosStatType::Strength },
        { TEXT("Agility"), ENiosStatType::Agility },
        { TEXT("Intelligence"), ENiosStatType::Intelligence },
        { TEXT("Stamina"), ENiosStatType::Stamina },
        { TEXT("Spirit"), ENiosStatType::Spirit },
        { TEXT("Constitution"), ENiosStatType::Constitution },

        { TEXT("PhysicalCrit"), ENiosStatType::PhysicalCrit },
        { TEXT("MagicCrit"), ENiosStatType::MagicCrit },
        { TEXT("PhysicalDefense"), ENiosStatType::PhysicalDefense },
        { TEXT("MagicDefense"), ENiosStatType::MagicDefense },
        { TEXT("PhysicalModifier"), ENiosStatType::PhysicalModifier },
        { TEXT("MagicModifier"), ENiosStatType::MagicModifier },

        { TEXT("PlayerLevel"), ENiosStatType::PlayerLevel },
        { TEXT("PlayerXP"), ENiosStatType::PlayerXP }
    };

    for (const FNiosAttributeToStatMapping& Mapping : Mappings)
    {
        if (AttributeName.Equals(Mapping.AttributeName, ESearchCase::IgnoreCase) ||
            AttributeName.EndsWith(FString::Printf(TEXT(".%s"), Mapping.AttributeName), ESearchCase::IgnoreCase))
        {
            return GetStatDisplayName(Mapping.StatType);
        }
    }

    FString CleanName = AttributeName;
    int32 DotIndex = INDEX_NONE;
    if (CleanName.FindLastChar(TEXT('.'), DotIndex))
    {
        CleanName = CleanName.RightChop(DotIndex + 1);
    }

    return FText::FromString(CleanName);
}

FText UActionSlotResolverSubsystem::GetItemTypeDisplayName(ENiosItemType ItemType) const
{
    const UEnum* Enum = StaticEnum<ENiosItemType>();
    const FName ID = Nios_EnumNameByValue(Enum, static_cast<int64>(ItemType));

    if (UNiosCore_UIDataSubsystem* UIData = GetUIDataSubsystem())
    {
        return UIData->GetItemTypeDisplayName(ID);
    }

    return Nios_FallbackEnumDisplayName(Enum, static_cast<int64>(ItemType));
}

FText UActionSlotResolverSubsystem::GetWeaponTypeDisplayName(ENiosWeaponType WeaponType) const
{
    const UEnum* Enum = StaticEnum<ENiosWeaponType>();
    const FName ID = Nios_EnumNameByValue(Enum, static_cast<int64>(WeaponType));

    if (UNiosCore_UIDataSubsystem* UIData = GetUIDataSubsystem())
    {
        return UIData->GetWeaponTypeDisplayName(ID);
    }

    return Nios_FallbackEnumDisplayName(Enum, static_cast<int64>(WeaponType));
}

FText UActionSlotResolverSubsystem::GetArmorTypeDisplayName(ENiosArmorType ArmorType) const
{
    const UEnum* Enum = StaticEnum<ENiosArmorType>();
    const FName ID = Nios_EnumNameByValue(Enum, static_cast<int64>(ArmorType));

    if (UNiosCore_UIDataSubsystem* UIData = GetUIDataSubsystem())
    {
        return UIData->GetArmorTypeDisplayName(ID);
    }

    return Nios_FallbackEnumDisplayName(Enum, static_cast<int64>(ArmorType));
}

FText UActionSlotResolverSubsystem::GetArmorSlotDisplayName(ENiosArmorSlot ArmorSlot) const
{
    const UEnum* Enum = StaticEnum<ENiosArmorSlot>();
    const FName ID = Nios_EnumNameByValue(Enum, static_cast<int64>(ArmorSlot));

    if (UNiosCore_UIDataSubsystem* UIData = GetUIDataSubsystem())
    {
        return UIData->GetArmorSlotDisplayName(ID);
    }

    return Nios_FallbackEnumDisplayName(Enum, static_cast<int64>(ArmorSlot));
}

namespace
{
    FNiosTooltipLine Nios_MakeTooltipLine(const FText& Label, const FText& Value, ENiosTooltipLineStyle Style = ENiosTooltipLineStyle::Normal)
    {
        FNiosTooltipLine Line;
        Line.Label = Label;
        Line.Value = Value;
        Line.Style = Style;
        return Line;
    }

    FText Nios_FloatText(float Value, int32 FractionalDigits = 0)
    {
        FNumberFormattingOptions Options;
        Options.MinimumFractionalDigits = FractionalDigits;
        Options.MaximumFractionalDigits = FractionalDigits;
        return FText::AsNumber(Value, &Options);
    }


    FText Nios_AttributeNameText(const FGameplayAttribute& Attribute)
    {
        if (!Attribute.IsValid())
        {
            return FText::GetEmpty();
        }

        return FText::FromString(Attribute.GetName());
    }

    FText Nios_SignedFloatText(float Value, int32 FractionalDigits = 0)
    {
        const FText AbsText = Nios_FloatText(FMath::Abs(Value), FractionalDigits);
        return FText::Format(
            FText::FromString(Value >= 0.f ? TEXT("+{0}") : TEXT("-{0}")),
            AbsText
        );
    }

    FName Nios_EnumNameByValue(const UEnum* Enum, int64 Value)
    {
        if (!Enum)
        {
            return NAME_None;
        }

        return FName(*Enum->GetNameStringByValue(Value));
    }

    FText Nios_FallbackEnumDisplayName(const UEnum* Enum, int64 Value)
    {
        return Enum ? Enum->GetDisplayNameTextByValue(Value) : FText::AsNumber(Value);
    }
}

bool UActionSlotResolverSubsystem::ResolveTooltipData(const FNiosActionSlotData& SlotData, FNiosTooltipData& OutTooltipData)
{
    OutTooltipData.Reset();

    if (SlotData.IsEmpty())
    {
        return false;
    }

    switch (SlotData.Type)
    {
    case ENiosActionSlotType::Item:
        return ResolveItemTooltipData(SlotData.ActionID, SlotData.Count, OutTooltipData);

    case ENiosActionSlotType::Skill:
        return ResolveSkillTooltipData(SlotData.ActionID, OutTooltipData);

    default:
        return false;
    }
}

bool UActionSlotResolverSubsystem::ResolveItemTooltipData(FName ItemID, int32 Count, FNiosTooltipData& OutTooltipData)
{
    OutTooltipData.Reset();

    if (ItemID.IsNone())
    {
        return false;
    }

    UDataTable* ItemsTable = GetItemsTable();
    if (!ItemsTable)
    {
        return false;
    }

    const FNiosItemBaseRow* BaseRow = ItemsTable->FindRow<FNiosItemBaseRow>(ItemID, TEXT("ResolveItemTooltipData Base"));
    if (!BaseRow)
    {
        return false;
    }

    FNiosResolvedInventoryItemData ItemData;
    ResolveInventoryItemData(ItemID, ItemData);

    OutTooltipData.bValid = true;
    OutTooltipData.SourceID = ItemID;
    OutTooltipData.Title = BaseRow->Name;
    OutTooltipData.Description = BaseRow->Description;
    OutTooltipData.Icon = BaseRow->Icon;
    OutTooltipData.Count = Count;
    OutTooltipData.bShowCount = BaseRow->bStackable;

    switch (BaseRow->ItemType)
    {
    case ENiosItemType::Weapon:
    {
        OutTooltipData.Type = ENiosTooltipType::Weapon;
        OutTooltipData.Subtitle = GetItemTypeDisplayName(ENiosItemType::Weapon);

        if (UDataTable* WeaponTable = GetWeaponTable())
        {
            if (const FNiosWeaponRow* WeaponRow = WeaponTable->FindRow<FNiosWeaponRow>(ItemID, TEXT("ResolveItemTooltipData Weapon")))
            {
                OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Type")), GetWeaponTypeDisplayName(WeaponRow->WeaponType)));
                OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Damage")), Nios_FloatText(WeaponRow->Damage, 0)));
                OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(
                    GetTooltipLabelWithFallback(TEXT("AttackSpeedTooltip"), FText::FromString(TEXT("Скорость атаки"))),
                    Nios_FloatText(WeaponRow->AttackSpeed, 2)
                ));

                if (WeaponRow->StatModifiers.Num() > 0)
                {
                    FNiosTooltipSection StatsSection;
                    StatsSection.Header = GetTooltipLabel(TEXT("Bonuses"));

                    for (const FNiosStatModifier& Modifier : WeaponRow->StatModifiers)
                    {
                        const bool bPositive = Modifier.Value >= 0.f;
                        StatsSection.Lines.Add(
                            Nios_MakeTooltipLine(
                                GetStatDisplayName(Modifier.StatType),
                                FText::Format(
                                    FText::FromString(bPositive ? TEXT("+{0}") : TEXT("{0}")),
                                    Nios_FloatText(Modifier.Value, 0)
                                ),
                                bPositive ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative
                            )
                        );
                    }

                    if (StatsSection.Lines.Num() > 0)
                    {
                        OutTooltipData.Sections.Add(StatsSection);
                    }
                }


                if (WeaponRow->GrantedStatusEffectIDs.Num() > 0)
                {
                    FNiosTooltipSection StatusSection;
                    StatusSection.Header = GetTooltipLabelWithFallback(TEXT("GrantedStatusEffects"), FText::FromString(TEXT("Постоянные эффекты")));

                    UNiosCore_StatusEffectCatalog* StatusCatalog = GetStatusEffectCatalog();
                    for (const FName EffectID : WeaponRow->GrantedStatusEffectIDs)
                    {
                        const FNiosStatusEffectCatalogEntry* Entry = StatusCatalog ? StatusCatalog->FindStatusEffect(EffectID) : nullptr;
                        const FText EffectName = Entry && !Entry->Name.IsEmpty() ? Entry->Name : FText::FromName(EffectID);
                        StatusSection.Lines.Add(Nios_MakeTooltipLine(GetTooltipLabelWithFallback(TEXT("StatusEffect"), FText::FromString(TEXT("Эффект"))), EffectName, Entry ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative));
                    }

                    if (StatusSection.Lines.Num() > 0)
                    {
                        OutTooltipData.Sections.Add(StatusSection);
                    }
                }
            }
        }
        break;
    }

    case ENiosItemType::Armor:
    {
        OutTooltipData.Type = ENiosTooltipType::Armor;
        OutTooltipData.Subtitle = GetItemTypeDisplayName(ENiosItemType::Armor);

        if (UDataTable* ArmorTable = GetArmorTable())
        {
            if (const FNiosArmorRow* ArmorRow = ArmorTable->FindRow<FNiosArmorRow>(ItemID, TEXT("ResolveItemTooltipData Armor")))
            {
                OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Type")), GetArmorTypeDisplayName(ArmorRow->ArmorType)));
                OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Slot")), GetArmorSlotDisplayName(ArmorRow->ArmorSlot)));
                OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabelWithFallback(TEXT("ArmorValue"), FText::FromString(TEXT("Броня"))), Nios_FloatText(ArmorRow->Armor, 0)));

                if (ArmorRow->StatModifiers.Num() > 0)
                {
                    FNiosTooltipSection StatsSection;
                    StatsSection.Header = GetTooltipLabel(TEXT("Bonuses"));

                    for (const FNiosStatModifier& Modifier : ArmorRow->StatModifiers)
                    {
                        const bool bPositive = Modifier.Value >= 0.f;
                        StatsSection.Lines.Add(
                            Nios_MakeTooltipLine(
                                GetStatDisplayName(Modifier.StatType),
                                FText::Format(
                                    FText::FromString(bPositive ? TEXT("+{0}") : TEXT("{0}")),
                                    Nios_FloatText(Modifier.Value, 0)
                                ),
                                bPositive ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative
                            )
                        );
                    }

                    if (StatsSection.Lines.Num() > 0)
                    {
                        OutTooltipData.Sections.Add(StatsSection);
                    }
                }


                if (ArmorRow->GrantedStatusEffectIDs.Num() > 0)
                {
                    FNiosTooltipSection StatusSection;
                    StatusSection.Header = GetTooltipLabelWithFallback(TEXT("GrantedStatusEffects"), FText::FromString(TEXT("Постоянные эффекты")));

                    UNiosCore_StatusEffectCatalog* StatusCatalog = GetStatusEffectCatalog();
                    for (const FName EffectID : ArmorRow->GrantedStatusEffectIDs)
                    {
                        const FNiosStatusEffectCatalogEntry* Entry = StatusCatalog ? StatusCatalog->FindStatusEffect(EffectID) : nullptr;
                        const FText EffectName = Entry && !Entry->Name.IsEmpty() ? Entry->Name : FText::FromName(EffectID);
                        StatusSection.Lines.Add(Nios_MakeTooltipLine(GetTooltipLabelWithFallback(TEXT("StatusEffect"), FText::FromString(TEXT("Эффект"))), EffectName, Entry ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative));
                    }

                    if (StatusSection.Lines.Num() > 0)
                    {
                        OutTooltipData.Sections.Add(StatusSection);
                    }
                }
            }
        }
        break;
    }

    case ENiosItemType::Usable:
    {
        OutTooltipData.Type = ENiosTooltipType::Item;
        OutTooltipData.Subtitle = GetItemTypeDisplayName(ENiosItemType::Usable);

        if (ItemData.SkillID != NAME_None)
        {
            OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Skill")), FText::FromName(ItemData.SkillID)));
        }
        if (ItemData.UseCooldown > 0.f)
        {
            OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Cooldown")), FText::Format(FText::FromString(TEXT("{0}s")), Nios_FloatText(ItemData.UseCooldown, 1))));
        }
        if (ItemData.bUseGlobalCooldown)
        {
            const float GlobalValue = ItemData.GlobalCooldownOverride;
            const FText GlobalText = GlobalValue > 0.f
                ? FText::Format(FText::FromString(TEXT("{0}s")), Nios_FloatText(GlobalValue, 1))
                : GetTooltipLabelWithFallback(TEXT("Default"), FText::FromString(TEXT("По умолчанию")));
            OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabelWithFallback(TEXT("GlobalCooldown"), FText::FromString(TEXT("Глобальный CD"))), GlobalText));
        }
        if (ItemData.bConsumeOnUse)
        {
            OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Consumes")), FText::AsNumber(ItemData.ConsumeCount)));
        }
        break;
    }

    case ENiosItemType::Misc:
        OutTooltipData.Type = ENiosTooltipType::Item;
        OutTooltipData.Subtitle = GetItemTypeDisplayName(ENiosItemType::Misc);
        break;

    default:
        OutTooltipData.Type = ENiosTooltipType::Item;
        OutTooltipData.Subtitle = GetItemTypeDisplayName(ENiosItemType::Misc);
        break;
    }

    if (BaseRow->Price > 0)
    {
        OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Price")), FText::AsNumber(BaseRow->Price)));
    }

    return true;
}


bool UActionSlotResolverSubsystem::ResolveSkillTooltipData(FName SkillID, FNiosTooltipData& OutTooltipData)
{
    OutTooltipData.Reset();

    if (SkillID.IsNone())
    {
        return false;
    }

    FNiosActionSlotData TempSlot;
    TempSlot.Type = ENiosActionSlotType::Skill;
    TempSlot.ActionID = SkillID;

    FNiosActionSlotVisualData VisualData;
    if (!ResolveSkillVisualData(TempSlot, VisualData))
    {
        return false;
    }

    OutTooltipData.bValid = true;
    OutTooltipData.Type = ENiosTooltipType::Skill;
    OutTooltipData.SourceID = SkillID;
    OutTooltipData.Title = VisualData.Name;
    OutTooltipData.Subtitle = GetTooltipLabel(TEXT("Skill"));
    OutTooltipData.Description = VisualData.Description;
    OutTooltipData.Icon = VisualData.Icon;

    UNiosCore_SkillDataAsset* SkillData = nullptr;
    if (UNiosCore_SkillCatalog* SkillCatalog = GetSkillCatalog())
    {
        if (const FNiosCoreSkillCatalogEntry* Entry = SkillCatalog->FindSkill(SkillID))
        {
            SkillData = Entry->SkillData;
        }
    }

    if (!SkillData)
    {
        return true;
    }

    auto SecondsText = [](float Seconds) -> FText
    {
        return FText::Format(FText::FromString(TEXT("{0}s")), Nios_FloatText(Seconds, 1));
    };

    auto JoinTextsWithComma = [](const TArray<FText>& Texts) -> FText
    {
        TArray<FString> Parts;
        Parts.Reserve(Texts.Num());

        for (const FText& Text : Texts)
        {
            const FString Part = Text.ToString();
            if (!Part.IsEmpty())
            {
                Parts.Add(Part);
            }
        }

        return Parts.Num() > 0
            ? FText::FromString(FString::Join(Parts, TEXT(", ")))
            : FText::GetEmpty();
    };


    auto AddSignedAttributeLine = [this](FNiosTooltipSection& Section, const FText& Label, const FGameplayAttribute& Attribute, float Value)
    {
        if (!Attribute.IsValid() || FMath::IsNearlyZero(Value)) { return; }

        Section.Lines.Add(Nios_MakeTooltipLine(
            Label,
            FText::Format(FText::FromString(TEXT("{0} {1}")), GetGameplayAttributeDisplayName(Attribute), Nios_SignedFloatText(Value, 0)),
            Value >= 0.f ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative));
    };

    auto ResolveEffectiveTargetPolicy = [SkillData](const FNiosSkillEffectDefinition& Effect, const UNiosCore_StatusEffectDataAsset* StatusData) -> ENiosCoreTargetApplicationPolicy
    {
        if (Effect.TargetPolicy != ENiosCoreTargetApplicationPolicy::AutoFromEffectType)
        {
            return Effect.TargetPolicy;
        }

        if (StatusData && StatusData->TargetPolicy != ENiosCoreTargetApplicationPolicy::AutoFromEffectType)
        {
            return StatusData->TargetPolicy;
        }

        switch (Effect.EffectType)
        {
        case ENiosSkillEffectType::Damage:
            return ENiosCoreTargetApplicationPolicy::EnemiesOnly;
        case ENiosSkillEffectType::Heal:
            return ENiosCoreTargetApplicationPolicy::FriendliesOnly;
        default:
            break;
        }

        if (SkillData)
        {
            switch (SkillData->AffectType)
            {
            case ENiosAbilityAffectType::Harm:
                return ENiosCoreTargetApplicationPolicy::EnemiesOnly;
            case ENiosAbilityAffectType::Help:
                return ENiosCoreTargetApplicationPolicy::FriendliesOnly;
            case ENiosAbilityAffectType::Neutral:
            default:
                return ENiosCoreTargetApplicationPolicy::SourceOnly;
            }
        }

        return ENiosCoreTargetApplicationPolicy::SourceOnly;
    };

    auto TargetPolicyObjectText = [](ENiosCoreTargetApplicationPolicy Policy) -> FText
    {
        switch (Policy)
        {
        case ENiosCoreTargetApplicationPolicy::EnemiesOnly:
            return FText::FromString(TEXT("противников"));
        case ENiosCoreTargetApplicationPolicy::FriendliesOnly:
            return FText::FromString(TEXT("союзников"));
        case ENiosCoreTargetApplicationPolicy::SourceOnly:
            return FText::FromString(TEXT("себя"));
        case ENiosCoreTargetApplicationPolicy::NonNeutralOnly:
            return FText::FromString(TEXT("цели"));
        case ENiosCoreTargetApplicationPolicy::AnyIncludingNeutral:
            return FText::FromString(TEXT("все цели"));
        case ENiosCoreTargetApplicationPolicy::AutoFromEffectType:
        default:
            return FText::FromString(TEXT("цель"));
        }
    };

    auto TargetPolicyLabelSuffix = [](ENiosCoreTargetApplicationPolicy Policy) -> FText
    {
        switch (Policy)
        {
        case ENiosCoreTargetApplicationPolicy::EnemiesOnly:
            return FText::FromString(TEXT("противникам"));
        case ENiosCoreTargetApplicationPolicy::FriendliesOnly:
            return FText::FromString(TEXT("союзникам"));
        case ENiosCoreTargetApplicationPolicy::SourceOnly:
            return FText::FromString(TEXT("себе"));
        case ENiosCoreTargetApplicationPolicy::NonNeutralOnly:
            return FText::FromString(TEXT("целям"));
        case ENiosCoreTargetApplicationPolicy::AnyIncludingNeutral:
            return FText::FromString(TEXT("всем целям"));
        case ENiosCoreTargetApplicationPolicy::AutoFromEffectType:
        default:
            return FText::FromString(TEXT("цели"));
        }
    };

    auto AddStatusEffectShortSummary = [this, &SecondsText](FNiosTooltipSection& Section, const FNiosStatusEffectCatalogEntry* StatusEntry, const UNiosCore_StatusEffectDataAsset* StatusData, bool bRemove, const FText& TargetObjectText)
    {
        const FText EffectName = StatusEntry && !StatusEntry->Name.IsEmpty()
            ? StatusEntry->Name
            : FText::FromString(TEXT("Статус-эффект"));

        FText ValueText = EffectName;
        if (StatusData)
        {
            if (StatusData->DurationPolicy == ENiosStatusEffectDurationPolicy::Duration)
            {
                ValueText = FText::Format(FText::FromString(TEXT("{0}, {1}")), EffectName, SecondsText(StatusData->Duration));
            }
            else if (StatusData->DurationPolicy == ENiosStatusEffectDurationPolicy::Infinite)
            {
                ValueText = FText::Format(FText::FromString(TEXT("{0}, постоянно")), EffectName);
            }
        }

        Section.Lines.Add(Nios_MakeTooltipLine(
            FText::Format(FText::FromString(bRemove ? TEXT("Снимает с {0}") : TEXT("Накладывает на {0}")), TargetObjectText),
            ValueText,
            bRemove ? ENiosTooltipLineStyle::Negative : ENiosTooltipLineStyle::Positive));

        if (!StatusData)
        {
            return;
        }

        int32 AddedOperationLines = 0;
        for (const FNiosStatusEffectOperation& Operation : StatusData->Operations)
        {
            if (AddedOperationLines >= 2)
            {
                break;
            }

            if (Operation.Type == ENiosStatusEffectOperationType::PeriodicAttributeDelta && Operation.Attribute.IsValid() && !FMath::IsNearlyZero(Operation.Value))
            {
                const FText Label = StatusData->TickInterval > KINDA_SMALL_NUMBER
                    ? FText::Format(FText::FromString(TEXT("Каждые {0}")), SecondsText(StatusData->TickInterval))
                    : FText::FromString(TEXT("Периодически"));

                Section.Lines.Add(Nios_MakeTooltipLine(
                    Label,
                    FText::Format(FText::FromString(TEXT("{0} {1}")), GetGameplayAttributeDisplayName(Operation.Attribute), Nios_SignedFloatText(Operation.Value, 0)),
                    Operation.Value >= 0.f ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative));
                ++AddedOperationLines;
            }
            else if (Operation.Type == ENiosStatusEffectOperationType::AttributeModifier && Operation.StatType != ENiosStatType::None && !FMath::IsNearlyZero(Operation.Value))
            {
                Section.Lines.Add(Nios_MakeTooltipLine(
                    GetStatDisplayName(Operation.StatType),
                    Nios_SignedFloatText(Operation.Value, 0),
                    Operation.Value >= 0.f ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative));
                ++AddedOperationLines;
            }
        }

        if (StatusData->MaxStacks > 1)
        {
            Section.Lines.Add(Nios_MakeTooltipLine(
                FText::FromString(TEXT("Стаки")),
                FText::AsNumber(StatusData->MaxStacks)));
        }
    };

    auto JoinTargetPolicyObjects = [&TargetPolicyObjectText](const TArray<ENiosCoreTargetApplicationPolicy>& Policies) -> FText
    {
        const bool bHasSelf = Policies.Contains(ENiosCoreTargetApplicationPolicy::SourceOnly);
        const bool bHasEnemies = Policies.Contains(ENiosCoreTargetApplicationPolicy::EnemiesOnly);
        const bool bHasAllies = Policies.Contains(ENiosCoreTargetApplicationPolicy::FriendliesOnly);
        const bool bHasAll = Policies.Contains(ENiosCoreTargetApplicationPolicy::AnyIncludingNeutral);
        const bool bHasNonNeutral = Policies.Contains(ENiosCoreTargetApplicationPolicy::NonNeutralOnly);

        if (bHasAll)
        {
            return FText::FromString(TEXT("все цели"));
        }

        if (bHasSelf && bHasEnemies && bHasAllies)
        {
            return FText::FromString(TEXT("себя, союзников и противников"));
        }

        if (bHasSelf && bHasEnemies)
        {
            return FText::FromString(TEXT("себя и противников"));
        }

        if (bHasSelf && bHasAllies)
        {
            return FText::FromString(TEXT("себя и союзников"));
        }

        if (bHasEnemies && bHasAllies)
        {
            return FText::FromString(TEXT("союзников и противников"));
        }

        if (bHasNonNeutral)
        {
            return FText::FromString(TEXT("цели"));
        }

        if (Policies.Num() > 0)
        {
            return TargetPolicyObjectText(Policies[0]);
        }

        return FText::FromString(TEXT("цель"));
    };

    if (SkillData->bRequireWeaponInHands && SkillData->RequiredWeaponTypes.Num() > 0)
    {
        TArray<FText> RequiredWeaponNames;
        RequiredWeaponNames.Reserve(SkillData->RequiredWeaponTypes.Num());

        for (const ENiosWeaponType WeaponType : SkillData->RequiredWeaponTypes)
        {
            if (WeaponType != ENiosWeaponType::None)
            {
                RequiredWeaponNames.Add(GetWeaponTypeDisplayName(WeaponType));
            }
        }

        const FText RequiredWeaponsText = JoinTextsWithComma(RequiredWeaponNames);
        if (!RequiredWeaponsText.IsEmpty())
        {
            OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(
                GetTooltipLabelWithFallback(TEXT("Required"), FText::FromString(TEXT("Требуется"))),
                RequiredWeaponsText,
                ENiosTooltipLineStyle::Warning));
        }
    }

    if (SkillData->CastTime > KINDA_SMALL_NUMBER)
    {
        OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("CastTime")), SecondsText(SkillData->CastTime)));
    }

    for (const FNiosSkillCostDefinition& Cost : SkillData->Costs)
    {
        if (!Cost.IsValid() || !Cost.bSpendResource) { continue; }
        OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetGameplayAttributeDisplayName(Cost.ResourceAttribute), Nios_FloatText(Cost.Value, 0), ENiosTooltipLineStyle::Negative));
    }

    if (SkillData->GetLocalCooldownDuration() > KINDA_SMALL_NUMBER)
    {
        OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Cooldown")), SecondsText(SkillData->GetLocalCooldownDuration())));
    }

    if (SkillData->TargetType == ENiosAbilityTargetType::GroundPoint)
    {
        const FNiosCoreGroundTargetValidationSettings& Ground = SkillData->GroundTargetValidation;
        if (Ground.MaxRange > KINDA_SMALL_NUMBER)
        {
            OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabelWithFallback(TEXT("Range"), FText::FromString(TEXT("Дальность"))), Nios_FloatText(Ground.MaxRange, 0)));
        }
    }

    int32 BestRadius = 0;
    int32 BestMaxTargets = 0;
    bool bHasTeleportDelivery = false;

    for (const FNiosSkillDeliveryDefinition& Delivery : SkillData->Deliveries)
    {
        if (Delivery.DeliveryType == ENiosSkillDeliveryType::AreaAtPoint)
        {
            BestRadius = FMath::Max(BestRadius, FMath::RoundToInt(Delivery.ImpactRadius));
            BestMaxTargets = FMath::Max(BestMaxTargets, Delivery.MaxTargets);
        }
        else if (Delivery.DeliveryType == ENiosSkillDeliveryType::TeleportToPoint)
        {
            bHasTeleportDelivery = true;
        }
    }

    if (BestRadius > 0)
    {
        OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabelWithFallback(TEXT("Radius"), FText::FromString(TEXT("Радиус"))), FText::AsNumber(BestRadius)));
    }

    if (BestMaxTargets > 0)
    {
        OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabelWithFallback(TEXT("MaxTargets"), FText::FromString(TEXT("Макс. целей"))), FText::AsNumber(BestMaxTargets)));
    }

    if (bHasTeleportDelivery || SkillData->SkillEffects.Num() > 0)
    {
        FNiosTooltipSection SummarySection;
        SummarySection.Header = GetTooltipLabelWithFallback(TEXT("Effects"), FText::FromString(TEXT("Эффекты")));

        if (SkillData->TargetType == ENiosAbilityTargetType::GroundPoint)
        {
            SummarySection.Lines.Add(Nios_MakeTooltipLine(FText::FromString(TEXT("Наведение")), FText::FromString(TEXT("Область"))));
        }

        if (bHasTeleportDelivery)
        {
            SummarySection.Lines.Add(Nios_MakeTooltipLine(FText::FromString(TEXT("Перемещение")), FText::FromString(TEXT("К выбранной точке")), ENiosTooltipLineStyle::Positive));
        }

        UNiosCore_StatusEffectCatalog* StatusCatalog = GetStatusEffectCatalog();

        struct FNiosLocalStatusTooltipGroup
        {
            FName EffectID = NAME_None;
            bool bRemove = false;
            const FNiosStatusEffectCatalogEntry* StatusEntry = nullptr;
            const UNiosCore_StatusEffectDataAsset* StatusData = nullptr;
            TArray<ENiosCoreTargetApplicationPolicy> TargetPolicies;
        };

        TArray<FNiosLocalStatusTooltipGroup> StatusGroups;

        auto AddStatusGroup = [&StatusGroups](FName EffectID, bool bRemove, const FNiosStatusEffectCatalogEntry* StatusEntry, const UNiosCore_StatusEffectDataAsset* StatusData, ENiosCoreTargetApplicationPolicy TargetPolicy)
        {
            for (FNiosLocalStatusTooltipGroup& Group : StatusGroups)
            {
                if (Group.EffectID == EffectID && Group.bRemove == bRemove && Group.StatusData == StatusData)
                {
                    Group.TargetPolicies.AddUnique(TargetPolicy);
                    return;
                }
            }

            FNiosLocalStatusTooltipGroup NewGroup;
            NewGroup.EffectID = EffectID;
            NewGroup.bRemove = bRemove;
            NewGroup.StatusEntry = StatusEntry;
            NewGroup.StatusData = StatusData;
            NewGroup.TargetPolicies.AddUnique(TargetPolicy);
            StatusGroups.Add(NewGroup);
        };

        for (const FNiosSkillEffectDefinition& Effect : SkillData->SkillEffects)
        {
            if (Effect.EffectType == ENiosSkillEffectType::ApplyStatusEffect || Effect.EffectType == ENiosSkillEffectType::RemoveStatusEffect)
            {
                const FNiosStatusEffectCatalogEntry* StatusEntry = StatusCatalog ? StatusCatalog->FindStatusEffect(Effect.StatusEffectID) : nullptr;
                const UNiosCore_StatusEffectDataAsset* StatusData = StatusEntry ? StatusEntry->EffectData : nullptr;
                const ENiosCoreTargetApplicationPolicy EffectiveTargetPolicy = ResolveEffectiveTargetPolicy(Effect, StatusData);
                AddStatusGroup(Effect.StatusEffectID, Effect.EffectType == ENiosSkillEffectType::RemoveStatusEffect, StatusEntry, StatusData, EffectiveTargetPolicy);
                continue;
            }

            if (!Effect.Attribute.IsValid()) { continue; }

            if (Effect.EffectType == ENiosSkillEffectType::Damage)
            {
                const ENiosCoreTargetApplicationPolicy EffectiveTargetPolicy = ResolveEffectiveTargetPolicy(Effect, nullptr);
                AddSignedAttributeLine(SummarySection, FText::Format(FText::FromString(TEXT("Урон {0}")), TargetPolicyLabelSuffix(EffectiveTargetPolicy)), Effect.Attribute, -FMath::Abs(Effect.BaseValue));
            }
            else if (Effect.EffectType == ENiosSkillEffectType::Heal)
            {
                const ENiosCoreTargetApplicationPolicy EffectiveTargetPolicy = ResolveEffectiveTargetPolicy(Effect, nullptr);
                AddSignedAttributeLine(SummarySection, FText::Format(FText::FromString(TEXT("Исцеление {0}")), TargetPolicyLabelSuffix(EffectiveTargetPolicy)), Effect.Attribute, FMath::Abs(Effect.BaseValue));
            }
            else if (Effect.EffectType == ENiosSkillEffectType::ResourceDelta || Effect.EffectType == ENiosSkillEffectType::AttributeDelta)
            {
                const ENiosCoreTargetApplicationPolicy EffectiveTargetPolicy = ResolveEffectiveTargetPolicy(Effect, nullptr);
                AddSignedAttributeLine(SummarySection, FText::Format(FText::FromString(TEXT("Изменение {0}")), TargetPolicyLabelSuffix(EffectiveTargetPolicy)), Effect.Attribute, Effect.BaseValue);
            }
        }

        for (const FNiosLocalStatusTooltipGroup& Group : StatusGroups)
        {
            AddStatusEffectShortSummary(SummarySection, Group.StatusEntry, Group.StatusData, Group.bRemove, JoinTargetPolicyObjects(Group.TargetPolicies));
        }

        if (SummarySection.Lines.Num() > 0)
        {
            OutTooltipData.Sections.Add(SummarySection);
        }
    }

    return true;
}

bool UActionSlotResolverSubsystem::ResolveStatTooltipData(const FNiosCalculatedStat& Stat, FNiosTooltipData& OutTooltipData)
{
    OutTooltipData.Reset();

    if (Stat.StatType == ENiosStatType::None)
    {
        return false;
    }

    OutTooltipData.bValid = true;
    OutTooltipData.Type = ENiosTooltipType::Stat;
    OutTooltipData.SourceID = FName(*UEnum::GetValueAsString(Stat.StatType));
    OutTooltipData.Title = GetStatDisplayName(Stat.StatType);
    OutTooltipData.Subtitle = GetTooltipLabel(TEXT("CharacterStat"));

    OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Base")), Nios_FloatText(Stat.BaseValue, 0)));
    OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Equipment")), Nios_FloatText(Stat.EquipmentValue, 0), Stat.EquipmentValue >= 0.f ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative));
    OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Passive")), Nios_FloatText(Stat.PassiveValue, 0)));
    OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Buff")), Nios_FloatText(Stat.BuffValue, 0), Stat.BuffValue >= 0.f ? ENiosTooltipLineStyle::Positive : ENiosTooltipLineStyle::Negative));
    OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Debuff")), Nios_FloatText(Stat.DebuffValue, 0), ENiosTooltipLineStyle::Negative));
    OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Runtime")), Nios_FloatText(Stat.RuntimeValue, 0)));
    OutTooltipData.MainLines.Add(Nios_MakeTooltipLine(GetTooltipLabel(TEXT("Final")), Nios_FloatText(Stat.FinalValue, 0), ENiosTooltipLineStyle::Header));

    return true;
}

bool UActionSlotResolverSubsystem::ResolveSkillDataAsset(FName SkillID, UNiosCore_SkillDataAsset*& OutSkillData)
{
    OutSkillData = nullptr;

    if (SkillID.IsNone())
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> ACTION SLOT RESOLVE SKILL DA FAILED: SkillID=None"));
        return false;
    }

    UNiosCore_SkillCatalog* SkillCatalog = GetSkillCatalog();
    if (!SkillCatalog)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> ACTION SLOT RESOLVE SKILL DA FAILED: SkillCatalog=None SkillID=%s"), *SkillID.ToString());
        return false;
    }

    OutSkillData = SkillCatalog->GetSkillData(SkillID);
    if (!OutSkillData)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> ACTION SLOT RESOLVE SKILL DA FAILED: SkillData=None SkillID=%s"), *SkillID.ToString());
        return false;
    }

    return true;
}

bool UActionSlotResolverSubsystem::ResolveSkillDataAssetFromActivationRequest(const FNiosCoreActivationRequest& Request, UNiosCore_SkillDataAsset*& OutSkillData)
{
    OutSkillData = nullptr;

    if (Request.Kind != ENiosCoreActivationKind::Skill)
    {
        return false;
    }

    return ResolveSkillDataAsset(Request.ID, OutSkillData);
}

bool UActionSlotResolverSubsystem::GetSkillTargetType(FName SkillID, ENiosAbilityTargetType& OutTargetType)
{
    OutTargetType = ENiosAbilityTargetType::None;

    UNiosCore_SkillDataAsset* SkillData = nullptr;
    if (!ResolveSkillDataAsset(SkillID, SkillData) || !SkillData)
    {
        return false;
    }

    OutTargetType = SkillData->TargetType;
    return true;
}

bool UActionSlotResolverSubsystem::GetSkillTargetTypeFromActivationRequest(const FNiosCoreActivationRequest& Request, ENiosAbilityTargetType& OutTargetType)
{
    OutTargetType = ENiosAbilityTargetType::None;

    if (Request.Kind != ENiosCoreActivationKind::Skill)
    {
        return false;
    }

    return GetSkillTargetType(Request.ID, OutTargetType);
}

bool UActionSlotResolverSubsystem::IsGroundTargetedSkill(const FNiosCoreActivationRequest& Request)
{
    if (Request.Kind != ENiosCoreActivationKind::Skill)
    {
        return false;
    }

    return IsGroundTargetedSkillByID(Request.ID);
}

bool UActionSlotResolverSubsystem::IsGroundTargetedSkillByID(FName SkillID)
{
    ENiosAbilityTargetType TargetType = ENiosAbilityTargetType::None;
    if (!GetSkillTargetType(SkillID, TargetType))
    {
        return false;
    }

    return TargetType == ENiosAbilityTargetType::GroundPoint;
}

bool UActionSlotResolverSubsystem::ShouldStartGroundTargetingForRequest(const FNiosCoreActivationRequest& Request, FName& OutSkillID)
{
    OutSkillID = NAME_None;

    if (Request.Kind != ENiosCoreActivationKind::Skill || Request.ID.IsNone())
    {
        return false;
    }

    if (!IsGroundTargetedSkillByID(Request.ID))
    {
        return false;
    }

    OutSkillID = Request.ID;
    return true;
}


namespace
{
    float Nios_ReadOptionalSkillFloatProperty(const UNiosCore_SkillDataAsset* SkillData, const FName PropertyName, float DefaultValue)
    {
        if (!SkillData || PropertyName.IsNone())
        {
            return DefaultValue;
        }

        const FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(SkillData->GetClass(), PropertyName);
        if (!FloatProperty)
        {
            return DefaultValue;
        }

        return FloatProperty->GetPropertyValue_InContainer(SkillData);
    }
}

bool UActionSlotResolverSubsystem::GetSkillGroundTargetingDistance(FName SkillID, float& OutDistance)
{
    OutDistance = 0.f;

    UNiosCore_SkillDataAsset* SkillData = nullptr;
    if (!ResolveSkillDataAsset(SkillID, SkillData) || !SkillData)
    {
        return false;
    }

    if (SkillData->TargetType != ENiosAbilityTargetType::GroundPoint)
    {
        return false;
    }

    // Future-proof: if/when SkillDataAsset exposes one of these fields, resolver will use it
    // without ActionSlot needing to know where it came from. Until then, default is safe for debug.
    float Distance = 1200.f;
    Distance = Nios_ReadOptionalSkillFloatProperty(SkillData, TEXT("GroundTargetingDistance"), Distance);
    Distance = Nios_ReadOptionalSkillFloatProperty(SkillData, TEXT("MaxGroundTargetingDistance"), Distance);
    Distance = Nios_ReadOptionalSkillFloatProperty(SkillData, TEXT("MaxCastRange"), Distance);
    Distance = Nios_ReadOptionalSkillFloatProperty(SkillData, TEXT("MaxRange"), Distance);

    OutDistance = FMath::Max(0.f, Distance);
    return true;
}

bool UActionSlotResolverSubsystem::ShouldStartGroundTargetingForRequestWithDistance(const FNiosCoreActivationRequest& Request, FName& OutSkillID, float& OutDistance)
{
    OutSkillID = NAME_None;
    OutDistance = 0.f;

    if (!ShouldStartGroundTargetingForRequest(Request, OutSkillID))
    {
        return false;
    }

    GetSkillGroundTargetingDistance(OutSkillID, OutDistance);
    return true;
}

bool UActionSlotResolverSubsystem::ResolveGroundTargetingPayloadForRequest(const FNiosCoreActivationRequest& Request, FNiosCoreGroundTargetingPayload& OutPayload)
{
    OutPayload = FNiosCoreGroundTargetingPayload();

    if (Request.Kind != ENiosCoreActivationKind::Skill || Request.ID.IsNone())
    {
        return false;
    }

    UNiosCore_SkillDataAsset* SkillData = nullptr;
    if (!ResolveSkillDataAsset(Request.ID, SkillData) || !SkillData)
    {
        return false;
    }

    OutPayload.SkillData = SkillData;
    OutPayload.SkillID = Request.ID;
    OutPayload.TargetType = SkillData->TargetType;
    OutPayload.bShouldStartGroundTargeting = SkillData->TargetType == ENiosAbilityTargetType::GroundPoint;

    if (OutPayload.bShouldStartGroundTargeting)
    {
        GetSkillGroundTargetingDistance(Request.ID, OutPayload.Distance);
    }

    return true;
}

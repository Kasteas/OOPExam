#include "Components/Equipment/NiosCore_EquipmentComponent.h"

#include "Data/Resolvers/ActionSlotResolverSubsystem.h"
#include "Data/Config/NiosCore_DatabaseSettings.h"
#include "Components/Inventory/NiosCore_InventoryComponent.h"
#include "Components/Stats/NiosCore_StatsComponent.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Data/StatusEffects/NiosCore_StatusEffectCatalog.h"
#include "Data/StatusEffects/NiosCore_StatusEffectDataAsset.h"
#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "HAL/IConsoleManager.h"
static TAutoConsoleVariable<int32> CVarNiosEquipmentDebug(
    TEXT("nios.Equipment.Debug"),
    0,
    TEXT("Nios equipment debug log level. 0=off, 1=attempts/failures, 2=resolve/validation, 3=stats/effects detail."),
    ECVF_Default);

#define NIOS_EQUIP_LOG(Format, ...) \
    do \
    { \
        if (CVarNiosEquipmentDebug.GetValueOnAnyThread() >= 1) \
        { \
            UE_LOG(LogTemp, Warning, TEXT("[EQUIP] " Format), ##__VA_ARGS__); \
        } \
    } while (0)

#define NIOS_EQUIP_LOGV(Level, Format, ...) \
    do \
    { \
        if (CVarNiosEquipmentDebug.GetValueOnAnyThread() >= Level) \
        { \
            UE_LOG(LogTemp, Warning, TEXT("[EQUIP] " Format), ##__VA_ARGS__); \
        } \
    } while (0)

static const TCHAR* NiosEquipmentRoleToText(ENetRole Role)
{
    switch (Role)
    {
    case ROLE_Authority: return TEXT("Authority");
    case ROLE_AutonomousProxy: return TEXT("AutonomousProxy");
    case ROLE_SimulatedProxy: return TEXT("SimulatedProxy");
    case ROLE_None: default: return TEXT("None");
    }
}

static bool NiosEquipment_IsAliveForEquipment(const AActor* Actor)
{
    if (!Actor)
    {
        return true;
    }

    AActor* MutableActor = const_cast<AActor*>(Actor);
    if (MutableActor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
    {
        return INiosGASActorInterface::Execute_IsAlive(MutableActor);
    }

    return true;
}

UNiosCore_EquipmentComponent::UNiosCore_EquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UNiosCore_EquipmentComponent::BeginPlay()
{
    Super::BeginPlay();

    NIOS_EQUIP_LOG("BeginPlay Owner=%s Role=%s EquippedSlots=%d",
        *GetNameSafe(GetOwner()),
        NiosEquipmentRoleToText(GetOwnerRole()),
        EquippedItems.Num());

    BindInventoryEvents(ResolveInventoryComponent());
    ValidateEquipmentAgainstInventory();
    RebuildEquipmentStats();
    RebuildEquipmentStatusEffects();
    BroadcastEquipmentVisualsChanged();
    SyncWeaponVisualStateWithEquipment();
    BroadcastWeaponVisualStateChanged(true);
}

void UNiosCore_EquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UNiosCore_EquipmentComponent, EquippedItems);
    DOREPLIFETIME(UNiosCore_EquipmentComponent, WeaponVisualState);
}

void UNiosCore_EquipmentComponent::InitializeInventoryComponent(UNiosCore_InventoryComponent* InInventoryComponent)
{
    NIOS_EQUIP_LOG("InitializeInventoryComponent Owner=%s Inventory=%s",
        *GetNameSafe(GetOwner()),
        *GetNameSafe(InInventoryComponent));
    BindInventoryEvents(InInventoryComponent);
    ValidateEquipmentAgainstInventory();
    RebuildEquipmentStats();
    RebuildEquipmentStatusEffects();
}

UNiosCore_InventoryComponent* UNiosCore_EquipmentComponent::GetInventoryComponent() const
{
    return ResolveInventoryComponent();
}

void UNiosCore_EquipmentComponent::UnbindInventoryEvents()
{
    if (!InventoryComponent)
    {
        return;
    }

    InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UNiosCore_EquipmentComponent::HandleLinkedInventoryChanged);
    InventoryComponent->OnInventorySlotChanged.RemoveDynamic(this, &UNiosCore_EquipmentComponent::HandleLinkedInventorySlotChanged);
}

void UNiosCore_EquipmentComponent::BindInventoryEvents(UNiosCore_InventoryComponent* InInventoryComponent)
{
    if (InventoryComponent == InInventoryComponent)
    {
        return;
    }

    UnbindInventoryEvents();
    InventoryComponent = InInventoryComponent;

    if (!InventoryComponent)
    {
        NIOS_EQUIP_LOG("BindInventoryEvents skipped Owner=%s Reason=NoInventory", *GetNameSafe(GetOwner()));
        return;
    }

    NIOS_EQUIP_LOG("BindInventoryEvents Owner=%s Inventory=%s Slots=%d",
        *GetNameSafe(GetOwner()),
        *GetNameSafe(InventoryComponent),
        InventoryComponent->InventorySlots.Num());

    InventoryComponent->OnInventoryChanged.AddUniqueDynamic(this, &UNiosCore_EquipmentComponent::HandleLinkedInventoryChanged);
    InventoryComponent->OnInventorySlotChanged.AddUniqueDynamic(this, &UNiosCore_EquipmentComponent::HandleLinkedInventorySlotChanged);
}

void UNiosCore_EquipmentComponent::HandleLinkedInventoryChanged()
{
    NIOS_EQUIP_LOGV(2, "InventoryChanged Owner=%s", *GetNameSafe(GetOwner()));
    ValidateEquipmentAgainstInventory();
}

void UNiosCore_EquipmentComponent::HandleLinkedInventorySlotChanged(int32 SlotIndex)
{
    NIOS_EQUIP_LOGV(2, "InventorySlotChanged Owner=%s SlotIndex=%d", *GetNameSafe(GetOwner()), SlotIndex);
    ValidateEquipmentAgainstInventory();
}

int32 UNiosCore_EquipmentComponent::FindSlotIndex(EEquipSlot Slot) const
{
    for (int32 i = 0; i < EquippedItems.Num(); ++i)
    {
        if (EquippedItems[i].Slot == Slot)
        {
            return i;
        }
    }

    return INDEX_NONE;
}

void UNiosCore_EquipmentComponent::SetSlot(EEquipSlot Slot, const FReplicatedEquipmentSlot& Equipment)
{
    if (Slot == EEquipSlot::None)
    {
        return;
    }

    FReplicatedEquipmentSlot NewEquipment = Equipment;
    NewEquipment.Slot = Slot;

    FReplicatedEquipmentSlot PreviousEquipment;
    PreviousEquipment.Slot = Slot;

    const int32 Index = FindSlotIndex(Slot);
    if (Index != INDEX_NONE)
    {
        PreviousEquipment = EquippedItems[Index];
        EquippedItems[Index] = NewEquipment;
    }
    else
    {
        EquippedItems.Add(NewEquipment);
    }
    NIOS_EQUIP_LOG("SetSlot Slot=%s Item=%s Instance=%d",
        *UEnum::GetValueAsString(Slot),
        *NewEquipment.ItemID.ToString(),
        NewEquipment.ItemInstanceIndex);
    OnEquipmentChanged.Broadcast(Slot);
    BroadcastEquipmentVisualSlotChanged(Slot);

    if (PreviousEquipment.ItemInstanceIndex != INDEX_NONE &&
        PreviousEquipment.ItemInstanceIndex != NewEquipment.ItemInstanceIndex)
    {
        RemoveEquipmentStatsForItemInstance(PreviousEquipment.ItemInstanceIndex);
        RemoveEquipmentStatusEffectsForItemInstance(PreviousEquipment.ItemInstanceIndex);
    }

    if (NewEquipment.ItemInstanceIndex != INDEX_NONE)
    {
        AddEquipmentStatsForSlot(NewEquipment);
        AddEquipmentStatusEffectsForSlot(NewEquipment);
    }

    if (UNiosCore_InventoryComponent* Inv = ResolveInventoryComponent())
    {
        if (PreviousEquipment.ItemInstanceIndex != INDEX_NONE &&
            PreviousEquipment.ItemInstanceIndex != NewEquipment.ItemInstanceIndex)
        {
            Inv->SetItemEquippedState(PreviousEquipment.ItemInstanceIndex, false);
        }

        if (NewEquipment.ItemInstanceIndex != INDEX_NONE)
        {
            Inv->SetItemEquippedState(NewEquipment.ItemInstanceIndex, true);
        }
    }

    SyncWeaponVisualStateWithEquipment();
}

void UNiosCore_EquipmentComponent::ClearSlotInternal(EEquipSlot Slot)
{
    const FReplicatedEquipmentSlot PreviousSlot = GetEquipmentSlot(Slot);

    NIOS_EQUIP_LOG("ClearSlotInternal Slot=%s PrevItem=%s PrevInstance=%d",
        *UEnum::GetValueAsString(Slot),
        *PreviousSlot.ItemID.ToString(),
        PreviousSlot.ItemInstanceIndex);

    FReplicatedEquipmentSlot Empty;
    Empty.Slot = Slot;

    SetSlot(Slot, Empty);
}

void UNiosCore_EquipmentComponent::NotifyInventorySlotForItemInstanceChanged(int32 ItemInstanceIndex) const
{
    if (ItemInstanceIndex == INDEX_NONE)
        return;

    UNiosCore_InventoryComponent* ResolvedInventoryComponent = ResolveInventoryComponent();
    if (!ResolvedInventoryComponent)
        return;

    const int32 InventorySlotIndex =
        ResolvedInventoryComponent->FindSlotIndexByItemInstanceIndex(ItemInstanceIndex);

    if (InventorySlotIndex != INDEX_NONE)
    {
        ResolvedInventoryComponent->OnInventorySlotChanged.Broadcast(InventorySlotIndex);
    }

    // �� �������� OnInventoryChanged ���.
    // ���������� �� ������ ������ ���������, ��� ������ ������ ���������� ��������� ������ �����.
}

UActionSlotResolverSubsystem* UNiosCore_EquipmentComponent::ResolveResolverSubsystem() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    UGameInstance* GameInstance = World->GetGameInstance();
    if (!GameInstance)
    {
        return nullptr;
    }

    return GameInstance->GetSubsystem<UActionSlotResolverSubsystem>();
}

bool UNiosCore_EquipmentComponent::ResolveItemData(FName ItemID, FNiosResolvedInventoryItemData& OutItemData) const
{
    if (ItemID.IsNone())
    {
        OutItemData = FNiosResolvedInventoryItemData();
        NIOS_EQUIP_LOGV(2, "ResolveItemData failed Owner=%s Item=None", *GetNameSafe(GetOwner()));
        return false;
    }

    UActionSlotResolverSubsystem* ResolverSubsystem = ResolveResolverSubsystem();
    if (!ResolverSubsystem)
    {
        NIOS_EQUIP_LOG("ResolveItemData failed Owner=%s Item=%s Reason=NoResolverSubsystem",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString());
        return false;
    }

    const bool bResolved = ResolverSubsystem->ResolveInventoryItemData(ItemID, OutItemData) && OutItemData.bValid;
    NIOS_EQUIP_LOGV(2, "ResolveItemData Owner=%s Item=%s Resolved=%d Valid=%d Type=%s",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        bResolved ? 1 : 0,
        OutItemData.bValid ? 1 : 0,
        *UEnum::GetValueAsString(OutItemData.ItemType));
    return bResolved;
}

UNiosCore_InventoryComponent* UNiosCore_EquipmentComponent::ResolveInventoryComponent() const
{
    if (InventoryComponent)
    {
        return InventoryComponent;
    }

    AActor* Owner = GetOwner();
    UNiosCore_InventoryComponent* Found = Owner ? Owner->FindComponentByClass<UNiosCore_InventoryComponent>() : nullptr;
    if (!Found)
    {
        NIOS_EQUIP_LOGV(2, "ResolveInventoryComponent failed Owner=%s", *GetNameSafe(Owner));
    }
    return Found;
}

UNiosCore_StatsComponent* UNiosCore_EquipmentComponent::ResolveStatsComponent() const
{
    if (StatsComponent)
    {
        NIOS_EQUIP_LOGV(3, "ResolveStatsComponent Owner=%s Source=Cached Component=%s", *GetNameSafe(GetOwner()), *GetNameSafe(StatsComponent));
        return StatsComponent;
    }

    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    if (UNiosCore_StatsComponent* OwnerStats = Owner->FindComponentByClass<UNiosCore_StatsComponent>())
    {
        NIOS_EQUIP_LOGV(3, "ResolveStatsComponent Owner=%s Source=Owner Component=%s", *GetNameSafe(Owner), *GetNameSafe(OwnerStats));
        return OwnerStats;
    }

    // MMO layout: player gameplay state/components live on PlayerState, while equipment may live on Character.
    if (const APawn* PawnOwner = Cast<APawn>(Owner))
    {
        if (APlayerState* PlayerState = PawnOwner->GetPlayerState())
        {
            if (UNiosCore_StatsComponent* PlayerStateStats = PlayerState->FindComponentByClass<UNiosCore_StatsComponent>())
            {
                NIOS_EQUIP_LOGV(3, "ResolveStatsComponent Owner=%s Source=PlayerState PlayerState=%s Component=%s",
                    *GetNameSafe(Owner),
                    *GetNameSafe(PlayerState),
                    *GetNameSafe(PlayerStateStats));
                return PlayerStateStats;
            }
        }
    }

    NIOS_EQUIP_LOG("ResolveStatsComponent failed Owner=%s", *GetNameSafe(Owner));
    return nullptr;
}

UNiosCore_StatusEffectComponent* UNiosCore_EquipmentComponent::ResolveStatusEffectComponent() const
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return nullptr;
    }

    if (UNiosCore_StatusEffectComponent* StatusEffects = Owner->FindComponentByClass<UNiosCore_StatusEffectComponent>())
    {
        return StatusEffects;
    }

    if (const APawn* PawnOwner = Cast<APawn>(Owner))
    {
        if (APlayerState* PlayerState = PawnOwner->GetPlayerState())
        {
            if (UNiosCore_StatusEffectComponent* StatusEffects = PlayerState->FindComponentByClass<UNiosCore_StatusEffectComponent>())
            {
                return StatusEffects;
            }
        }
    }

    return nullptr;
}

FName UNiosCore_EquipmentComponent::MakeEquipmentStatsSourceID(int32 ItemInstanceIndex) const
{
    if (ItemInstanceIndex == INDEX_NONE)
    {
        return NAME_None;
    }

    return FName(*FString::Printf(TEXT("Equipment_%d"), ItemInstanceIndex));
}

FName UNiosCore_EquipmentComponent::MakeEquipmentStatusEffectSourceID(int32 ItemInstanceIndex) const
{
    if (ItemInstanceIndex == INDEX_NONE)
    {
        return NAME_None;
    }

    return FName(*FString::Printf(TEXT("Equipment_%d"), ItemInstanceIndex));
}

void UNiosCore_EquipmentComponent::RemoveEquipmentStatsForItemInstance(int32 ItemInstanceIndex) const
{
    if (GetOwnerRole() != ROLE_Authority || ItemInstanceIndex == INDEX_NONE)
    {
        NIOS_EQUIP_LOGV(3, "RemoveEquipmentStats skipped Owner=%s Instance=%d Role=%s",
            *GetNameSafe(GetOwner()),
            ItemInstanceIndex,
            NiosEquipmentRoleToText(GetOwnerRole()));
        return;
    }

    UNiosCore_StatsComponent* ResolvedStatsComponent = ResolveStatsComponent();
    if (!ResolvedStatsComponent)
    {
        NIOS_EQUIP_LOG("RemoveEquipmentStats failed Owner=%s Instance=%d Reason=NoStatsComponent",
            *GetNameSafe(GetOwner()),
            ItemInstanceIndex);
        return;
    }

    const FName SourceID = MakeEquipmentStatsSourceID(ItemInstanceIndex);
    if (!SourceID.IsNone())
    {
        NIOS_EQUIP_LOGV(2, "RemoveEquipmentStats Owner=%s Instance=%d Source=%s",
            *GetNameSafe(GetOwner()),
            ItemInstanceIndex,
            *SourceID.ToString());
        ResolvedStatsComponent->RemoveModifierSource(SourceID);
    }
}

void UNiosCore_EquipmentComponent::RemoveEquipmentStatusEffectsForItemInstance(int32 ItemInstanceIndex) const
{
    if (GetOwnerRole() != ROLE_Authority || ItemInstanceIndex == INDEX_NONE)
    {
        return;
    }

    UNiosCore_StatusEffectComponent* StatusEffects = ResolveStatusEffectComponent();
    if (!StatusEffects)
    {
        return;
    }

    const FName SourceID = MakeEquipmentStatusEffectSourceID(ItemInstanceIndex);
    if (!SourceID.IsNone())
    {
        const int32 Removed = StatusEffects->RemoveStatusEffectsBySource(ENiosStatusEffectSourceType::Equipment, SourceID);
        if (Removed > 0)
        {
            NIOS_EQUIP_LOG("Removed %d equipment status effects Source=%s", Removed, *SourceID.ToString());
        }
    }
}

void UNiosCore_EquipmentComponent::AddEquipmentStatsForSlot(const FReplicatedEquipmentSlot& EquipmentSlot) const
{
    if (GetOwnerRole() != ROLE_Authority || EquipmentSlot.IsEmpty() || EquipmentSlot.ItemInstanceIndex == INDEX_NONE)
    {
        NIOS_EQUIP_LOGV(3, "AddEquipmentStats skipped Owner=%s Slot=%s Item=%s Instance=%d Role=%s Empty=%d",
            *GetNameSafe(GetOwner()),
            *UEnum::GetValueAsString(EquipmentSlot.Slot),
            *EquipmentSlot.ItemID.ToString(),
            EquipmentSlot.ItemInstanceIndex,
            NiosEquipmentRoleToText(GetOwnerRole()),
            EquipmentSlot.IsEmpty() ? 1 : 0);
        return;
    }

    UNiosCore_StatsComponent* ResolvedStatsComponent = ResolveStatsComponent();
    if (!ResolvedStatsComponent)
    {
        NIOS_EQUIP_LOG("AddEquipmentStats failed Owner=%s Slot=%s Item=%s Instance=%d Reason=NoStatsComponent",
            *GetNameSafe(GetOwner()),
            *UEnum::GetValueAsString(EquipmentSlot.Slot),
            *EquipmentSlot.ItemID.ToString(),
            EquipmentSlot.ItemInstanceIndex);
        return;
    }

    FNiosStatModifierSource Source;
    Source.SourceID = MakeEquipmentStatsSourceID(EquipmentSlot.ItemInstanceIndex);
    Source.Layer = ENiosStatModifierLayer::Equipment;

    FNiosResolvedInventoryItemData ItemData;
    if (!ResolveItemData(EquipmentSlot.ItemID, ItemData))
    {
        NIOS_EQUIP_LOG("AddEquipmentStats failed Owner=%s Slot=%s Item=%s Instance=%d Reason=ResolveItemData",
            *GetNameSafe(GetOwner()),
            *UEnum::GetValueAsString(EquipmentSlot.Slot),
            *EquipmentSlot.ItemID.ToString(),
            EquipmentSlot.ItemInstanceIndex);
        return;
    }

    if (ItemData.ItemType == ENiosItemType::Weapon)
    {
        FNiosWeaponRow WeaponRow;
        if (ResolveWeaponRow(EquipmentSlot.ItemID, WeaponRow))
        {
            Source.Modifiers = WeaponRow.StatModifiers;
            NIOS_EQUIP_LOGV(2, "AddEquipmentStats resolved weapon Owner=%s Item=%s Modifiers=%d",
                *GetNameSafe(GetOwner()),
                *EquipmentSlot.ItemID.ToString(),
                Source.Modifiers.Num());
        }
        else
        {
            NIOS_EQUIP_LOG("AddEquipmentStats failed Owner=%s Item=%s Reason=ResolveWeaponRow",
                *GetNameSafe(GetOwner()),
                *EquipmentSlot.ItemID.ToString());
        }
    }
    else if (ItemData.ItemType == ENiosItemType::Armor)
    {
        FNiosArmorRow ArmorRow;
        if (ResolveArmorRow(EquipmentSlot.ItemID, ArmorRow))
        {
            Source.Modifiers = ArmorRow.StatModifiers;
            NIOS_EQUIP_LOGV(2, "AddEquipmentStats resolved armor Owner=%s Item=%s ArmorSlot=%s Modifiers=%d",
                *GetNameSafe(GetOwner()),
                *EquipmentSlot.ItemID.ToString(),
                *UEnum::GetValueAsString(ArmorRow.ArmorSlot),
                Source.Modifiers.Num());
        }
        else
        {
            NIOS_EQUIP_LOG("AddEquipmentStats failed Owner=%s Item=%s Reason=ResolveArmorRow",
                *GetNameSafe(GetOwner()),
                *EquipmentSlot.ItemID.ToString());
        }
    }
    else
    {
        NIOS_EQUIP_LOG("AddEquipmentStats skipped Owner=%s Item=%s Reason=ItemTypeNotEquipment Type=%s",
            *GetNameSafe(GetOwner()),
            *EquipmentSlot.ItemID.ToString(),
            *UEnum::GetValueAsString(ItemData.ItemType));
    }

    if (Source.IsValid())
    {
        NIOS_EQUIP_LOG("AddEquipmentStats apply Owner=%s Stats=%s Item=%s Instance=%d Source=%s Modifiers=%d",
            *GetNameSafe(GetOwner()),
            *GetNameSafe(ResolvedStatsComponent),
            *EquipmentSlot.ItemID.ToString(),
            EquipmentSlot.ItemInstanceIndex,
            *Source.SourceID.ToString(),
            Source.Modifiers.Num());
        ResolvedStatsComponent->AddOrReplaceModifierSource(Source);
    }
    else
    {
        NIOS_EQUIP_LOG("AddEquipmentStats no valid source Owner=%s Item=%s Instance=%d Source=%s Modifiers=%d",
            *GetNameSafe(GetOwner()),
            *EquipmentSlot.ItemID.ToString(),
            EquipmentSlot.ItemInstanceIndex,
            *Source.SourceID.ToString(),
            Source.Modifiers.Num());
    }
}

void UNiosCore_EquipmentComponent::AddEquipmentStatusEffectsForSlot(const FReplicatedEquipmentSlot& EquipmentSlot) const
{
    if (GetOwnerRole() != ROLE_Authority || EquipmentSlot.IsEmpty() || EquipmentSlot.ItemInstanceIndex == INDEX_NONE)
    {
        return;
    }

    UNiosCore_StatusEffectComponent* StatusEffects = ResolveStatusEffectComponent();
    if (!StatusEffects)
    {
        UE_LOG(LogTemp, Warning, TEXT(">>> EQUIPMENT STATUS EFFECT SKIPPED: no StatusEffectComponent Owner=%s Item=%s Instance=%d"),
            *GetNameSafe(GetOwner()),
            *EquipmentSlot.ItemID.ToString(),
            EquipmentSlot.ItemInstanceIndex);
        return;
    }

    FNiosResolvedInventoryItemData ItemData;
    if (!ResolveItemData(EquipmentSlot.ItemID, ItemData))
    {
        return;
    }

    TArray<FName> GrantedEffectIDs;
    if (ItemData.ItemType == ENiosItemType::Weapon)
    {
        FNiosWeaponRow WeaponRow;
        if (ResolveWeaponRow(EquipmentSlot.ItemID, WeaponRow))
        {
            GrantedEffectIDs = WeaponRow.GrantedStatusEffectIDs;
        }
    }
    else if (ItemData.ItemType == ENiosItemType::Armor)
    {
        FNiosArmorRow ArmorRow;
        if (ResolveArmorRow(EquipmentSlot.ItemID, ArmorRow))
        {
            GrantedEffectIDs = ArmorRow.GrantedStatusEffectIDs;
        }
    }

    if (GrantedEffectIDs.Num() == 0)
    {
        return;
    }

    const FName SourceID = MakeEquipmentStatusEffectSourceID(EquipmentSlot.ItemInstanceIndex);
    if (!SourceID.IsNone())
    {
        // Re-applying the same equipment instance must replace, not stack.
        // This also protects two-handed items that may occupy two equipment slots with one ItemInstanceIndex.
        StatusEffects->RemoveStatusEffectsBySource(ENiosStatusEffectSourceType::Equipment, SourceID);
    }

    for (const FName EffectID : GrantedEffectIDs)
    {
        if (EffectID.IsNone())
        {
            continue;
        }

        if (!IsValidEquipmentGrantedStatusEffect(EffectID))
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> EQUIPMENT STATUS EFFECT REJECTED: EffectID=%s Item=%s Instance=%d Reason=NotInfiniteOrMissing"),
                *EffectID.ToString(),
                *EquipmentSlot.ItemID.ToString(),
                EquipmentSlot.ItemInstanceIndex);
            continue;
        }

        const FGuid Handle = StatusEffects->ApplyStatusEffectByID(EffectID, GetOwner(), ENiosStatusEffectSourceType::Equipment, SourceID);
        if (Handle.IsValid())
        {
            NIOS_EQUIP_LOG("Applied equipment status effect Effect=%s Source=%s", *EffectID.ToString(), *SourceID.ToString());
        }
    }
}

void UNiosCore_EquipmentComponent::RefreshEquipmentStatusEffectsAfterRevive()
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    RebuildEquipmentStatusEffects();
}

void UNiosCore_EquipmentComponent::RebuildEquipmentStats() const
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        NIOS_EQUIP_LOGV(2, "RebuildEquipmentStats skipped Owner=%s Role=%s",
            *GetNameSafe(GetOwner()),
            NiosEquipmentRoleToText(GetOwnerRole()));
        return;
    }

    UNiosCore_StatsComponent* ResolvedStatsComponent = ResolveStatsComponent();
    if (!ResolvedStatsComponent)
    {
        NIOS_EQUIP_LOG("RebuildEquipmentStats failed Owner=%s Reason=NoStatsComponent", *GetNameSafe(GetOwner()));
        return;
    }

    NIOS_EQUIP_LOG("RebuildEquipmentStats Owner=%s Stats=%s EquippedSlots=%d",
        *GetNameSafe(GetOwner()),
        *GetNameSafe(ResolvedStatsComponent),
        EquippedItems.Num());

    ResolvedStatsComponent->RemoveModifierSourcesByLayer(ENiosStatModifierLayer::Equipment);

    for (const FReplicatedEquipmentSlot& EquipmentSlot : EquippedItems)
    {
        if (!EquipmentSlot.IsEmpty())
        {
            AddEquipmentStatsForSlot(EquipmentSlot);
        }
    }
}

void UNiosCore_EquipmentComponent::RebuildEquipmentStatusEffects() const
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    UNiosCore_StatusEffectComponent* StatusEffects = ResolveStatusEffectComponent();
    if (!StatusEffects)
    {
        return;
    }

    StatusEffects->RemoveStatusEffectsBySource(ENiosStatusEffectSourceType::Equipment, NAME_None);

    for (const FReplicatedEquipmentSlot& EquipmentSlot : EquippedItems)
    {
        if (!EquipmentSlot.IsEmpty())
        {
            AddEquipmentStatusEffectsForSlot(EquipmentSlot);
        }
    }
}

bool UNiosCore_EquipmentComponent::IsValidEquipmentGrantedStatusEffect(FName EffectID) const
{
    if (EffectID.IsNone())
    {
        return false;
    }

    UActionSlotResolverSubsystem* ResolverSubsystem = ResolveResolverSubsystem();
    const UNiosCore_StatusEffectCatalog* Catalog = ResolverSubsystem ? ResolverSubsystem->GetStatusEffectCatalog() : nullptr;
    const FNiosStatusEffectCatalogEntry* Entry = Catalog ? Catalog->FindStatusEffect(EffectID) : nullptr;
    const UNiosCore_StatusEffectDataAsset* EffectData = Entry ? Entry->EffectData.Get() : nullptr;

    if (!EffectData)
    {
        return false;
    }

    return EffectData->DurationPolicy == ENiosStatusEffectDurationPolicy::Infinite;
}

const FNiosInventorySlot* UNiosCore_EquipmentComponent::FindInventorySlotByItemInstanceIndex(int32 ItemInstanceIndex) const
{
    if (ItemInstanceIndex == INDEX_NONE)
    {
        return nullptr;
    }

    const UNiosCore_InventoryComponent* ResolvedInventoryComponent = ResolveInventoryComponent();
    if (!ResolvedInventoryComponent)
    {
        return nullptr;
    }

    for (const FNiosInventorySlot& Slot : ResolvedInventoryComponent->InventorySlots)
    {
        if (!Slot.IsEmpty() && Slot.ItemInstanceIndex == ItemInstanceIndex)
        {
            return &Slot;
        }
    }

    return nullptr;
}

bool UNiosCore_EquipmentComponent::ResolveWeaponRow(FName ItemID, FNiosWeaponRow& OutWeaponRow) const
{
    if (ItemID.IsNone())
    {
        NIOS_EQUIP_LOGV(2, "ResolveWeaponRow failed Owner=%s Item=None", *GetNameSafe(GetOwner()));
        return false;
    }

    UActionSlotResolverSubsystem* ResolverSubsystem = ResolveResolverSubsystem();
    if (!ResolverSubsystem || !ResolverSubsystem->DatabaseSettings || ResolverSubsystem->DatabaseSettings->DT_Weapon.IsNull())
    {
        NIOS_EQUIP_LOG("ResolveWeaponRow failed Owner=%s Item=%s Reason=MissingResolverOrDT_Weapon",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString());
        return false;
    }

    UDataTable* WeaponTable = ResolverSubsystem->DatabaseSettings->DT_Weapon.LoadSynchronous();
    if (!WeaponTable)
    {
        NIOS_EQUIP_LOG("ResolveWeaponRow failed Owner=%s Item=%s Reason=WeaponTableLoadFailed",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString());
        return false;
    }

    const FNiosWeaponRow* WeaponRow = WeaponTable->FindRow<FNiosWeaponRow>(ItemID, TEXT("EquipmentComponent ResolveWeaponRow"));
    if (!WeaponRow)
    {
        NIOS_EQUIP_LOG("ResolveWeaponRow failed Owner=%s Item=%s Table=%s Reason=RowMissing",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString(),
            *GetNameSafe(WeaponTable));
        return false;
    }

    OutWeaponRow = *WeaponRow;
    NIOS_EQUIP_LOGV(2, "ResolveWeaponRow ok Owner=%s Item=%s WeaponType=%s Modifiers=%d Effects=%d",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        *UEnum::GetValueAsString(OutWeaponRow.WeaponType),
        OutWeaponRow.StatModifiers.Num(),
        OutWeaponRow.GrantedStatusEffectIDs.Num());
    return true;
}

bool UNiosCore_EquipmentComponent::ResolveArmorRow(FName ItemID, FNiosArmorRow& OutArmorRow) const
{
    if (ItemID.IsNone())
    {
        NIOS_EQUIP_LOGV(2, "ResolveArmorRow failed Owner=%s Item=None", *GetNameSafe(GetOwner()));
        return false;
    }

    UActionSlotResolverSubsystem* ResolverSubsystem = ResolveResolverSubsystem();
    if (!ResolverSubsystem || !ResolverSubsystem->DatabaseSettings || ResolverSubsystem->DatabaseSettings->DT_Armor.IsNull())
    {
        NIOS_EQUIP_LOG("ResolveArmorRow failed Owner=%s Item=%s Reason=MissingResolverOrDT_Armor",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString());
        return false;
    }

    UDataTable* ArmorTable = ResolverSubsystem->DatabaseSettings->DT_Armor.LoadSynchronous();
    if (!ArmorTable)
    {
        NIOS_EQUIP_LOG("ResolveArmorRow failed Owner=%s Item=%s Reason=ArmorTableLoadFailed",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString());
        return false;
    }

    const FNiosArmorRow* ArmorRow = ArmorTable->FindRow<FNiosArmorRow>(ItemID, TEXT("EquipmentComponent ResolveArmorRow"));
    if (!ArmorRow)
    {
        NIOS_EQUIP_LOG("ResolveArmorRow failed Owner=%s Item=%s Table=%s Reason=RowMissing",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString(),
            *GetNameSafe(ArmorTable));
        return false;
    }

    OutArmorRow = *ArmorRow;
    NIOS_EQUIP_LOGV(2, "ResolveArmorRow ok Owner=%s Item=%s ArmorSlot=%s Modifiers=%d Effects=%d",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        *UEnum::GetValueAsString(OutArmorRow.ArmorSlot),
        OutArmorRow.StatModifiers.Num(),
        OutArmorRow.GrantedStatusEffectIDs.Num());
    return true;
}

bool UNiosCore_EquipmentComponent::IsArtifactSlot(EEquipSlot Slot) const
{
    return Slot == EEquipSlot::Artifact1 ||
        Slot == EEquipSlot::Artifact2 ||
        Slot == EEquipSlot::Artifact3 ||
        Slot == EEquipSlot::Artifact4;
}

bool UNiosCore_EquipmentComponent::IsHandSlot(EEquipSlot Slot) const
{
    return Slot == EEquipSlot::LeftHand || Slot == EEquipSlot::RightHand;
}

bool UNiosCore_EquipmentComponent::IsArmorSlotCompatible(ENiosArmorSlot ArmorSlot, EEquipSlot TargetSlot) const
{
    switch (ArmorSlot)
    {
    case ENiosArmorSlot::Head:    return TargetSlot == EEquipSlot::Helmet;
    case ENiosArmorSlot::Chest:   return TargetSlot == EEquipSlot::Torso;
    case ENiosArmorSlot::Hands:   return TargetSlot == EEquipSlot::Hands;
    case ENiosArmorSlot::Legs:    return TargetSlot == EEquipSlot::Legs;
    case ENiosArmorSlot::Feet:    return TargetSlot == EEquipSlot::Feet;
    case ENiosArmorSlot::Back:    return TargetSlot == EEquipSlot::Cloak;
    case ENiosArmorSlot::Ring:    return TargetSlot == EEquipSlot::Ring1 || TargetSlot == EEquipSlot::Ring2;
    case ENiosArmorSlot::Trinket: return IsArtifactSlot(TargetSlot) || TargetSlot == EEquipSlot::Accessory;
    case ENiosArmorSlot::OffHand: return TargetSlot == EEquipSlot::LeftHand;
    default:                      return false;
    }
}

bool UNiosCore_EquipmentComponent::IsOffHandArmorSlot(ENiosArmorSlot ArmorSlot) const
{
    return ArmorSlot == ENiosArmorSlot::OffHand;
}

bool UNiosCore_EquipmentComponent::IsTwoHandedWeaponType(ENiosWeaponType WeaponType) const
{
    switch (WeaponType)
    {
    case ENiosWeaponType::Staff:
    case ENiosWeaponType::TwoHandedSword:
    case ENiosWeaponType::GreatWeapon:
    case ENiosWeaponType::Polearm:
    case ENiosWeaponType::Dual:
    case ENiosWeaponType::TwoHanded: // Legacy DataTable value.
        return true;

    default:
        return false;
    }
}

bool UNiosCore_EquipmentComponent::IsOffHandWeaponType(ENiosWeaponType WeaponType) const
{
    switch (WeaponType)
    {
    case ENiosWeaponType::Shield:
    case ENiosWeaponType::Orb:
    case ENiosWeaponType::Torch:
    case ENiosWeaponType::OffHand:
        return true;

    default:
        return false;
    }
}

bool UNiosCore_EquipmentComponent::IsWeaponSlotCompatible(ENiosWeaponType WeaponType, EEquipSlot TargetSlot) const
{
    if (WeaponType == ENiosWeaponType::Bow)
    {
        return TargetSlot == EEquipSlot::RangedWeapon;
    }

    // Two-handed weapons are equipped by targeting RightHand, then stored in both hands internally.
    // LeftHand is reserved for true off-hand utility items.
    if (IsTwoHandedWeaponType(WeaponType))
    {
        return TargetSlot == EEquipSlot::RightHand;
    }

    if (IsOffHandWeaponType(WeaponType))
    {
        return TargetSlot == EEquipSlot::LeftHand;
    }

    switch (WeaponType)
    {
    case ENiosWeaponType::Sword:
    case ENiosWeaponType::Axe:
    case ENiosWeaponType::Dagger:
        return TargetSlot == EEquipSlot::RightHand;

    default:
        return false;
    }
}

bool UNiosCore_EquipmentComponent::CanModifyEquipmentNow() const
{
    const AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return true;
    }

    if (!NiosEquipment_IsAliveForEquipment(OwnerActor))
    {
        NIOS_EQUIP_LOG("CanModifyEquipmentNow false Owner=%s Reason=OwnerDead", *GetNameSafe(OwnerActor));
        return false;
    }

    if (const APawn* PawnOwner = Cast<APawn>(OwnerActor))
    {
        if (const APlayerState* PS = PawnOwner->GetPlayerState())
        {
            if (!NiosEquipment_IsAliveForEquipment(PS))
            {
                NIOS_EQUIP_LOG("CanModifyEquipmentNow false Owner=%s PlayerState=%s Reason=PlayerStateDead",
                    *GetNameSafe(OwnerActor),
                    *GetNameSafe(PS));
                return false;
            }
        }
    }

    return true;
}

bool UNiosCore_EquipmentComponent::CanEquipItemToSlot(const FName ItemID, EEquipSlot TargetSlot) const
{
    NIOS_EQUIP_LOG("CanEquipItemToSlot begin Owner=%s Item=%s TargetSlot=%s Role=%s",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        *UEnum::GetValueAsString(TargetSlot),
        NiosEquipmentRoleToText(GetOwnerRole()));

    if (ItemID.IsNone())
    {
        NIOS_EQUIP_LOG("CanEquipItemToSlot false Item=None");
        return false;
    }

    if (TargetSlot == EEquipSlot::None)
    {
        NIOS_EQUIP_LOG("CanEquipItemToSlot false Item=%s Reason=TargetSlotNone", *ItemID.ToString());
        return false;
    }

    FNiosResolvedInventoryItemData ItemData;
    if (!ResolveItemData(ItemID, ItemData))
    {
        NIOS_EQUIP_LOG("CanEquipItemToSlot false Item=%s TargetSlot=%s Reason=ResolveItemData",
            *ItemID.ToString(),
            *UEnum::GetValueAsString(TargetSlot));
        return false;
    }

    if (ItemData.ItemType == ENiosItemType::Weapon)
    {
        FNiosWeaponRow WeaponRow;
        const bool bResolvedWeapon = ResolveWeaponRow(ItemID, WeaponRow);
        const bool bCompatible = bResolvedWeapon && IsWeaponSlotCompatible(WeaponRow.WeaponType, TargetSlot);
        NIOS_EQUIP_LOG("CanEquipItemToSlot weapon Item=%s TargetSlot=%s Resolved=%d WeaponType=%s Compatible=%d",
            *ItemID.ToString(),
            *UEnum::GetValueAsString(TargetSlot),
            bResolvedWeapon ? 1 : 0,
            bResolvedWeapon ? *UEnum::GetValueAsString(WeaponRow.WeaponType) : TEXT("None"),
            bCompatible ? 1 : 0);
        return bCompatible;
    }

    if (ItemData.ItemType == ENiosItemType::Armor)
    {
        FNiosArmorRow ArmorRow;
        const bool bResolvedArmor = ResolveArmorRow(ItemID, ArmorRow);
        const bool bCompatible = bResolvedArmor && IsArmorSlotCompatible(ArmorRow.ArmorSlot, TargetSlot);
        NIOS_EQUIP_LOG("CanEquipItemToSlot armor Item=%s TargetSlot=%s Resolved=%d ArmorSlot=%s Compatible=%d",
            *ItemID.ToString(),
            *UEnum::GetValueAsString(TargetSlot),
            bResolvedArmor ? 1 : 0,
            bResolvedArmor ? *UEnum::GetValueAsString(ArmorRow.ArmorSlot) : TEXT("None"),
            bCompatible ? 1 : 0);
        return bCompatible;
    }

    NIOS_EQUIP_LOG("CanEquipItemToSlot false Item=%s TargetSlot=%s Reason=UnsupportedItemType Type=%s",
        *ItemID.ToString(),
        *UEnum::GetValueAsString(TargetSlot),
        *UEnum::GetValueAsString(ItemData.ItemType));
    return false;
}

EEquipSlot UNiosCore_EquipmentComponent::FindDefaultSlotForItem(FName ItemID) const
{
    NIOS_EQUIP_LOG("FindDefaultSlotForItem begin Owner=%s Item=%s", *GetNameSafe(GetOwner()), *ItemID.ToString());

    FNiosResolvedInventoryItemData ItemData;
    if (!ResolveItemData(ItemID, ItemData))
    {
        NIOS_EQUIP_LOG("FindDefaultSlotForItem result Item=%s Slot=None Reason=ResolveItemData", *ItemID.ToString());
        return EEquipSlot::None;
    }

    if (ItemData.ItemType == ENiosItemType::Weapon)
    {
        FNiosWeaponRow WeaponRow;
        if (!ResolveWeaponRow(ItemID, WeaponRow))
        {
            NIOS_EQUIP_LOG("FindDefaultSlotForItem result Item=%s Slot=None Reason=ResolveWeaponRow", *ItemID.ToString());
            return EEquipSlot::None;
        }

        EEquipSlot Result = EEquipSlot::None;
        if (WeaponRow.WeaponType == ENiosWeaponType::Bow)
        {
            Result = EEquipSlot::RangedWeapon;
        }
        else if (IsOffHandWeaponType(WeaponRow.WeaponType))
        {
            Result = EEquipSlot::LeftHand;
        }
        else
        {
            // Main-hand and two-handed weapons are always equipped through RightHand.
            Result = EEquipSlot::RightHand;
        }

        NIOS_EQUIP_LOG("FindDefaultSlotForItem result Item=%s Type=Weapon WeaponType=%s Slot=%s",
            *ItemID.ToString(),
            *UEnum::GetValueAsString(WeaponRow.WeaponType),
            *UEnum::GetValueAsString(Result));
        return Result;
    }

    if (ItemData.ItemType == ENiosItemType::Armor)
    {
        FNiosArmorRow ArmorRow;
        if (!ResolveArmorRow(ItemID, ArmorRow))
        {
            NIOS_EQUIP_LOG("FindDefaultSlotForItem result Item=%s Slot=None Reason=ResolveArmorRow", *ItemID.ToString());
            return EEquipSlot::None;
        }

        EEquipSlot Result = EEquipSlot::None;
        switch (ArmorRow.ArmorSlot)
        {
        case ENiosArmorSlot::Head:    Result = EEquipSlot::Helmet; break;
        case ENiosArmorSlot::Chest:   Result = EEquipSlot::Torso; break;
        case ENiosArmorSlot::Hands:   Result = EEquipSlot::Hands; break;
        case ENiosArmorSlot::Legs:    Result = EEquipSlot::Legs; break;
        case ENiosArmorSlot::Feet:    Result = EEquipSlot::Feet; break;
        case ENiosArmorSlot::Back:    Result = EEquipSlot::Cloak; break;
        case ENiosArmorSlot::Ring:
            if (GetEquipmentSlot(EEquipSlot::Ring1).IsEmpty()) Result = EEquipSlot::Ring1;
            else if (GetEquipmentSlot(EEquipSlot::Ring2).IsEmpty()) Result = EEquipSlot::Ring2;
            else Result = EEquipSlot::Ring1;
            break;
        case ENiosArmorSlot::Trinket:
            if (GetEquipmentSlot(EEquipSlot::Artifact1).IsEmpty()) Result = EEquipSlot::Artifact1;
            else if (GetEquipmentSlot(EEquipSlot::Artifact2).IsEmpty()) Result = EEquipSlot::Artifact2;
            else if (GetEquipmentSlot(EEquipSlot::Artifact3).IsEmpty()) Result = EEquipSlot::Artifact3;
            else if (GetEquipmentSlot(EEquipSlot::Artifact4).IsEmpty()) Result = EEquipSlot::Artifact4;
            else Result = EEquipSlot::Artifact1;
            break;
        case ENiosArmorSlot::OffHand:
            Result = EEquipSlot::LeftHand;
            break;
        default:
            Result = EEquipSlot::None;
            break;
        }

        NIOS_EQUIP_LOG("FindDefaultSlotForItem result Item=%s Type=Armor ArmorSlot=%s Slot=%s",
            *ItemID.ToString(),
            *UEnum::GetValueAsString(ArmorRow.ArmorSlot),
            *UEnum::GetValueAsString(Result));
        return Result;
    }

    NIOS_EQUIP_LOG("FindDefaultSlotForItem result Item=%s Slot=None Reason=UnsupportedItemType Type=%s",
        *ItemID.ToString(),
        *UEnum::GetValueAsString(ItemData.ItemType));
    return EEquipSlot::None;
}

bool UNiosCore_EquipmentComponent::EquipItem(const FName ItemID)
{
    NIOS_EQUIP_LOG("EquipItem begin Owner=%s Item=%s Role=%s",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        NiosEquipmentRoleToText(GetOwnerRole()));
    if (!CanModifyEquipmentNow())
    {
        return false;
    }
    const EEquipSlot TargetSlot = FindDefaultSlotForItem(ItemID);
    const bool bResult = EquipItemToSlot(ItemID, TargetSlot);
    NIOS_EQUIP_LOG("EquipItem result Owner=%s Item=%s TargetSlot=%s Result=%d",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        *UEnum::GetValueAsString(TargetSlot),
        bResult ? 1 : 0);
    return bResult;
}

bool UNiosCore_EquipmentComponent::EquipItemToSlot(const FName ItemID, EEquipSlot TargetSlot)
{
    NIOS_EQUIP_LOG("EquipItemToSlot begin Owner=%s Item=%s TargetSlot=%s Role=%s",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        *UEnum::GetValueAsString(TargetSlot),
        NiosEquipmentRoleToText(GetOwnerRole()));
    if (!CanModifyEquipmentNow())
    {
        return false;
    }
    // Compatibility path: item is equipped by template ItemID only, without inventory ownership reference.
    const bool bInstancePath = EquipInventoryItemToSlot(INDEX_NONE, TargetSlot);
    const bool bDataPath = bInstancePath ? false : EquipInventoryItemToSlotFromData(ItemID, INDEX_NONE, TargetSlot);
    const bool bResult = bInstancePath || bDataPath;
    NIOS_EQUIP_LOG("EquipItemToSlot result Owner=%s Item=%s TargetSlot=%s InstancePath=%d DataPath=%d Result=%d",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        *UEnum::GetValueAsString(TargetSlot),
        bInstancePath ? 1 : 0,
        bDataPath ? 1 : 0,
        bResult ? 1 : 0);
    return bResult;
}

bool UNiosCore_EquipmentComponent::EquipInventoryItem(int32 ItemInstanceIndex)
{
    NIOS_EQUIP_LOG("EquipInventoryItem begin Owner=%s Instance=%d", *GetNameSafe(GetOwner()), ItemInstanceIndex);
    if (!CanModifyEquipmentNow())
    {
        return false;
    }
    const int32 InventorySlotIndex = ResolveInventorySlotIndexForItemInstance(ItemInstanceIndex);
    const bool bResult = AutoEquipItemFromInventorySlot(InventorySlotIndex);
    NIOS_EQUIP_LOG("EquipInventoryItem result Owner=%s Instance=%d InventorySlot=%d Result=%d",
        *GetNameSafe(GetOwner()),
        ItemInstanceIndex,
        InventorySlotIndex,
        bResult ? 1 : 0);
    return bResult;
}

bool UNiosCore_EquipmentComponent::AutoEquipItemFromInventorySlot(int32 InventorySlotIndex)
{   
    NIOS_EQUIP_LOG("AutoEquipItemFromInventorySlot SlotIndex=%d Role=%s",
        InventorySlotIndex,
        NiosEquipmentRoleToText(GetOwnerRole()));

    if (InventorySlotIndex == INDEX_NONE)
    {
        return false;
    }

    if (GetOwnerRole() != ROLE_Authority)
    {
        Server_AutoEquipItemFromInventorySlot(InventorySlotIndex);
        return true;
    }

    if (!CanModifyEquipmentNow())
    {
        return false;
    }
    const UNiosCore_InventoryComponent* ResolvedInventoryComponent = ResolveInventoryComponent();
    if (!ResolvedInventoryComponent || !ResolvedInventoryComponent->InventorySlots.IsValidIndex(InventorySlotIndex))
    {
        NIOS_EQUIP_LOG("AutoEquip failed Owner=%s InventorySlot=%d Inventory=%s Reason=InvalidSlotOrNoInventory",
            *GetNameSafe(GetOwner()),
            InventorySlotIndex,
            *GetNameSafe(ResolvedInventoryComponent));
        return false;
    }

    const FNiosInventorySlot& InventorySlot = ResolvedInventoryComponent->InventorySlots[InventorySlotIndex];
    if (InventorySlot.IsEmpty())
    {
        NIOS_EQUIP_LOG("AutoEquip failed Owner=%s InventorySlot=%d Reason=InventorySlotEmpty",
            *GetNameSafe(GetOwner()),
            InventorySlotIndex);
        return false;
    }

    const EEquipSlot TargetSlot = FindDefaultSlotForItem(InventorySlot.ItemID);
    NIOS_EQUIP_LOG("AutoEquip resolved Item=%s Instance=%d TargetSlot=%s",
        *InventorySlot.ItemID.ToString(),
        InventorySlot.ItemInstanceIndex,
        *UEnum::GetValueAsString(TargetSlot));
    return EquipInventoryItemToSlotFromData(
        InventorySlot.ItemID,
        InventorySlot.ItemInstanceIndex,
        TargetSlot);
}

void UNiosCore_EquipmentComponent::Server_AutoEquipItemFromInventorySlot_Implementation(int32 InventorySlotIndex)
{
    AutoEquipItemFromInventorySlot(InventorySlotIndex);
}

bool UNiosCore_EquipmentComponent::EquipItemFromInventorySlotRequest(const FNiosEquipmentSlotRequest& Request)
{
    if (!Request.IsValid())
    {
        return false;
    }

    return EquipItemFromInventorySlot(Request.InventorySlotIndex, Request.TargetSlot);
}

bool UNiosCore_EquipmentComponent::EquipItemFromInventorySlot(int32 InventorySlotIndex, EEquipSlot TargetSlot)
{
    NIOS_EQUIP_LOG("EquipItemFromInventorySlot begin Owner=%s InventorySlot=%d TargetSlot=%s Role=%s",
        *GetNameSafe(GetOwner()),
        InventorySlotIndex,
        *UEnum::GetValueAsString(TargetSlot),
        NiosEquipmentRoleToText(GetOwnerRole()));

    if (InventorySlotIndex == INDEX_NONE || TargetSlot == EEquipSlot::None)
    {
        return false;
    }

    if (GetOwnerRole() != ROLE_Authority)
    {
        Server_EquipItemFromInventorySlot(InventorySlotIndex, TargetSlot);
        return true;
    }

    if (!CanModifyEquipmentNow())
    {
        return false;
    }
    const UNiosCore_InventoryComponent* ResolvedInventoryComponent = ResolveInventoryComponent();
    if (!ResolvedInventoryComponent || !ResolvedInventoryComponent->InventorySlots.IsValidIndex(InventorySlotIndex))
    {
        NIOS_EQUIP_LOG("EquipItemFromInventorySlot failed Owner=%s InventorySlot=%d Inventory=%s Reason=InvalidSlotOrNoInventory",
            *GetNameSafe(GetOwner()),
            InventorySlotIndex,
            *GetNameSafe(ResolvedInventoryComponent));
        return false;
    }

    const FNiosInventorySlot& InventorySlot = ResolvedInventoryComponent->InventorySlots[InventorySlotIndex];
    if (InventorySlot.IsEmpty())
    {
        NIOS_EQUIP_LOG("EquipItemFromInventorySlot failed Owner=%s InventorySlot=%d Reason=InventorySlotEmpty",
            *GetNameSafe(GetOwner()),
            InventorySlotIndex);
        return false;
    }

    NIOS_EQUIP_LOG("EquipItemFromInventorySlot resolved Owner=%s InventorySlot=%d Item=%s Instance=%d TargetSlot=%s",
        *GetNameSafe(GetOwner()),
        InventorySlotIndex,
        *InventorySlot.ItemID.ToString(),
        InventorySlot.ItemInstanceIndex,
        *UEnum::GetValueAsString(TargetSlot));

    return EquipInventoryItemToSlotFromData(
        InventorySlot.ItemID,
        InventorySlot.ItemInstanceIndex,
        TargetSlot
    );
}

void UNiosCore_EquipmentComponent::Server_EquipItemFromInventorySlot_Implementation(int32 InventorySlotIndex, EEquipSlot TargetSlot)
{
    EquipItemFromInventorySlot(InventorySlotIndex, TargetSlot);
}

bool UNiosCore_EquipmentComponent::EquipInventoryItemToSlot(int32 ItemInstanceIndex, EEquipSlot TargetSlot)
{
    NIOS_EQUIP_LOG("EquipInventoryItemToSlot Instance=%d TargetSlot=%s",
        ItemInstanceIndex,
        *UEnum::GetValueAsString(TargetSlot));
    if (!CanModifyEquipmentNow())
    {
        return false;
    }
    const FNiosInventorySlot* InventorySlot = FindInventorySlotByItemInstanceIndex(ItemInstanceIndex);
    if (!InventorySlot)
    {
        NIOS_EQUIP_LOG("FAIL: Inventory slot not found for Instance=%d", ItemInstanceIndex);

        return false;
    }
    return EquipInventoryItemToSlotFromData(InventorySlot->ItemID, ItemInstanceIndex, TargetSlot);
}

bool UNiosCore_EquipmentComponent::EquipInventoryItemToSlotFromData(const FName ItemID, int32 ItemInstanceIndex, EEquipSlot TargetSlot)
{
    NIOS_EQUIP_LOG("EquipFromData Item=%s Instance=%d TargetSlot=%s",
        *ItemID.ToString(),
        ItemInstanceIndex,
        *UEnum::GetValueAsString(TargetSlot));
    if (!CanModifyEquipmentNow())
    {
        return false;
    }
    if (!CanEquipItemToSlot(ItemID, TargetSlot))
    {
        NIOS_EQUIP_LOG("FAIL: CanEquipItemToSlot false Item=%s TargetSlot=%s",
            *ItemID.ToString(),
            *UEnum::GetValueAsString(TargetSlot));
        return false;
    }

    FNiosResolvedInventoryItemData ItemData;
    if (!ResolveItemData(ItemID, ItemData))
    {
        NIOS_EQUIP_LOG("EquipFromData failed Owner=%s Item=%s Instance=%d TargetSlot=%s Reason=ResolveItemDataAfterCanEquip",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString(),
            ItemInstanceIndex,
            *UEnum::GetValueAsString(TargetSlot));
        return false;
    }

    FNiosWeaponRow WeaponRow;
    const bool bIsWeapon = ItemData.ItemType == ENiosItemType::Weapon && ResolveWeaponRow(ItemID, WeaponRow);

    FNiosArmorRow ArmorRow;
    const bool bIsArmor = ItemData.ItemType == ENiosItemType::Armor && ResolveArmorRow(ItemID, ArmorRow);

    // Same item instance cannot live in two unrelated equipment slots.
    TArray<EEquipSlot> SlotsToClear;
    for (const FReplicatedEquipmentSlot& EquippedSlot : EquippedItems)
    {
        if (ItemInstanceIndex != INDEX_NONE && EquippedSlot.ItemInstanceIndex == ItemInstanceIndex && EquippedSlot.Slot != TargetSlot)
        {
            SlotsToClear.AddUnique(EquippedSlot.Slot);
        }
    }

    for (EEquipSlot SlotToClear : SlotsToClear)
    {
        ClearSlotInternal(SlotToClear);
    }

    if (bIsWeapon && IsTwoHandedWeaponType(WeaponRow.WeaponType))
    {
        FReplicatedEquipmentSlot EquipLeft;
        EquipLeft.Slot = EEquipSlot::LeftHand;
        EquipLeft.ItemID = ItemID;
        EquipLeft.ItemInstanceIndex = ItemInstanceIndex;

        FReplicatedEquipmentSlot EquipRight;
        EquipRight.Slot = EEquipSlot::RightHand;
        EquipRight.ItemID = ItemID;
        EquipRight.ItemInstanceIndex = ItemInstanceIndex;

        SetSlot(EEquipSlot::LeftHand, EquipLeft);
        SetSlot(EEquipSlot::RightHand, EquipRight);
        NIOS_EQUIP_LOG("EquipFromData success Owner=%s Item=%s Instance=%d TargetSlot=%s Mode=TwoHanded",
            *GetNameSafe(GetOwner()),
            *ItemID.ToString(),
            ItemInstanceIndex,
            *UEnum::GetValueAsString(TargetSlot));
        return true;
    }

    const bool bEquipsIntoHand =
        (bIsWeapon && IsHandSlot(TargetSlot)) ||
        (bIsArmor && IsOffHandArmorSlot(ArmorRow.ArmorSlot) && TargetSlot == EEquipSlot::LeftHand);

    if (bEquipsIntoHand)
    {
        const FReplicatedEquipmentSlot LeftHand = GetEquipmentSlot(EEquipSlot::LeftHand);
        const FReplicatedEquipmentSlot RightHand = GetEquipmentSlot(EEquipSlot::RightHand);

        const bool bHadTwoHanded = !LeftHand.IsEmpty() && !RightHand.IsEmpty() && LeftHand.ItemInstanceIndex == RightHand.ItemInstanceIndex;
        if (bHadTwoHanded)
        {
            ClearSlotInternal(EEquipSlot::LeftHand);
            ClearSlotInternal(EEquipSlot::RightHand);
        }
    }

    FReplicatedEquipmentSlot Equip;
    Equip.Slot = TargetSlot;
    Equip.ItemID = ItemID;
    Equip.ItemInstanceIndex = ItemInstanceIndex;
    SetSlot(TargetSlot, Equip);

    NIOS_EQUIP_LOG("EquipFromData success Owner=%s Item=%s Instance=%d TargetSlot=%s",
        *GetNameSafe(GetOwner()),
        *ItemID.ToString(),
        ItemInstanceIndex,
        *UEnum::GetValueAsString(TargetSlot));
    return true;
}

bool UNiosCore_EquipmentComponent::UnequipItem(const FName ItemID)
{
    if (!CanModifyEquipmentNow())
    {
        return false;
    }

    if (ItemID.IsNone())
    {
        return false;
    }

    TArray<EEquipSlot> SlotsToClear;
    for (const FReplicatedEquipmentSlot& EquippedSlot : EquippedItems)
    {
        if (EquippedSlot.ItemID == ItemID)
        {
            SlotsToClear.AddUnique(EquippedSlot.Slot);
        }
    }

    for (EEquipSlot SlotToClear : SlotsToClear)
    {
        ClearSlotInternal(SlotToClear);
    }

    return SlotsToClear.Num() > 0;
}

bool UNiosCore_EquipmentComponent::UnequipItemInstance(
    int32 ItemInstanceIndex)
{
    if (!CanModifyEquipmentNow())
    {
        return false;
    }

    if (ItemInstanceIndex == INDEX_NONE)
    {
        return false;
    }

    TArray<EEquipSlot> SlotsToClear;

    for (const FReplicatedEquipmentSlot& EquippedSlot : EquippedItems)
    {
        if (EquippedSlot.ItemInstanceIndex == ItemInstanceIndex)
        {
            SlotsToClear.AddUnique(EquippedSlot.Slot);
        }
    }

    for (EEquipSlot SlotToClear : SlotsToClear)
    {
        ClearSlotInternal(SlotToClear);
    }

    return SlotsToClear.Num() > 0;
}

bool UNiosCore_EquipmentComponent::IsItemInstanceEquipped(int32 ItemInstanceIndex) const
{
    if (ItemInstanceIndex == INDEX_NONE)
    {
        return false;
    }

    for (const FReplicatedEquipmentSlot& EquippedSlot : EquippedItems)
    {
        if (!EquippedSlot.IsEmpty() && EquippedSlot.ItemInstanceIndex == ItemInstanceIndex)
        {
            return true;
        }
    }

    return false;
}

bool UNiosCore_EquipmentComponent::IsInventorySlotEquipped(int32 InventorySlotIndex) const
{
    const UNiosCore_InventoryComponent* ResolvedInventoryComponent = ResolveInventoryComponent();
    if (!ResolvedInventoryComponent || !ResolvedInventoryComponent->InventorySlots.IsValidIndex(InventorySlotIndex))
    {
        return false;
    }

    const FNiosInventorySlot& InventorySlot = ResolvedInventoryComponent->InventorySlots[InventorySlotIndex];
    return !InventorySlot.IsEmpty() && IsItemInstanceEquipped(InventorySlot.ItemInstanceIndex);
}

int32 UNiosCore_EquipmentComponent::ResolveInventorySlotIndexForItemInstance(int32 ItemInstanceIndex) const
{
    const UNiosCore_InventoryComponent* ResolvedInventoryComponent = ResolveInventoryComponent();
    return ResolvedInventoryComponent ? ResolvedInventoryComponent->FindSlotIndexByItemInstanceIndex(ItemInstanceIndex) : INDEX_NONE;
}

bool UNiosCore_EquipmentComponent::UnequipSlot(EEquipSlot Slot)
{
    if (Slot == EEquipSlot::None)
    {
        return false;
    }

    if (GetOwnerRole() != ROLE_Authority)
    {
        Server_UnequipSlot(Slot);
        return true;
    }

    if (!CanModifyEquipmentNow())
    {
        return false;
    }

    FReplicatedEquipmentSlot CurrentSlot =
        GetEquipmentSlot(Slot);

    const FReplicatedEquipmentSlot LeftHand =
        GetEquipmentSlot(EEquipSlot::LeftHand);

    const FReplicatedEquipmentSlot RightHand =
        GetEquipmentSlot(EEquipSlot::RightHand);

    // UI can display a two-handed weapon in the opposite hand as a mirror, especially when
    // older/transition data only has one real hand slot populated. Treat unequip from the
    // mirrored hand exactly like unequip from the real hand.
    if (CurrentSlot.IsEmpty() && IsHandSlot(Slot))
    {
        const EEquipSlot OtherHandSlot =
            Slot == EEquipSlot::LeftHand ? EEquipSlot::RightHand : EEquipSlot::LeftHand;
        const FReplicatedEquipmentSlot OtherHand =
            Slot == EEquipSlot::LeftHand ? RightHand : LeftHand;

        FNiosWeaponRow OtherWeaponRow;
        if (!OtherHand.IsEmpty() &&
            ResolveWeaponRow(OtherHand.ItemID, OtherWeaponRow) &&
            IsTwoHandedWeaponType(OtherWeaponRow.WeaponType))
        {
            CurrentSlot = OtherHand;
            Slot = OtherHandSlot;
        }
    }

    if (CurrentSlot.IsEmpty())
    {
        return false;
    }

    const bool bTwoHanded =
        !LeftHand.IsEmpty() &&
        !RightHand.IsEmpty() &&
        LeftHand.ItemInstanceIndex == RightHand.ItemInstanceIndex;

    if (bTwoHanded && IsHandSlot(Slot))
    {
        ClearSlotInternal(EEquipSlot::LeftHand);
        ClearSlotInternal(EEquipSlot::RightHand);

        return true;
    }

    ClearSlotInternal(Slot);

    return true;
}

void UNiosCore_EquipmentComponent::Server_UnequipSlot_Implementation(EEquipSlot Slot)
{
    UnequipSlot(Slot);
}

FReplicatedEquipmentSlot UNiosCore_EquipmentComponent::GetEquipmentSlot(EEquipSlot Slot) const
{
    const int32 Index = FindSlotIndex(Slot);
    if (Index != INDEX_NONE)
    {
        return EquippedItems[Index];
    }

    FReplicatedEquipmentSlot Empty;
    Empty.Slot = Slot;
    return Empty;
}

bool UNiosCore_EquipmentComponent::GetEquippedSlotByItemInstance(
    int32 ItemInstanceIndex,
    FReplicatedEquipmentSlot& OutEquipmentSlot
) const
{
    OutEquipmentSlot = FReplicatedEquipmentSlot();

    if (ItemInstanceIndex == INDEX_NONE)
    {
        return false;
    }

    for (const FReplicatedEquipmentSlot& EquippedSlot : EquippedItems)
    {
        if (!EquippedSlot.IsEmpty() &&
            EquippedSlot.ItemInstanceIndex == ItemInstanceIndex)
        {
            OutEquipmentSlot = EquippedSlot;
            return true;
        }
    }

    return false;
}


void UNiosCore_EquipmentComponent::ValidateEquipmentAgainstInventory()
{
    const UNiosCore_InventoryComponent* ResolvedInventoryComponent = ResolveInventoryComponent();
    if (!ResolvedInventoryComponent)
    {
        NIOS_EQUIP_LOGV(2, "ValidateEquipmentAgainstInventory skipped Owner=%s Reason=NoInventory", *GetNameSafe(GetOwner()));
        return;
    }

    NIOS_EQUIP_LOGV(2, "ValidateEquipmentAgainstInventory Owner=%s EquippedSlots=%d InventorySlots=%d",
        *GetNameSafe(GetOwner()),
        EquippedItems.Num(),
        ResolvedInventoryComponent->InventorySlots.Num());

    TArray<EEquipSlot> SlotsToClear;

    for (const FReplicatedEquipmentSlot& EquippedSlot : EquippedItems)
    {
        if (EquippedSlot.IsEmpty())
        {
            continue;
        }

        if (EquippedSlot.ItemInstanceIndex == INDEX_NONE)
        {
            continue;
        }

        const int32 InventorySlotIndex = ResolvedInventoryComponent->FindSlotIndexByItemInstanceIndex(EquippedSlot.ItemInstanceIndex);
        if (InventorySlotIndex == INDEX_NONE ||
            !ResolvedInventoryComponent->InventorySlots.IsValidIndex(InventorySlotIndex) ||
            ResolvedInventoryComponent->InventorySlots[InventorySlotIndex].IsEmpty() ||
            ResolvedInventoryComponent->InventorySlots[InventorySlotIndex].ItemID != EquippedSlot.ItemID)
        {
            const FName InventoryItemID = ResolvedInventoryComponent->InventorySlots.IsValidIndex(InventorySlotIndex)
                ? ResolvedInventoryComponent->InventorySlots[InventorySlotIndex].ItemID
                : NAME_None;
            NIOS_EQUIP_LOG("ValidateEquipment clearing Owner=%s EquipSlot=%s Item=%s Instance=%d InventorySlot=%d InventoryItem=%s Reason=MissingOrMismatch",
                *GetNameSafe(GetOwner()),
                *UEnum::GetValueAsString(EquippedSlot.Slot),
                *EquippedSlot.ItemID.ToString(),
                EquippedSlot.ItemInstanceIndex,
                InventorySlotIndex,
                *InventoryItemID.ToString());
            SlotsToClear.AddUnique(EquippedSlot.Slot);
            continue;
        }

        bool bSlotCompatible = CanEquipItemToSlot(EquippedSlot.ItemID, EquippedSlot.Slot);

        // Two-handed weapons are commanded through RightHand but mirrored into LeftHand internally.
        // Keep that mirrored LeftHand slot valid during inventory validation.
        if (!bSlotCompatible && EquippedSlot.Slot == EEquipSlot::LeftHand)
        {
            const FReplicatedEquipmentSlot RightHand = GetEquipmentSlot(EEquipSlot::RightHand);
            FNiosWeaponRow WeaponRow;
            if (!RightHand.IsEmpty() &&
                RightHand.ItemInstanceIndex == EquippedSlot.ItemInstanceIndex &&
                RightHand.ItemID == EquippedSlot.ItemID &&
                ResolveWeaponRow(EquippedSlot.ItemID, WeaponRow) &&
                IsTwoHandedWeaponType(WeaponRow.WeaponType))
            {
                bSlotCompatible = true;
            }
        }

        if (!bSlotCompatible)
        {
            NIOS_EQUIP_LOG("ValidateEquipment clearing Owner=%s EquipSlot=%s Item=%s Instance=%d Reason=SlotIncompatible",
                *GetNameSafe(GetOwner()),
                *UEnum::GetValueAsString(EquippedSlot.Slot),
                *EquippedSlot.ItemID.ToString(),
                EquippedSlot.ItemInstanceIndex);
            SlotsToClear.AddUnique(EquippedSlot.Slot);
        }
    }

    for (EEquipSlot SlotToClear : SlotsToClear)
    {
        ClearSlotInternal(SlotToClear);
    }
}

TArray<FReplicatedEquipmentSlot> UNiosCore_EquipmentComponent::GetEquippedItems() const
{
    TArray<FReplicatedEquipmentSlot> Result;

    for (const FReplicatedEquipmentSlot& EquippedSlot : EquippedItems)
    {
        if (!EquippedSlot.IsEmpty())
        {
            Result.Add(EquippedSlot);
        }
    }

    return Result;
}

bool UNiosCore_EquipmentComponent::GetWeaponRowForItem(
    FName ItemID,
    FNiosWeaponRow& OutWeaponRow
) const
{
    return ResolveWeaponRow(ItemID, OutWeaponRow);
}

bool UNiosCore_EquipmentComponent::GetArmorRowForItem(
    FName ItemID,
    FNiosArmorRow& OutArmorRow
) const
{
    return ResolveArmorRow(ItemID, OutArmorRow);
}

void UNiosCore_EquipmentComponent::BroadcastEquipmentVisualSlotChanged(EEquipSlot Slot)
{
    if (Slot == EEquipSlot::None)
    {
        return;
    }

    OnEquipmentVisualSlotChanged.Broadcast(Slot, GetEquipmentSlot(Slot));
    RequestEquipmentMeshForSlot(Slot);
}


void UNiosCore_EquipmentComponent::RequestEquipmentMeshForSlot(EEquipSlot Slot)
{
    if (Slot == EEquipSlot::None)
    {
        return;
    }

    const FReplicatedEquipmentSlot EquipmentSlot = GetEquipmentSlot(Slot);
    if (EquipmentSlot.IsEmpty())
    {
        OnEquipmentMeshReady.Broadcast(Slot, EquipmentSlot, nullptr, nullptr);
        return;
    }

    FNiosResolvedInventoryItemData ItemData;
    if (!ResolveItemData(EquipmentSlot.ItemID, ItemData))
    {
        OnEquipmentMeshReady.Broadcast(Slot, EquipmentSlot, nullptr, nullptr);
        return;
    }

    if (ItemData.ItemType == ENiosItemType::Weapon)
    {
        FNiosWeaponRow WeaponRow;
        if (!ResolveWeaponRow(EquipmentSlot.ItemID, WeaponRow) || WeaponRow.Mesh.IsNull())
        {
            OnEquipmentMeshReady.Broadcast(Slot, EquipmentSlot, nullptr, nullptr);
            return;
        }

        const TSoftObjectPtr<UStaticMesh> MeshPtr = WeaponRow.Mesh;
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            MeshPtr.ToSoftObjectPath(),
            FStreamableDelegate::CreateWeakLambda(this, [this, Slot, EquipmentSlot, MeshPtr]()
            {
                const FReplicatedEquipmentSlot CurrentSlot = GetEquipmentSlot(Slot);
                if (CurrentSlot.ItemID != EquipmentSlot.ItemID ||
                    CurrentSlot.ItemInstanceIndex != EquipmentSlot.ItemInstanceIndex)
                {
                    return;
                }

                OnEquipmentMeshReady.Broadcast(Slot, EquipmentSlot, MeshPtr.Get(), nullptr);
            })
        );

        return;
    }

    if (ItemData.ItemType == ENiosItemType::Armor)
    {
        FNiosArmorRow ArmorRow;
        if (!ResolveArmorRow(EquipmentSlot.ItemID, ArmorRow) || ArmorRow.Mesh.IsNull())
        {
            OnEquipmentMeshReady.Broadcast(Slot, EquipmentSlot, nullptr, nullptr);
            return;
        }

        const TSoftObjectPtr<USkeletalMesh> MeshPtr = ArmorRow.Mesh;
        UAssetManager::GetStreamableManager().RequestAsyncLoad(
            MeshPtr.ToSoftObjectPath(),
            FStreamableDelegate::CreateWeakLambda(this, [this, Slot, EquipmentSlot, MeshPtr]()
            {
                const FReplicatedEquipmentSlot CurrentSlot = GetEquipmentSlot(Slot);
                if (CurrentSlot.ItemID != EquipmentSlot.ItemID ||
                    CurrentSlot.ItemInstanceIndex != EquipmentSlot.ItemInstanceIndex)
                {
                    return;
                }

                OnEquipmentMeshReady.Broadcast(Slot, EquipmentSlot, nullptr, MeshPtr.Get());
            })
        );

        return;
    }

    OnEquipmentMeshReady.Broadcast(Slot, EquipmentSlot, nullptr, nullptr);
}

void UNiosCore_EquipmentComponent::RequestAllEquipmentMeshes()
{
    RequestEquipmentMeshForSlot(EEquipSlot::Helmet);
    RequestEquipmentMeshForSlot(EEquipSlot::Torso);
    RequestEquipmentMeshForSlot(EEquipSlot::Legs);
    RequestEquipmentMeshForSlot(EEquipSlot::Hands);
    RequestEquipmentMeshForSlot(EEquipSlot::Feet);
    RequestEquipmentMeshForSlot(EEquipSlot::Accessory);
    RequestEquipmentMeshForSlot(EEquipSlot::Ring1);
    RequestEquipmentMeshForSlot(EEquipSlot::Ring2);
    RequestEquipmentMeshForSlot(EEquipSlot::Artifact1);
    RequestEquipmentMeshForSlot(EEquipSlot::Artifact2);
    RequestEquipmentMeshForSlot(EEquipSlot::Artifact3);
    RequestEquipmentMeshForSlot(EEquipSlot::Artifact4);
    RequestEquipmentMeshForSlot(EEquipSlot::LeftHand);
    RequestEquipmentMeshForSlot(EEquipSlot::RightHand);
    RequestEquipmentMeshForSlot(EEquipSlot::RangedWeapon);
    RequestEquipmentMeshForSlot(EEquipSlot::Cloak);
    RequestEquipmentMeshForSlot(EEquipSlot::Belt);
}

void UNiosCore_EquipmentComponent::BroadcastAllVisualSlotsChanged()
{
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Helmet);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Torso);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Legs);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Hands);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Feet);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Accessory);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Ring1);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Ring2);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Artifact1);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Artifact2);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Artifact3);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Artifact4);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::LeftHand);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::RightHand);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::RangedWeapon);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Cloak);
    BroadcastEquipmentVisualSlotChanged(EEquipSlot::Belt);
}

void UNiosCore_EquipmentComponent::BroadcastEquipmentVisualsChanged()
{
    OnEquipmentVisualsChanged.Broadcast(GetEquippedItems());
    BroadcastAllVisualSlotsChanged();
}

void UNiosCore_EquipmentComponent::BroadcastAllSlotsChanged()
{
    OnEquipmentChanged.Broadcast(EEquipSlot::Helmet);
    OnEquipmentChanged.Broadcast(EEquipSlot::Torso);
    OnEquipmentChanged.Broadcast(EEquipSlot::Legs);
    OnEquipmentChanged.Broadcast(EEquipSlot::Hands);
    OnEquipmentChanged.Broadcast(EEquipSlot::Feet);
    OnEquipmentChanged.Broadcast(EEquipSlot::Accessory);
    OnEquipmentChanged.Broadcast(EEquipSlot::Ring1);
    OnEquipmentChanged.Broadcast(EEquipSlot::Ring2);
    OnEquipmentChanged.Broadcast(EEquipSlot::Artifact1);
    OnEquipmentChanged.Broadcast(EEquipSlot::Artifact2);
    OnEquipmentChanged.Broadcast(EEquipSlot::Artifact3);
    OnEquipmentChanged.Broadcast(EEquipSlot::Artifact4);
    OnEquipmentChanged.Broadcast(EEquipSlot::LeftHand);
    OnEquipmentChanged.Broadcast(EEquipSlot::RightHand);
    OnEquipmentChanged.Broadcast(EEquipSlot::RangedWeapon);
    OnEquipmentChanged.Broadcast(EEquipSlot::Cloak);
    OnEquipmentChanged.Broadcast(EEquipSlot::Belt);
}

void UNiosCore_EquipmentComponent::OnRep_EquipmentChanged()
{
    BroadcastAllSlotsChanged();
    BroadcastEquipmentVisualsChanged();
    SyncWeaponVisualStateWithEquipment();
    BroadcastWeaponVisualStateChanged(true);
}


EEquipSlot UNiosCore_EquipmentComponent::ResolveDefaultActiveWeaponSlot() const
{
    const FReplicatedEquipmentSlot RightHand = GetEquipmentSlot(EEquipSlot::RightHand);
    if (!RightHand.IsEmpty())
    {
        return EEquipSlot::RightHand;
    }

    const FReplicatedEquipmentSlot LeftHand = GetEquipmentSlot(EEquipSlot::LeftHand);
    if (!LeftHand.IsEmpty())
    {
        return EEquipSlot::LeftHand;
    }

    const FReplicatedEquipmentSlot Ranged = GetEquipmentSlot(EEquipSlot::RangedWeapon);
    if (!Ranged.IsEmpty())
    {
        return EEquipSlot::RangedWeapon;
    }

    return EEquipSlot::None;
}

FReplicatedEquipmentSlot UNiosCore_EquipmentComponent::GetActiveWeaponEquipmentSlot() const
{
    if (WeaponVisualState.ActiveWeaponSlot != EEquipSlot::None)
    {
        return GetEquipmentSlot(WeaponVisualState.ActiveWeaponSlot);
    }

    const EEquipSlot FallbackSlot = ResolveDefaultActiveWeaponSlot();
    return GetEquipmentSlot(FallbackSlot);
}

void UNiosCore_EquipmentComponent::SyncWeaponVisualStateWithEquipment()
{
    const FReplicatedEquipmentSlot ActiveSlotData = GetActiveWeaponEquipmentSlot();

    if (ActiveSlotData.IsEmpty())
    {
        if (WeaponVisualState.ReadyState != ENiosWeaponReadyState::None ||
            WeaponVisualState.SocketState != ENiosWeaponVisualSocketState::None ||
            WeaponVisualState.ActiveWeaponSlot != EEquipSlot::None ||
            !WeaponVisualState.ItemID.IsNone() ||
            WeaponVisualState.ItemInstanceIndex != INDEX_NONE)
        {
            FNiosWeaponVisualState Cleared;
            ApplyWeaponVisualState(Cleared, true);
        }
        return;
    }

    if (WeaponVisualState.ActiveWeaponSlot == EEquipSlot::None ||
        WeaponVisualState.ItemID != ActiveSlotData.ItemID ||
        WeaponVisualState.ItemInstanceIndex != ActiveSlotData.ItemInstanceIndex)
    {
        FNiosWeaponVisualState NewState = WeaponVisualState;
        NewState.ActiveWeaponSlot = ActiveSlotData.Slot;
        NewState.ItemID = ActiveSlotData.ItemID;
        NewState.ItemInstanceIndex = ActiveSlotData.ItemInstanceIndex;

        if (NewState.ReadyState == ENiosWeaponReadyState::None)
        {
            NewState.ReadyState = ENiosWeaponReadyState::Sheathed;
        }

        if (NewState.SocketState == ENiosWeaponVisualSocketState::None)
        {
            NewState.SocketState = ActiveSlotData.Slot == EEquipSlot::RangedWeapon
                ? ENiosWeaponVisualSocketState::RangedBack
                : ENiosWeaponVisualSocketState::Back;
        }

        ApplyWeaponVisualState(NewState, true);
    }
}

void UNiosCore_EquipmentComponent::SetWeaponVisualState(
    ENiosWeaponReadyState NewReadyState,
    ENiosWeaponVisualSocketState NewSocketState,
    EEquipSlot ActiveWeaponSlot)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        Server_SetWeaponVisualState(NewReadyState, NewSocketState, ActiveWeaponSlot);
        return;
    }

    if (ActiveWeaponSlot == EEquipSlot::None)
    {
        ActiveWeaponSlot = ResolveDefaultActiveWeaponSlot();
    }

    const FReplicatedEquipmentSlot ActiveSlotData = GetEquipmentSlot(ActiveWeaponSlot);

    FNiosWeaponVisualState NewState;
    NewState.ReadyState = ActiveSlotData.IsEmpty() ? ENiosWeaponReadyState::None : NewReadyState;
    NewState.SocketState = ActiveSlotData.IsEmpty() ? ENiosWeaponVisualSocketState::None : NewSocketState;
    NewState.ActiveWeaponSlot = ActiveSlotData.IsEmpty() ? EEquipSlot::None : ActiveWeaponSlot;
    NewState.ItemID = ActiveSlotData.ItemID;
    NewState.ItemInstanceIndex = ActiveSlotData.ItemInstanceIndex;

    ApplyWeaponVisualState(NewState, true);
}

void UNiosCore_EquipmentComponent::Server_SetWeaponVisualState_Implementation(
    ENiosWeaponReadyState NewReadyState,
    ENiosWeaponVisualSocketState NewSocketState,
    EEquipSlot ActiveWeaponSlot)
{
    SetWeaponVisualState(NewReadyState, NewSocketState, ActiveWeaponSlot);
}

void UNiosCore_EquipmentComponent::ApplyWeaponVisualState(const FNiosWeaponVisualState& NewState, bool bBroadcastAttach)
{
    const bool bChanged =
        WeaponVisualState.ReadyState != NewState.ReadyState ||
        WeaponVisualState.SocketState != NewState.SocketState ||
        WeaponVisualState.ActiveWeaponSlot != NewState.ActiveWeaponSlot ||
        WeaponVisualState.ItemID != NewState.ItemID ||
        WeaponVisualState.ItemInstanceIndex != NewState.ItemInstanceIndex;

    WeaponVisualState = NewState;

    if (bChanged || bBroadcastAttach)
    {
        BroadcastWeaponVisualStateChanged(bBroadcastAttach);
    }
}

void UNiosCore_EquipmentComponent::BroadcastWeaponVisualStateChanged(bool bBroadcastAttach)
{
    OnWeaponVisualStateChanged.Broadcast(WeaponVisualState.ReadyState, WeaponVisualState);
    BP_OnWeaponVisualStateChanged(WeaponVisualState.ReadyState, WeaponVisualState);

    if (!bBroadcastAttach)
    {
        return;
    }

    const FReplicatedEquipmentSlot ActiveSlotData = GetActiveWeaponEquipmentSlot();
    OnWeaponAttachRequested.Broadcast(WeaponVisualState.SocketState, ActiveSlotData, WeaponVisualState);
    BP_OnWeaponAttachRequested(WeaponVisualState.SocketState, ActiveSlotData, WeaponVisualState);
}

void UNiosCore_EquipmentComponent::OnRep_WeaponVisualState()
{
    BroadcastWeaponVisualStateChanged(true);
}

void UNiosCore_EquipmentComponent::AttachWeaponVisualToHand()
{
    SetWeaponVisualState(ENiosWeaponReadyState::InHands, ENiosWeaponVisualSocketState::Hand, WeaponVisualState.ActiveWeaponSlot);
}

void UNiosCore_EquipmentComponent::AttachWeaponVisualToBack()
{
    SetWeaponVisualState(ENiosWeaponReadyState::Sheathed, ENiosWeaponVisualSocketState::Back, WeaponVisualState.ActiveWeaponSlot);
}

void UNiosCore_EquipmentComponent::AttachWeaponVisualToHip()
{
    SetWeaponVisualState(ENiosWeaponReadyState::Sheathed, ENiosWeaponVisualSocketState::Hip, WeaponVisualState.ActiveWeaponSlot);
}

void UNiosCore_EquipmentComponent::AttachWeaponVisualToCurrentSocket()
{
    BroadcastWeaponVisualStateChanged(true);
}

bool UNiosCore_EquipmentComponent::ResolveWeaponSlotMatchingTypes(
    const TArray<ENiosWeaponType>& RequiredWeaponTypes,
    EEquipSlot& OutSlot,
    FReplicatedEquipmentSlot& OutSlotData) const
{
    OutSlot = EEquipSlot::None;
    OutSlotData = FReplicatedEquipmentSlot();

    auto MatchesRequiredTypes = [&](const FReplicatedEquipmentSlot& SlotData) -> bool
    {
        if (SlotData.IsEmpty())
        {
            return false;
        }

        if (RequiredWeaponTypes.Num() == 0)
        {
            return true;
        }

        FNiosWeaponRow WeaponRow;
        if (!ResolveWeaponRow(SlotData.ItemID, WeaponRow))
        {
            return false;
        }

        return RequiredWeaponTypes.Contains(WeaponRow.WeaponType);
    };

    // Prefer the currently active weapon slot when it satisfies the requirement.
    if (WeaponVisualState.ActiveWeaponSlot != EEquipSlot::None)
    {
        const FReplicatedEquipmentSlot ActiveSlotData = GetEquipmentSlot(WeaponVisualState.ActiveWeaponSlot);
        if (MatchesRequiredTypes(ActiveSlotData))
        {
            OutSlot = WeaponVisualState.ActiveWeaponSlot;
            OutSlotData = ActiveSlotData;
            return true;
        }
    }

    // Then try the usual active/default weapon slot.
    const EEquipSlot DefaultSlot = ResolveDefaultActiveWeaponSlot();
    if (DefaultSlot != EEquipSlot::None)
    {
        const FReplicatedEquipmentSlot DefaultSlotData = GetEquipmentSlot(DefaultSlot);
        if (MatchesRequiredTypes(DefaultSlotData))
        {
            OutSlot = DefaultSlot;
            OutSlotData = DefaultSlotData;
            return true;
        }
    }

    static const EEquipSlot CandidateSlots[] =
    {
        EEquipSlot::RightHand,
        EEquipSlot::LeftHand,
        EEquipSlot::RangedWeapon
    };

    for (const EEquipSlot CandidateSlot : CandidateSlots)
    {
        const FReplicatedEquipmentSlot CandidateSlotData = GetEquipmentSlot(CandidateSlot);
        if (MatchesRequiredTypes(CandidateSlotData))
        {
            OutSlot = CandidateSlot;
            OutSlotData = CandidateSlotData;
            return true;
        }
    }

    return false;
}

bool UNiosCore_EquipmentComponent::HasEquippedWeaponMatchingTypes(const TArray<ENiosWeaponType>& RequiredWeaponTypes) const
{
    EEquipSlot Slot = EEquipSlot::None;
    FReplicatedEquipmentSlot SlotData;
    return ResolveWeaponSlotMatchingTypes(RequiredWeaponTypes, Slot, SlotData);
}

bool UNiosCore_EquipmentComponent::IsWeaponInHandsMatchingTypes(const TArray<ENiosWeaponType>& RequiredWeaponTypes) const
{
    EEquipSlot Slot = EEquipSlot::None;
    FReplicatedEquipmentSlot SlotData;
    if (!ResolveWeaponSlotMatchingTypes(RequiredWeaponTypes, Slot, SlotData))
    {
        return false;
    }

    return WeaponVisualState.ReadyState == ENiosWeaponReadyState::InHands &&
        WeaponVisualState.ActiveWeaponSlot == Slot &&
        WeaponVisualState.ItemID == SlotData.ItemID &&
        WeaponVisualState.ItemInstanceIndex == SlotData.ItemInstanceIndex;
}

ENiosWeaponVisualSocketState UNiosCore_EquipmentComponent::GetDefaultSheathedSocketForSlot(EEquipSlot Slot) const
{
    return Slot == EEquipSlot::RangedWeapon
        ? ENiosWeaponVisualSocketState::RangedBack
        : ENiosWeaponVisualSocketState::Back;
}

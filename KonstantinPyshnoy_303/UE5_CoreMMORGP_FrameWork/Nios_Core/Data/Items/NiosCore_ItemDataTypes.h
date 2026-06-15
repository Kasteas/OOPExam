#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Data/Stats/NiosCore_StatTypes.h"
#include "Data/StatusEffects/NiosCore_StatusEffectTypes.h"
#include "NiosCore_ItemDataTypes.generated.h"

/** ================= ENUMS ================== */

UENUM(BlueprintType)
enum class ENiosItemType : uint8
{
    None,
    Weapon,
    Armor,
    Usable,
    Misc
};

UENUM(BlueprintType)
enum class ENiosWeaponType : uint8
{
    None UMETA(DisplayName = "None"),

    // Existing values kept in their old order for DataTable/backward compatibility.
    Sword UMETA(DisplayName = "Sword"),
    Axe UMETA(DisplayName = "Axe"),
    Dagger UMETA(DisplayName = "Dagger"),
    Staff UMETA(DisplayName = "Staff"),
    Bow UMETA(DisplayName = "Bow"),

    // Legacy generic two-handed value. Prefer more specific values below for new rows.
    TwoHanded UMETA(DisplayName = "Two-Handed (Legacy)"),

    // New two-hand occupancy weapon categories. These occupy both LeftHand and RightHand by the same ItemInstanceIndex.
    TwoHandedSword UMETA(DisplayName = "Two-Handed Sword"),
    GreatWeapon UMETA(DisplayName = "Great Weapon"),
    Polearm UMETA(DisplayName = "Polearm"),
    Dual UMETA(DisplayName = "Dual Weapons"),

    // Off-hand utility categories. These are allowed only in LeftHand and do not count as main-hand weapons.
    Shield UMETA(DisplayName = "Shield"),
    Orb UMETA(DisplayName = "Orb"),
    Torch UMETA(DisplayName = "Torch"),
    OffHand UMETA(DisplayName = "Off-Hand Utility")
};

UENUM(BlueprintType)
enum class ENiosArmorType : uint8
{
    None, Cloth, Leather, Mail, Plate
};

UENUM(BlueprintType)
enum class ENiosArmorSlot : uint8
{
    None, Head, Chest, Hands, Legs, Feet, Shoulder, Back, Ring, Trinket,

    // Generic left-hand secondary slot for shields, off-hand foci/orbs, torches, etc.
    OffHand
};

/** ================= STRUCTS ================== */
USTRUCT(BlueprintType)
struct FNiosResolvedInventoryItemData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    bool bValid = false;

    UPROPERTY(BlueprintReadOnly)
    FName ItemID = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    ENiosItemType ItemType = ENiosItemType::None;

    UPROPERTY(BlueprintReadOnly)
    bool bStackable = false;

    UPROPERTY(BlueprintReadOnly)
    int32 MaxStack = 1;

    UPROPERTY(BlueprintReadOnly)
    FName SkillID = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    TObjectPtr<class UNiosCore_SkillDataAsset> AbilityData = nullptr;

    UPROPERTY(BlueprintReadOnly)
    bool bConsumeOnUse = false;

    UPROPERTY(BlueprintReadOnly)
    int32 ConsumeCount = 1;

    /** Local usable-item cooldown duration. Runtime stores absolute end time, not ticking remaining time. */
    UPROPERTY(BlueprintReadOnly)
    float UseCooldown = 10.f;

    /** If true, usable item may also trigger a global item/consumable cooldown later. */
    UPROPERTY(BlueprintReadOnly)
    bool bUseGlobalCooldown = true;

    /**
     * Optional global cooldown override for usable items.
     * < 0: use CooldownComponent.DefaultItemGlobalCooldown.
     * = 0: do not start global item cooldown.
     * > 0: use this value.
     */
    UPROPERTY(BlueprintReadOnly)
    float GlobalCooldownOverride = -1.f;
};
// ������� ������ DT_Items
USTRUCT(BlueprintType)
struct FNiosItemBaseRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Name;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ENiosItemType ItemType = ENiosItemType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 Price = 0;

    // ����� �� ���������
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bStackable = true;

    // ����� ������
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 MaxStack = 200;

    FORCEINLINE bool IsStackable() const { return bStackable; }
};

// Weapon
USTRUCT(BlueprintType)
struct FNiosWeaponRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ENiosWeaponType WeaponType = ENiosWeaponType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Damage = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float AttackSpeed = 1.f;


    /** Runtime stat bonuses granted while this weapon instance is equipped. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FNiosStatModifier> StatModifiers;

    /** Permanent status effects granted while this weapon instance is equipped. Must resolve to Infinite status effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Equipment|Status Effects")
    TArray<FName> GrantedStatusEffectIDs;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<UStaticMesh> Mesh;
};

// Armor
USTRUCT(BlueprintType)
struct FNiosArmorRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ENiosArmorType ArmorType = ENiosArmorType::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    ENiosArmorSlot ArmorSlot = ENiosArmorSlot::None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    float Armor = 0.f;


    /** Runtime stat bonuses granted while this armor instance is equipped. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TArray<FNiosStatModifier> StatModifiers;

    /** Permanent status effects granted while this armor instance is equipped. Must resolve to Infinite status effects. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Nios|Equipment|Status Effects")
    TArray<FName> GrantedStatusEffectIDs;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TSoftObjectPtr<USkeletalMesh> Mesh;
};

// Usable
USTRUCT(BlueprintType)
struct FNiosUsableRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName ItemID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    FName SkillID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    TObjectPtr<class UNiosCore_SkillDataAsset> AbilityData = nullptr;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bConsumeOnUse = true;

    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    int32 ConsumeCount = 1;

    /** Local/personal cooldown for this usable item. Default 10 seconds for generic consumables. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
    float UseCooldown = 10.f;

    /** If true, this item also participates in a global item/consumable cooldown. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly)
    bool bUseGlobalCooldown = true;

    /**
     * Optional per-item global cooldown override.
     * < 0: use CooldownComponent.DefaultItemGlobalCooldown.
     * = 0: item has no global cooldown.
     * > 0: use this value as global cooldown duration.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "bUseGlobalCooldown", ClampMin = "-1"))
    float GlobalCooldownOverride = -1.f;
};

// InventorySlot ��� UI
USTRUCT(BlueprintType)
struct FNiosInventorySlot
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    int32 SlotIndex = INDEX_NONE;

    // Stable per-character item instance index. Server/DB can address item as CharacterID + ItemInstanceIndex.
    UPROPERTY(BlueprintReadOnly)
    int32 ItemInstanceIndex = INDEX_NONE;

    UPROPERTY(BlueprintReadOnly)
    FName ItemID = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    int32 Count = 0;

    UPROPERTY(BlueprintReadOnly)
    bool bEquipped = false;
    FORCEINLINE bool IsEmpty() const { return ItemID.IsNone() || Count <= 0 || ItemInstanceIndex == INDEX_NONE; }
    FORCEINLINE void Clear() { ItemInstanceIndex = INDEX_NONE; ItemID = NAME_None; Count = 0; bEquipped = false;
    }

};

// ItemCache ��� �������� ������ � ��������
USTRUCT(BlueprintType)
struct FNiosInventoryItemCache
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FName ItemID = NAME_None;

    UPROPERTY(BlueprintReadOnly)
    int32 Count = 0;

    UPROPERTY(BlueprintReadOnly)
    TArray<int32> SlotIndices;

    FORCEINLINE bool IsEmpty() const { return ItemID.IsNone() || Count <= 0; }

    FORCEINLINE bool operator==(const FNiosInventoryItemCache& Other) const
    {
        return ItemID == Other.ItemID;
    }
};

UENUM(BlueprintType)
enum class EEquipSlot : uint8
{
    None           UMETA(DisplayName = "None"),

    Helmet         UMETA(DisplayName = "Helmet"),
    Torso          UMETA(DisplayName = "Torso"),
    Legs           UMETA(DisplayName = "Legs"),
    Hands          UMETA(DisplayName = "Hands"),
    Feet           UMETA(DisplayName = "Feet"),
    Accessory      UMETA(DisplayName = "Accessory"),

    Ring1          UMETA(DisplayName = "Ring 1"),
    Ring2          UMETA(DisplayName = "Ring 2"),

    Artifact1      UMETA(DisplayName = "Artifact 1"),
    Artifact2      UMETA(DisplayName = "Artifact 2"),
    Artifact3      UMETA(DisplayName = "Artifact 3"),
    Artifact4      UMETA(DisplayName = "Artifact 4"),

    LeftHand       UMETA(DisplayName = "Left Hand"),
    RightHand      UMETA(DisplayName = "Right Hand"),
    RangedWeapon   UMETA(DisplayName = "Ranged Weapon"),
    Cloak          UMETA(DisplayName = "Cloak"),
    Belt           UMETA(DisplayName = "Belt")
};



/** Message/request payload for UI, GameplayMessages or Blueprint calls: equip item from inventory slot into target equipment slot. */
USTRUCT(BlueprintType)
struct FNiosEquipmentSlotRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 InventorySlotIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EEquipSlot TargetSlot = EEquipSlot::None;

    FORCEINLINE bool IsValid() const
    {
        return InventorySlotIndex != INDEX_NONE && TargetSlot != EEquipSlot::None;
    }
};

USTRUCT(BlueprintType)
struct FReplicatedEquipmentSlot
{
    GENERATED_BODY()

    /** ���� ���������� */
    UPROPERTY(BlueprintReadWrite)
    EEquipSlot Slot = EEquipSlot::None;

    /** ItemID �������� */
    UPROPERTY(BlueprintReadWrite)
    FName ItemID = NAME_None;

    /** Stable reference to concrete item instance inside character inventory. */
    UPROPERTY(BlueprintReadWrite)
    int32 ItemInstanceIndex = INDEX_NONE;

    /** Soft Mesh ��� ������� */
    UPROPERTY(BlueprintReadWrite)
    TSoftObjectPtr<UStaticMesh> Mesh;

    /** ��� SkeletalMesh ��� Transform */
    UPROPERTY(BlueprintReadWrite)
    FTransform RelativeTransform = FTransform::Identity;

    FORCEINLINE bool IsEmpty() const { return ItemID.IsNone(); }
};
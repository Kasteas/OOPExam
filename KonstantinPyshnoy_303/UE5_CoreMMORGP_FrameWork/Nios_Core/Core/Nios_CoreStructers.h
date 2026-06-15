#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPtr.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimInstance.h"
#include "AttributeSet.h"
#include "GameplayEffect.h"
#include "Engine/Texture2D.h"
#include "UObject/ObjectMacros.h"
#include "GameplayEffectTypes.h" 
#include "Nios_CoreStructers.generated.h"

UENUM(BlueprintType)
enum class ENiosActionSlotType : uint8
{
	None,
	Skill,
	Item,
	Emote
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosActionSlotPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 InventorySlotIndex = INDEX_NONE;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGuid InstanceID;

	bool HasInventorySlot() const
	{
		return InventorySlotIndex != INDEX_NONE;
	}
};

UENUM(BlueprintType)
enum class ENiosSlotInteractionProfile : uint8
{
	SkillBook     UMETA(DisplayName = "Skill Book"),
	Hotbar        UMETA(DisplayName = "Hotbar"),
	Inventory     UMETA(DisplayName = "Inventory"),
	Equipment     UMETA(DisplayName = "Equipment")
};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FTextMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Text;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosActionSlotVisualData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Slot")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Slot")
	FText Description;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Slot")
	TSoftObjectPtr<UTexture2D> Icon;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Slot")
	int32 Count = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Slot")
	bool bShowCount = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Slot")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Slot")
	bool bValid = false;
};
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosActionSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENiosActionSlotType Type = ENiosActionSlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActionID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FNiosActionSlotPayload Payload;

	UPROPERTY(BlueprintReadWrite, Category = "ActionSlot")
	int32 Count = 1;
	bool IsEmpty() const
	{
		return Type == ENiosActionSlotType::None || ActionID.IsNone();
	}

	bool IsSkill() const
	{
		return Type == ENiosActionSlotType::Skill;
	}

	bool IsItem() const
	{
		return Type == ENiosActionSlotType::Item;
	}

	void Clear()
	{
		Type = ENiosActionSlotType::None;
		ActionID = NAME_None;
		Payload = FNiosActionSlotPayload();
	}


};
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosActionSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ENiosActionSlotType Type = ENiosActionSlotType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActionID = NAME_None;

	bool IsEmpty() const
	{
		return Type == ENiosActionSlotType::None || ActionID.IsNone();
	}
};

UENUM(BlueprintType)
enum class EMessageType : uint8
{
	none UMETA(DisplayName = "none"),
	Loot UMETA(DisplayName = "Loot"),
	Error UMETA(DisplayName = "Error"),
	QuestTaked UMETA(DisplayName = "QuestTaked"),
	QuestComplete UMETA(DisplayName = "QuestComplete"),
	QuestCanceled UMETA(DisplayName = "QuestCanceled")
	//    UMETA(DisplayName = ""),
};

UENUM(BlueprintType)
enum class ESQLGetType : uint8
{
	none UMETA(DisplayName = "none"),
	GetPlayerLogins UMETA(DisplayName = "GetPlayerLogPasIdForLogIn"),
	GetPlayerPasswordByLogin UMETA(DisplayName = "GetPlayerPasswordByLogin"),
	GetPlayerCharacters UMETA(DisplayName = "GetPlayerCharacters"),
	GetPlayerIdByLogin UMETA(DisplayName = "GetPlayerIdByLogin"),
	GetPartryInfoForCharacter UMETA(DisplayName = "GetPartryInfoForCharacter"),
	GetNewPartyId UMETA(DisplayName = "GetNewPartyId"),
	GetQuestsInfo UMETA(DisplayName = "GetQuestsInfo"),
	GetPartyStatusByName UMETA(DisplayName = "GetPartyStatusByName"),
	GetItemsByCharacterName UMETA(DisplayName = "GetItemsByCharacterName"),
	//    UMETA(DisplayName = ""),
};
UENUM(BlueprintType)
enum class ERaceType : uint8
{
	none UMETA(DisplayName = "none"),
	Human UMETA(DisplayName = "Human"),
	Woman UMETA(DisplayName = "Woman"),
	Muscomorph UMETA(DisplayName = "Muscomorpg"),
	Karckmahre UMETA(DisplayName = "Kackmahre")
	//    UMETA(DisplayName = ""),
};

UENUM(BlueprintType)
enum class EAIMoveType : uint8
{
	none UMETA(DisplayName = "none"),
	SplineMove UMETA(DisplayName = "SplineMove"),
	RandomPatrol UMETA(DisplayName = "RandomPatrol"),
	PointGuardian UMETA(DisplayName = "PointGuardian")
};


UENUM(BlueprintType)
enum class EQuestType : uint8
{
	TalkTo UMETA(DisplayName = "TalkTo"),
	Kill UMETA(DisplayName = "Kill"),
	CollectItemsFor UMETA(DisplayName = "CollectItemsFor")
};

UENUM(BlueprintType)
enum class ETargetType : uint8
{
	None UMETA(DisplayName = "None"),
	Enemy UMETA(DisplayName = "Enemy"),
	Friendly UMETA(DisplayName = "Friendly"),
	TalkNPC UMETA(DisplayName = "TalkNPC"),
	Container UMETA(DisplayName = "Container"),
};


UENUM(BlueprintType)
enum class EPlayerClassType : uint8
{
	None UMETA(DisplayName = "None"),
	Warrior UMETA(DisplayName = "Warrior"),
	Mage UMETA(DisplayName = "Mage"),
	Priest UMETA(DisplayName = "Priest"),
	Hunter UMETA(DisplayName = "Hunter"),
};

UENUM(BlueprintType)
enum class EDamageType : uint8
{
	None UMETA(DisplayName = "None"),
	Physical UMETA(DisplayName = "Physical"),
	Magick UMETA(DisplayName = "Magick"),

};


UENUM(BlueprintType)
enum class EEffectType : uint8
{
	None UMETA(DisplayName = "None"),
	DurationEffect UMETA(DisplayName = "DurationEffect"),
	TickableEffect UMETA(DisplayName = "TickableEffect"),

};


UENUM(BlueprintType)
enum class EPartyChangeButton : uint8
{
	None UMETA(DisplayName = "None"),
	Kick UMETA(DisplayName = "Kick"),
	MakeLeader UMETA(DisplayName = "MakeLeader"),
	Ungroup UMETA(DisplayName = "Ungroup"),
	LeaveFromParty UMETA(DisplayName = "LeaveFromParty"),
};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FAbilityData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSoftObjectPtr<UMaterialInstance> Icon;
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText Description;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FBoolMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool eBool = false;
};




USTRUCT(BlueprintType)
struct NIOS_CORE_API FCharacterClassData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class ACharacter> CharacterClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ClassName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ClassDescription;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UTexture2D> ClassIcon = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USkeletalMesh> PreviewMesh = nullptr;
	UPROPERTY(BlueprintReadWrite)
	FString PlayerName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<UAnimSequence> PlayAnimation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UAnimInstance> AnimBP;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 level = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 XP = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPlayerClassType Class = EPlayerClassType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class UGameplayEffect> StartupEffect;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FSQLSelectMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Command;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Query = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	ESQLGetType GetType = ESQLGetType::none;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FAnimMessageSrt
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EMessageType Messagetype = EMessageType::none;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Text;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	TObjectPtr<UTexture2D> Icon = nullptr;
};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName Name;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Quanity = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 SQL_Index = -1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Tags;
};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FItemIndex
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	uint8 Index = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FSlotInfo Item;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FBackpackInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName BackpackID;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	uint8 Size = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray <FItemIndex> Items;
};







USTRUCT(BlueprintType)
struct NIOS_CORE_API FBackpackTargetStruct
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 BackpackIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	uint8 SlotIndex = 0;

};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FStat
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayAttribute StatTag;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Value = 0.f;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FClassStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray <FStat> LevelStats;

};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FEquipmentItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ItemId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag EquipType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray <FStat> Stats;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FObjectiveDetalis
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName TargetId;


	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 QuanityTarget = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName MapId;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FQuestStep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	EQuestType QuestType = EQuestType::TalkTo;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName StepText;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FObjectiveDetalis Detalis;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FQuestRewards
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 Gold = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	float XP = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	TArray <FSlotInfo> Items;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FEnemyProgress
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float XP = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FClassStats EnemyStats;

};
////////////////////////////////////////////////////////////////
//DATA TABLES
USTRUCT(BlueprintType)
struct NIOS_CORE_API FItemData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ItemName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag ItemType;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float Cost = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSoftObjectPtr<UTexture2D> Image;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FText Description;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FArmorInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray <FStat> Stats;


};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FWeaponInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
	float MinDamage = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Damage")
	float MaxDamage = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray <FStat> Stats;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	TSoftObjectPtr<UStaticMesh> MyStaticMesh;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FQuestInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	TArray <FQuestStep> QuestStep;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName QuestName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName QuestOverview;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limitation")
	int32 Level = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Limitation")
	TArray <FName> CompletedQuest;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FQuestRewards Rewards;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FLevelData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Level = 1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float XP = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TMap <EPlayerClassType, FClassStats> ClassStats;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FEnemyData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	FName EnemyName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	FName Race;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	TArray<TSubclassOf<UGameplayAbility>> GameplayAbilityClasses;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	TSoftObjectPtr <UObject> PreviewMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Main")
	TSoftObjectPtr<UObject> AnimBPAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LevelSettings")
	TMap <int32, FEnemyProgress> EnemyProgress;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FDefaultSkillsData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	EPlayerClassType PlayerClass = EPlayerClassType::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	TArray<TSubclassOf<UGameplayAbility>> GameplayAbilityClasses;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FEffectData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	EEffectType EffectType = EEffectType::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	float Duration = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	FClassStats StatsValue;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	TSoftObjectPtr<UTexture2D> Image;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FCharacterMenuData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	EPlayerClassType PlayerClass = EPlayerClassType::None;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	FString ClassName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	FString ClassDescription;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	TObjectPtr<UTexture2D> ClassIcon = nullptr;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FAnimData : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Main")
	TMap < FGameplayTag, TSoftObjectPtr<UAnimMontage>> AnimList;


};


////////////////////////////////////////////////////////////////
//DATA TABLES End

USTRUCT(BlueprintType)
struct NIOS_CORE_API FQuestDialog
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName QuestId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 QuestStep = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FString TalkOption;

};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FQuestList
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName QuestId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FQuestInfo Quest;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 CurrentStep = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 CurrentStatus = 0;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FProgressQuestMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	EQuestType QuestType = EQuestType::TalkTo;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName TriggerId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 Quanity = 0;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FQuestStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FName QuestId;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 CurrentStep = 0;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	int32 StepStatus = 0;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FActorMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	TObjectPtr<AActor> Actor = nullptr;

};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FDropedItem
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	FSlotInfo Item;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	float DropChange = 0.f;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FAggroData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	TObjectPtr<AActor> target = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Quest")
	float AggroValue = 0.f;
};


USTRUCT(BlueprintType)
struct NIOS_CORE_API FEffectStatus
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FActiveGameplayEffectHandle ActiveEffectHandle;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName EffectName;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FPlayerPartyInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString Player;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Leader = false;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FPlayerPartyInfoMessage
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray <FPlayerPartyInfo> PlayerParty;
};

USTRUCT(BlueprintType)
struct NIOS_CORE_API FMessageActorWithBool
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<AActor> Actor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool Result = false;


};
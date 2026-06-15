#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "NiosCore_AbilityLoadoutComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayAbility;

/**
 * High-level activation kind used by UI/hotbar/input mapping.
 *
 * This is intentionally NOT a GameplayTag-based input system.
 * Input/UI can create FNiosCoreActivationRequest and route it in Blueprint:
 * - Skill + ID = find granted/battle skill by SkillDataAsset/SkillID and activate it.
 * - Item + ID = find item use logic, inventory entry, or item executor and run it.
 * - Emote + ID = play selected emote.
 * - Mount + ID = mount/dismount or mount-specific action.
 * - UtilityAbility + AbilityClass = activate a persistent GAS utility ability.
 * - ToggleWeapon = shortcut for the weapon-ready toggle ability.
 */
UENUM(BlueprintType)
enum class ENiosCoreActivationKind : uint8
{
    None            UMETA(DisplayName="None"),

    Skill           UMETA(DisplayName="Skill"),
    Item            UMETA(DisplayName="Item"),
    Emote           UMETA(DisplayName="Emote"),
    Mount           UMETA(DisplayName="Mount"),
    Profession      UMETA(DisplayName="Profession"),
    Interaction     UMETA(DisplayName="Interaction"),

    UtilityAbility  UMETA(DisplayName="Utility Ability"),
    ToggleWeapon    UMETA(DisplayName="Toggle Weapon"),

    Debug           UMETA(DisplayName="Debug")
};

/**
 * Blueprint-friendly request describing WHAT the player wants to activate.
 *
 * Input mapping / hotbar should produce this struct, then Blueprint can route it
 * to the correct system without hardcoding every key directly into character code.
 *
 * ID is optional and semantic:
 * - Skill:       ID = MeleeAttack / Fireball / HealTarget, then GASC finds matching granted skill/DA.
 * - Item:        ID = Potion23 / Bandage01, then inventory/item system finds use logic.
 * - Emote:       ID = Wave / Sit / Dance.
 * - Mount:       ID = Horse01 / CurrentMount / MountSprint.
 * - Profession: ID = MiningSwing / FishingCast / CraftBlacksmith.
 * - Interaction:ID = Talk / Loot / Inspect / Use.
 *
 * AbilityClass is optional and mainly for UtilityAbility-style actions
 * such as GA_ToggleWeaponReady, GA_Interact, GA_ToggleMount, etc.
 */
USTRUCT(BlueprintType)
struct NIOS_CORE_API FNiosCoreActivationRequest
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Activation")
    ENiosCoreActivationKind Kind = ENiosCoreActivationKind::None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Activation")
    FName ID = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Activation")
    TSubclassOf<UGameplayAbility> AbilityClass;

    /** Optional slot/variant/index. Useful for item stack index, emote variant, mount action, profession tier, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Activation")
    int32 Variant = 0;

    bool HasID() const { return !ID.IsNone(); }

    bool HasAbilityClass() const { return AbilityClass != nullptr; }
};

/**
 * Persistent ability loadout owner for MMO characters/player state.
 *
 * This component is intentionally NOT a skill-book UI and NOT a hotbar.
 * It is the authoritative place that grants long-lived abilities to the ASC.
 *
 * Examples of abilities that belong here:
 * - Combat abilities granted by class/spec/loadout.
 * - Utility abilities that every character can have, such as ToggleWeaponReady.
 * - Item-use abilities, when consumables/tools are executed through GAS.
 * - Emote abilities, because emotes still need networked animation/state rules.
 * - Mount abilities, such as summon, mount, dismount, sprint, special mount actions.
 * - Profession abilities, such as gather, craft interaction, fishing, mining, etc.
 * - Interaction abilities, such as interact, talk, inspect, loot, open door.
 * - Debug/test abilities for development maps.
 *
 * What should NOT live here:
 * - Per-frame state.
 * - Runtime cast state.
 * - Selected target.
 * - Temporary one-shot abilities created only for a single effect.
 * - UI slot layout. UI may reference these abilities, but does not own them.
 *
 * Design rule:
 * Give persistent abilities once on server when the ASC is initialized, then activate them
 * by class/handle later. Do not GiveAbility/RemoveAbility every time a player presses a key.
 */
UCLASS(ClassGroup=(Nios), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_AbilityLoadoutComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_AbilityLoadoutComponent();

    virtual void BeginPlay() override;

    /** Grants all configured persistent abilities to the owner's ASC. Server only. Safe to call multiple times. */
    UFUNCTION(BlueprintCallable, Category="Nios|Ability Loadout")
    void GrantStartupAbilities(UAbilitySystemComponent* ASCOverride = nullptr);

    /** Removes abilities granted by this component. Mostly useful for debug/loadout rebuilds. Server only. */
    UFUNCTION(BlueprintCallable, Category="Nios|Ability Loadout")
    void ClearGrantedStartupAbilities(UAbilitySystemComponent* ASCOverride = nullptr);

    UFUNCTION(BlueprintPure, Category="Nios|Ability Loadout")
    bool HasGrantedStartupAbilities() const { return bStartupAbilitiesGranted; }

    /** Convenience activation path for hotkeys such as ToggleWeaponReady. */
    UFUNCTION(BlueprintCallable, Category="Nios|Ability Loadout")
    bool TryActivateAbilityByClass(TSubclassOf<UGameplayAbility> AbilityClass, UAbilitySystemComponent* ASCOverride = nullptr) const;

    /** Optional predefined activation requests for BP/UI/hotbar experiments.
     *  This component only stores the data; actual routing can stay in Blueprint. */
    UFUNCTION(BlueprintPure, Category="Nios|Ability Loadout|Activation")
    TArray<FNiosCoreActivationRequest> GetActivationRequests() const { return ActivationRequests; }

protected:
    /**
     * Optional UI/input/hotbar activation request presets.
     *
     * This is intentionally only data storage. Blueprint decides how to route each request:
     * Skill -> GASC TryActivateSkillByID, Item -> Inventory UseItem, ToggleWeapon -> GA_ToggleWeaponReady, etc.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Activation")
    TArray<FNiosCoreActivationRequest> ActivationRequests;

    /** Skills/classes/spec abilities. Usually player progression/loadout owned. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Combat")
    TArray<TSubclassOf<UGameplayAbility>> CombatAbilities;

    /** Always-available utility actions: ToggleWeaponReady, Interact, Dodge, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Utility")
    TArray<TSubclassOf<UGameplayAbility>> UtilityAbilities;

    /** Item-use abilities: potions, scrolls, tools, usable inventory actions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Items")
    TArray<TSubclassOf<UGameplayAbility>> ItemAbilities;

    /** Social/emote abilities: wave, sit, dance, gesture, roleplay actions. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Emotes")
    TArray<TSubclassOf<UGameplayAbility>> EmoteAbilities;

    /** Mount abilities: summon mount, mount/dismount, mount sprint/special. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Mounts")
    TArray<TSubclassOf<UGameplayAbility>> MountAbilities;

    /** Profession abilities: gathering, crafting actions, fishing, mining, harvesting. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Professions")
    TArray<TSubclassOf<UGameplayAbility>> ProfessionAbilities;

    /** World interaction abilities: interact, loot, talk, inspect, use object. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Interaction")
    TArray<TSubclassOf<UGameplayAbility>> InteractionAbilities;

    /** Dev-only ability bucket for test maps and temporary experiments. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Nios|Ability Loadout|Debug")
    TArray<TSubclassOf<UGameplayAbility>> DebugAbilities;

private:
    UAbilitySystemComponent* ResolveASC(UAbilitySystemComponent* ASCOverride) const;
    void GrantAbilityArray(UAbilitySystemComponent* ASC, const TArray<TSubclassOf<UGameplayAbility>>& Abilities, const TCHAR* BucketName);
    bool ASCAlreadyHasAbilityClass(const UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass) const;

private:
    UPROPERTY(Transient)
    TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

    UPROPERTY(Transient)
    bool bStartupAbilitiesGranted = false;
};

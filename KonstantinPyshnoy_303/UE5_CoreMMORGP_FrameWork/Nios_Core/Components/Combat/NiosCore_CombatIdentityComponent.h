#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "NiosCore_CombatIdentityComponent.generated.h"

/**
 * Lightweight combat identity override for actors that are not full Nios GAS actors,
 * or for special designer-authored actors such as immortal NPCs, city guards,
 * training dummies, destructibles, or event objects.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Nios), meta=(BlueprintSpawnableComponent))
class NIOS_CORE_API UNiosCore_CombatIdentityComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UNiosCore_CombatIdentityComponent();

    /** If true, this component's TeamID overrides INiosGASActorInterface::GetTeamID. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Combat|Identity")
    bool bOverrideTeamID = false;

    /** Simple starter team id. Later this can be replaced/augmented by factions, guilds, wars, city ownership, etc. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Combat|Identity", meta = (EditCondition = "bOverrideTeamID"))
    FName TeamID = TEXT("Neutral");

    /** Actor cannot be harmed by ordinary combat rules. Good for peaceful NPCs and protected world objects. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Combat|Rules")
    bool bImmortal = false;

    /** Actor can appear as a hostile/valid combat target. Disable for pure interaction objects. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Combat|Rules")
    bool bCanBeTargeted = true;

    /** Actor is allowed to cause hostile damage. Disable for passive NPCs/props even if they have an ASC. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Combat|Rules")
    bool bCanDealDamage = true;

    /** Future extension point: faction/city/guild/war/arena tags. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Nios|Combat|Identity")
    FGameplayTagContainer CombatTags;

    UFUNCTION(BlueprintCallable, Category = "Nios|Combat")
    void SetImmortal(bool bNewImmortal);

    UFUNCTION(BlueprintCallable, Category = "Nios|Combat")
    void SetCanBeTargeted(bool bNewCanBeTargeted);

    UFUNCTION(BlueprintCallable, Category = "Nios|Combat")
    void SetCanDealDamage(bool bNewCanDealDamage);

    UFUNCTION(BlueprintCallable, Category = "Nios|Combat")
    void SetTeamID(FName NewTeamID, bool bEnableOverride = true);

    UFUNCTION(BlueprintPure, Category = "Nios|Combat")
    bool IsImmortal() const { return bImmortal; }

    UFUNCTION(BlueprintPure, Category = "Nios|Combat")
    bool CanBeCombatTarget() const { return bCanBeTargeted && !bImmortal; }

    UFUNCTION(BlueprintPure, Category = "Nios|Combat")
    bool CanDealCombatDamage() const { return bCanDealDamage; }
};

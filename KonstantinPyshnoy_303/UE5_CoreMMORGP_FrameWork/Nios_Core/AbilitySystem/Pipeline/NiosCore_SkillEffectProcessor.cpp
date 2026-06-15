#include "AbilitySystem/Pipeline/NiosCore_SkillEffectProcessor.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Core/NiosCore_AbilitySystemComponent.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "AbilitySystem/Pipeline/NiosCore_SkillTargetResolver.h"
#include "Rules/Targeting/NiosTargetRulesSubsystem.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "AbilitySystem/Interfaces/NiosGASActorInterface.h"
#include "Components/StatusEffects/NiosCore_StatusEffectComponent.h"
#include "Components/Loot/NiosCore_LootSourceComponent.h"
#include "Components/ActorComponent.h"
#include "Data/StatusEffects/NiosCore_StatusEffectDataAsset.h"
#include "Data/StatusEffects/NiosCore_StatusEffectCatalog.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerState.h"


namespace
{

    bool Nios_SkillEffect_GetMaxAttributeForResourceAttribute(const FGameplayAttribute& Attribute, FGameplayAttribute& OutMaxAttribute)
    {
        if (Attribute == UNiosCore_AttributeSet::GetHealthAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxHealthAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetManaAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxManaAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetEnergyAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxEnergyAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetRageAttribute())
        {
            OutMaxAttribute = UNiosCore_AttributeSet::GetMaxRageAttribute();
            return true;
        }

        return false;
    }

    bool Nios_SkillEffect_GetResourceAttributeForMaxAttribute(const FGameplayAttribute& Attribute, FGameplayAttribute& OutResourceAttribute)
    {
        if (Attribute == UNiosCore_AttributeSet::GetMaxHealthAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetHealthAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetMaxManaAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetManaAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetMaxEnergyAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetEnergyAttribute();
            return true;
        }
        if (Attribute == UNiosCore_AttributeSet::GetMaxRageAttribute())
        {
            OutResourceAttribute = UNiosCore_AttributeSet::GetRageAttribute();
            return true;
        }

        return false;
    }

    bool Nios_ApplyClampedDeltaToCurrentResource(
        UAbilitySystemComponent* ASC,
        const FGameplayAttribute& Attribute,
        float Delta,
        float& OutAfterValue)
    {
        OutAfterValue = 0.f;

        if (!ASC || !Attribute.IsValid())
        {
            return false;
        }

        FGameplayAttribute MaxAttribute;
        if (!Nios_SkillEffect_GetMaxAttributeForResourceAttribute(Attribute, MaxAttribute))
        {
            return false;
        }

        const float BeforeValue = ASC->GetNumericAttribute(Attribute);
        const float MaxValue = FMath::Max(0.f, ASC->GetNumericAttribute(MaxAttribute));
        const float NewValue = FMath::Clamp(BeforeValue + Delta, 0.f, MaxValue);

        if (UNiosCore_AbilitySystemComponent* NiosASC = Cast<UNiosCore_AbilitySystemComponent>(ASC))
        {
            NiosASC->SetAttributeValue(Attribute, NewValue);
            OutAfterValue = NiosASC->GetNumericAttribute(Attribute);
            return true;
        }

        ASC->SetNumericAttributeBase(Attribute, NewValue);
        OutAfterValue = NewValue;
        return true;
    }

    UNiosCore_StatusEffectComponent* ResolveStatusEffectComponentFromASC(UAbilitySystemComponent* ASC)
    {
        if (!ASC)
        {
            return nullptr;
        }

        if (AActor* OwnerActor = ASC->GetOwnerActor())
        {
            if (UNiosCore_StatusEffectComponent* Component = OwnerActor->FindComponentByClass<UNiosCore_StatusEffectComponent>())
            {
                return Component;
            }
        }

        if (AActor* AvatarActor = ASC->GetAvatarActor())
        {
            if (UNiosCore_StatusEffectComponent* Component = AvatarActor->FindComponentByClass<UNiosCore_StatusEffectComponent>())
            {
                return Component;
            }

            if (const APawn* Pawn = Cast<APawn>(AvatarActor))
            {
                if (APlayerState* PlayerState = Pawn->GetPlayerState())
                {
                    return PlayerState->FindComponentByClass<UNiosCore_StatusEffectComponent>();
                }
            }
        }

        return nullptr;
    }

    UNiosCore_StatusEffectDataAsset* ResolveStatusEffectData(UWorld* World, const FNiosSkillEffectDefinition& Effect, FNiosStatusEffectCatalogEntry* OutEntry = nullptr)
    {
        if (OutEntry)
        {
            *OutEntry = FNiosStatusEffectCatalogEntry();
        }

        if (!Effect.StatusEffectID.IsNone())
        {
            FNiosStatusEffectCatalogEntry Entry;
            if (UNiosCore_StatusEffectCatalogLibrary::ResolveStatusEffectByID(World, Effect.StatusEffectID, Entry))
            {
                if (OutEntry)
                {
                    *OutEntry = Entry;
                }
                return Entry.EffectData;
            }
        }

        return nullptr;
    }

    ENiosCoreTargetApplicationPolicy PolicyFromSkillAffectType(const UNiosCore_SkillDataAsset* Skill)
    {
        if (!Skill)
        {
            return ENiosCoreTargetApplicationPolicy::SourceOnly;
        }

        switch (Skill->AffectType)
        {
        case ENiosAbilityAffectType::Harm:
            return ENiosCoreTargetApplicationPolicy::EnemiesOnly;

        case ENiosAbilityAffectType::Help:
            return ENiosCoreTargetApplicationPolicy::FriendliesOnly;

        case ENiosAbilityAffectType::Neutral:
        default:
            // Safe default: neutral skills should not accidentally modify arbitrary neutral actors.
            // Explicitly choose AnyIncludingNeutral for non-stat neutral effects such as movement/interaction.
            return ENiosCoreTargetApplicationPolicy::SourceOnly;
        }
    }

    ENiosCoreTargetApplicationPolicy ResolveTargetPolicy(
        UWorld* World,
        const UNiosCore_SkillDataAsset* Skill,
        const FNiosSkillEffectDefinition& Effect)
    {
        if (Effect.TargetPolicy != ENiosCoreTargetApplicationPolicy::AutoFromEffectType)
        {
            return Effect.TargetPolicy;
        }

        switch (Effect.EffectType)
        {
        case ENiosSkillEffectType::Damage:
            return ENiosCoreTargetApplicationPolicy::EnemiesOnly;

        case ENiosSkillEffectType::Heal:
            return ENiosCoreTargetApplicationPolicy::FriendliesOnly;

        case ENiosSkillEffectType::ApplyStatusEffect:
        case ENiosSkillEffectType::RemoveStatusEffect:
            if (UNiosCore_StatusEffectDataAsset* StatusEffect = ResolveStatusEffectData(World, Effect))
            {
                if (StatusEffect->TargetPolicy != ENiosCoreTargetApplicationPolicy::AutoFromEffectType)
                {
                    return StatusEffect->TargetPolicy;
                }
            }
            return PolicyFromSkillAffectType(Skill);

        case ENiosSkillEffectType::ResourceDelta:
        case ENiosSkillEffectType::AttributeDelta:
        default:
            return PolicyFromSkillAffectType(Skill);
        }
    }

    bool IsTargetAllowedForPolicy(
        UWorld* World,
        AActor* SourceActor,
        UAbilitySystemComponent* SourceASC,
        AActor* TargetActor,
        UAbilitySystemComponent* TargetASC,
        ENiosCoreTargetApplicationPolicy Policy)
    {
        if (!TargetASC)
        {
            return false;
        }

        if (Policy == ENiosCoreTargetApplicationPolicy::AnyIncludingNeutral)
        {
            return true;
        }

        const bool bIsSource = (SourceASC && TargetASC == SourceASC) || (SourceActor && TargetActor == SourceActor);
        if (Policy == ENiosCoreTargetApplicationPolicy::SourceOnly)
        {
            return bIsSource;
        }

        if (!SourceActor || !TargetActor || !World)
        {
            return false;
        }

        if (const UNiosTargetRulesSubsystem* Rules = World->GetSubsystem<UNiosTargetRulesSubsystem>())
        {
            switch (Policy)
            {
            case ENiosCoreTargetApplicationPolicy::EnemiesOnly:
                return Rules->CanHarm(SourceActor, TargetActor);

            case ENiosCoreTargetApplicationPolicy::FriendliesOnly:
                return Rules->CanHelp(SourceActor, TargetActor);

            case ENiosCoreTargetApplicationPolicy::NonNeutralOnly:
                return Rules->GetRelation(SourceActor, TargetActor) != ENiosCoreTargetRelation::Neutral;

            case ENiosCoreTargetApplicationPolicy::AutoFromEffectType:
            case ENiosCoreTargetApplicationPolicy::SourceOnly:
            case ENiosCoreTargetApplicationPolicy::AnyIncludingNeutral:
            default:
                break;
            }
        }

        return false;
    }

    struct FNiosDamageInstigatorEventParams
    {
        AActor* DamageInstigator = nullptr;
    };

    struct FNiosDamagedByEventParams
    {
        AActor* SourceActor = nullptr;
        float DamageAmount = 0.f;
    };

    void NotifyDamageListeners(AActor* TargetActor, AActor* SourceActor, float DamageAmount)
    {
        if (!TargetActor || !SourceActor || TargetActor == SourceActor || DamageAmount <= 0.f)
        {
            return;
        }

        static const FName ReportDamageInstigatorName(TEXT("ReportDamageInstigator"));
        static const FName NotifyDamagedByName(TEXT("NotifyDamagedBy"));

        if (UFunction* ReportInstigatorFunction = TargetActor->FindFunction(ReportDamageInstigatorName))
        {
            FNiosDamageInstigatorEventParams Params;
            Params.DamageInstigator = SourceActor;
            TargetActor->ProcessEvent(ReportInstigatorFunction, &Params);
        }

        TArray<UActorComponent*> Components;
        TargetActor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (!Component)
            {
                continue;
            }

            if (UFunction* NotifyDamagedByFunction = Component->FindFunction(NotifyDamagedByName))
            {
                FNiosDamagedByEventParams Params;
                Params.SourceActor = SourceActor;
                Params.DamageAmount = DamageAmount;
                Component->ProcessEvent(NotifyDamagedByFunction, &Params);
            }
        }
    }

    AActor* ResolveLifecycleActorForTargetASC(UAbilitySystemComponent* TargetASC, AActor* TargetActor)
    {
        if (TargetASC)
        {
            if (AActor* AvatarActor = TargetASC->GetAvatarActor())
            {
                return AvatarActor;
            }

            if (APlayerState* PlayerStateOwner = Cast<APlayerState>(TargetASC->GetOwnerActor()))
            {
                if (const AController* OwningController = Cast<AController>(PlayerStateOwner->GetOwner()))
                {
                    if (APawn* ControlledPawn = OwningController->GetPawn())
                    {
                        return ControlledPawn;
                    }
                }
            }
        }

        return TargetActor;
    }

    void HandleZeroHealthDeath(AActor* TargetActor, AActor* SourceActor, UAbilitySystemComponent* TargetASC, const FGameplayAttribute& Attribute)
    {
        if (!TargetActor || !TargetASC || !Attribute.IsValid())
        {
            return;
        }

        if (Attribute != UNiosCore_AttributeSet::GetHealthAttribute())
        {
            return;
        }

        if (TargetASC->GetNumericAttribute(Attribute) > 0.f)
        {
            return;
        }

        AActor* LifecycleActor = ResolveLifecycleActorForTargetASC(TargetASC, TargetActor);
        if (LifecycleActor && LifecycleActor->GetClass()->ImplementsInterface(UNiosGASActorInterface::StaticClass()))
        {
            INiosGASActorInterface::Execute_HandleDeath(LifecycleActor, SourceActor);
        }
    }
}

void FNiosCoreSkillEffectProcessor::ApplyEffectsByIndices(
    const FNiosCoreSkillEffectProcessParams& Params,
    const TArray<int32>& EffectIndices)
{
    const UNiosCore_SkillDataAsset* Skill = Params.Skill;
    if (!Skill || !Params.SourceASC || !Params.SourceActor || !Params.TargetData)
    {
        return;
    }

    auto ShouldExecuteEffect = [&EffectIndices](int32 EffectIndex)
    {
        return EffectIndices.Num() <= 0 || EffectIndices.Contains(EffectIndex);
    };

    for (int32 EffectIndex = 0; EffectIndex < Skill->SkillEffects.Num(); ++EffectIndex)
    {
        if (!ShouldExecuteEffect(EffectIndex))
        {
            continue;
        }

        const FNiosSkillEffectDefinition& Effect = Skill->SkillEffects[EffectIndex];
        if (!Effect.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL EFFECT SKIPPED INVALID: Skill=%s Index=%d EffectType=%d Attribute=%s BaseValue=%.2f Scalings=%d"),
                *GetNameSafe(Skill),
                EffectIndex,
                static_cast<int32>(Effect.EffectType),
                *Effect.Attribute.GetName(),
                Effect.BaseValue,
                Effect.Scalings.Num());
            continue;
        }

        TArray<UAbilitySystemComponent*> TargetASCs;
        const ENiosSkillEffectTarget EffectiveTarget = FNiosCoreSkillTargetResolver::ResolveEffectiveEffectTarget(Skill, Effect);
        const ENiosCoreTargetApplicationPolicy TargetPolicy = ResolveTargetPolicy(Params.World, Skill, Effect);

        if (Params.OverrideTargetASCs.Num() > 0 && EffectiveTarget != ENiosSkillEffectTarget::Source)
        {
            TargetASCs = Params.OverrideTargetASCs;
        }
        else
        {
            FNiosCoreSkillTargetResolver::ResolveEffectTargetASCs(
                Params.World,
                Params.SourceASC,
                Params.SourceActor,
                Params.SelectedActor,
                Skill,
                Effect,
                *Params.TargetData,
                TargetASCs);
        }

        if (TargetASCs.Num() <= 0)
        {
            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL EFFECT HAS NO TARGETS: Skill=%s Index=%d EffectTarget=%d EffectiveTarget=%d TargetPolicy=%d SkillTargetType=%d"),
                *GetNameSafe(Skill),
                EffectIndex,
                static_cast<int32>(Effect.Target),
                static_cast<int32>(EffectiveTarget),
                static_cast<int32>(TargetPolicy),
                static_cast<int32>(Skill->TargetType));
            continue;
        }

        const bool bIsStatusEffect = Effect.EffectType == ENiosSkillEffectType::ApplyStatusEffect || Effect.EffectType == ENiosSkillEffectType::RemoveStatusEffect;
        const float RawValue = bIsStatusEffect ? 0.f : CalculateEffectValue(Params.SourceASC, Effect);
        if (!bIsStatusEffect && FMath::IsNearlyZero(RawValue))
        {
            continue;
        }

        const float Delta = bIsStatusEffect ? 0.f : NormalizeDelta(Effect.EffectType, RawValue);

        for (UAbilitySystemComponent* TargetASC : TargetASCs)
        {
            if (!TargetASC)
            {
                continue;
            }

            AActor* TargetActor = TargetASC->GetAvatarActor();
            if (!TargetActor)
            {
                TargetActor = TargetASC->GetOwnerActor();
            }

            if (!IsTargetAllowedForPolicy(Params.World, Params.SourceActor, Params.SourceASC, TargetActor, TargetASC, TargetPolicy))
            {
                UE_LOG(LogTemp, Warning, TEXT(">>> SKILL EFFECT BLOCKED BY EFFECT TARGET POLICY: Skill=%s Index=%d EffectType=%d TargetPolicy=%d Source=%s Target=%s RelationAffectType=%d"),
                    *GetNameSafe(Skill),
                    EffectIndex,
                    static_cast<int32>(Effect.EffectType),
                    static_cast<int32>(TargetPolicy),
                    *GetNameSafe(Params.SourceActor),
                    *GetNameSafe(TargetActor),
                    static_cast<int32>(Skill->AffectType));
                continue;
            }

            if (bIsStatusEffect)
            {
                UNiosCore_StatusEffectComponent* StatusEffects = ResolveStatusEffectComponentFromASC(TargetASC);
                FNiosStatusEffectCatalogEntry CatalogEntry;
                UNiosCore_StatusEffectDataAsset* StatusEffectData = ResolveStatusEffectData(Params.World, Effect, &CatalogEntry);
                const FName ResolvedEffectID = !Effect.StatusEffectID.IsNone() ? Effect.StatusEffectID : (Effect.EffectID.IsNone() ? FName(*GetNameSafe(StatusEffectData)) : Effect.EffectID);

                if (!StatusEffectData || ResolvedEffectID.IsNone())
                {
                    UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT SKIPPED: Missing StatusEffectID/catalog entry Skill=%s Index=%d StatusEffectID=%s TargetASC=%s TargetActor=%s"),
                        *GetNameSafe(Skill),
                        EffectIndex,
                        *Effect.StatusEffectID.ToString(),
                        *GetNameSafe(TargetASC),
                        *GetNameSafe(TargetActor));
                    continue;
                }

                if (!StatusEffects)
                {
                    UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT SKIPPED: Target has no StatusEffectComponent Skill=%s StatusEffectID=%s TargetASC=%s TargetActor=%s OwnerActor=%s AvatarActor=%s"),
                        *GetNameSafe(Skill),
                        *ResolvedEffectID.ToString(),
                        *GetNameSafe(TargetASC),
                        *GetNameSafe(TargetActor),
                        *GetNameSafe(TargetASC->GetOwnerActor()),
                        *GetNameSafe(TargetASC->GetAvatarActor()));
                    continue;
                }

                const FName SourceID = Effect.EffectID.IsNone() ? ResolvedEffectID : Effect.EffectID;

                if (Effect.EffectType == ENiosSkillEffectType::ApplyStatusEffect)
                {
                    StatusEffects->ApplyStatusEffectByID(ResolvedEffectID, Params.SourceActor, ENiosStatusEffectSourceType::Skill, SourceID);
                }
                else
                {
                    StatusEffects->RemoveStatusEffectsBySource(ENiosStatusEffectSourceType::Skill, SourceID);
                }

                if (Params.OnEffectApplied)
                {
                    Params.OnEffectApplied(TargetASC, Effect);
                }

                UE_LOG(LogTemp, Warning, TEXT(">>> STATUS EFFECT PROCESSED: Skill=%s EffectType=%d StatusEffect=%s SourceID=%s TargetASC=%s TargetActor=%s SourceActor=%s"),
                    *GetNameSafe(Skill),
                    static_cast<int32>(Effect.EffectType),
                    *ResolvedEffectID.ToString(),
                    *SourceID.ToString(),
                    *GetNameSafe(TargetASC),
                    *GetNameSafe(TargetActor),
                    *GetNameSafe(Params.SourceActor));
                continue;
            }

            if (!Effect.Attribute.IsValid())
            {
                continue;
            }

            const float BeforeValue = TargetASC->GetNumericAttribute(Effect.Attribute);
            if (Effect.EffectType == ENiosSkillEffectType::Damage)
            {
                NotifyDamageListeners(TargetActor, Params.SourceActor, FMath::Abs(Delta));
                UNiosCore_LootSourceComponent::ReportDamageForLoot(TargetActor, Params.SourceActor, FMath::Abs(Delta));
            }

            float AfterValue = 0.f;
            if (!Nios_ApplyClampedDeltaToCurrentResource(TargetASC, Effect.Attribute, Delta, AfterValue))
            {
                TargetASC->ApplyModToAttribute(Effect.Attribute, EGameplayModOp::Additive, Delta);
                ClampResourceAttribute(TargetASC, Effect.Attribute);
                AfterValue = TargetASC->GetNumericAttribute(Effect.Attribute);
            }

            if (Effect.EffectType == ENiosSkillEffectType::Damage)
            {
                HandleZeroHealthDeath(TargetActor, Params.SourceActor, TargetASC, Effect.Attribute);
                if (Effect.Attribute == UNiosCore_AttributeSet::GetHealthAttribute() && TargetASC->GetNumericAttribute(Effect.Attribute) <= 0.f)
                {
                    UNiosCore_LootSourceComponent::NotifyActorDiedForLoot(TargetActor);
                }
            }

            if (Params.OnEffectApplied)
            {
                Params.OnEffectApplied(TargetASC, Effect);
            }

            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL EFFECT APPLIED: EffectType=%d EffectTarget=%d EffectiveTarget=%d Attribute=%s Raw=%.2f Delta=%.2f Before=%.2f After=%.2f TargetASC=%s TargetActor=%s SourceActor=%s"),
                static_cast<int32>(Effect.EffectType),
                static_cast<int32>(Effect.Target),
                static_cast<int32>(EffectiveTarget),
                *Effect.Attribute.GetName(),
                RawValue,
                Delta,
                BeforeValue,
                AfterValue,
                *GetNameSafe(TargetASC),
                *GetNameSafe(TargetActor),
                *GetNameSafe(Params.SourceActor));
        }
    }
}

float FNiosCoreSkillEffectProcessor::CalculateEffectValue(
    UAbilitySystemComponent* SourceASC,
    const FNiosSkillEffectDefinition& Effect)
{
    float Value = Effect.BaseValue;

    if (!SourceASC)
    {
        return Value;
    }

    for (const FNiosSkillStatScaling& Scaling : Effect.Scalings)
    {
        if (!Scaling.SourceAttribute.IsValid() || FMath::IsNearlyZero(Scaling.Scale))
        {
            continue;
        }

        const float SourceValue = SourceASC->GetNumericAttribute(Scaling.SourceAttribute);
        Value += SourceValue * Scaling.Scale;
    }

    return Value;
}

float FNiosCoreSkillEffectProcessor::NormalizeDelta(
    ENiosSkillEffectType EffectType,
    float RawValue)
{
    switch (EffectType)
    {
    case ENiosSkillEffectType::Damage:
        return -FMath::Abs(RawValue);

    case ENiosSkillEffectType::Heal:
        return FMath::Abs(RawValue);

    case ENiosSkillEffectType::ResourceDelta:
    case ENiosSkillEffectType::AttributeDelta:
    case ENiosSkillEffectType::ApplyStatusEffect:
    case ENiosSkillEffectType::RemoveStatusEffect:
    default:
        return RawValue;
    }
}


void FNiosCoreSkillEffectProcessor::ClampResourceAttribute(
    UAbilitySystemComponent* ASC,
    FGameplayAttribute Attribute)
{
    if (!ASC || !Attribute.IsValid())
    {
        return;
    }

    auto ClampCurrent = [ASC](FGameplayAttribute ResourceAttribute, FGameplayAttribute MaxAttribute)
    {
        const float CurrentValue = ASC->GetNumericAttribute(ResourceAttribute);
        const float MaxValue = FMath::Max(0.f, ASC->GetNumericAttribute(MaxAttribute));
        const float ClampedValue = FMath::Clamp(CurrentValue, 0.f, MaxValue);

        if (!FMath::IsNearlyEqual(CurrentValue, ClampedValue))
        {
            if (UNiosCore_AbilitySystemComponent* NiosASC = Cast<UNiosCore_AbilitySystemComponent>(ASC))
            {
                NiosASC->SetAttributeValue(ResourceAttribute, ClampedValue);
            }
            else
            {
                ASC->SetNumericAttributeBase(ResourceAttribute, ClampedValue);
            }

            UE_LOG(LogTemp, Warning, TEXT(">>> RESOURCE CLAMPED: Attribute=%s Before=%.2f After=%.2f Max=%.2f ASC=%s"),
                *ResourceAttribute.GetName(),
                CurrentValue,
                ClampedValue,
                MaxValue,
                *GetNameSafe(ASC));
        }
    };

    FGameplayAttribute MaxAttribute;
    if (Nios_SkillEffect_GetMaxAttributeForResourceAttribute(Attribute, MaxAttribute))
    {
        ClampCurrent(Attribute, MaxAttribute);
        return;
    }

    FGameplayAttribute ResourceAttribute;
    if (Nios_SkillEffect_GetResourceAttributeForMaxAttribute(Attribute, ResourceAttribute))
    {
        ClampCurrent(ResourceAttribute, Attribute);
    }
}

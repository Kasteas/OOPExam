#include "AbilitySystem/Pipeline/NiosCore_SkillTargetResolver.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "AbilitySystem/Skills/NiosCore_SkillDataAsset.h"
#include "AbilitySystem/Attributes/NiosCore_AttributeSet.h"
#include "AbilitySystem/Skills/NiosCore_SkillDeliveryTypes.h"
#include "Rules/Targeting/NiosTargetRulesSubsystem.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "UObject/UnrealType.h"

namespace
{
    static const FName NAME_CombatRangeRadiusOverride(TEXT("CombatRangeRadiusOverride"));
    static const FName NAME_NiosCombatRangeRadius(TEXT("NiosCombatRangeRadius"));
    static const FName NAME_CombatRadius(TEXT("CombatRadius"));

    float ReadFloatPropertyByName(const UObject* Object, const FName PropertyName)
    {
        if (!Object || PropertyName.IsNone())
        {
            return 0.f;
        }

        if (const FFloatProperty* FloatProperty = FindFProperty<FFloatProperty>(Object->GetClass(), PropertyName))
        {
            return FMath::Max(0.f, FloatProperty->GetPropertyValue_InContainer(Object));
        }

        if (const FDoubleProperty* DoubleProperty = FindFProperty<FDoubleProperty>(Object->GetClass(), PropertyName))
        {
            return FMath::Max(0.f, static_cast<float>(DoubleProperty->GetPropertyValue_InContainer(Object)));
        }

        return 0.f;
    }

    float ReadDesignerCombatRadiusOverride(const AActor* Actor)
    {
        if (!Actor)
        {
            return 0.f;
        }

        const FName CandidateNames[] =
        {
            NAME_CombatRangeRadiusOverride,
            NAME_NiosCombatRangeRadius,
            NAME_CombatRadius
        };

        for (const FName CandidateName : CandidateNames)
        {
            const float Radius = ReadFloatPropertyByName(Actor, CandidateName);
            if (Radius > KINDA_SMALL_NUMBER)
            {
                return Radius;
            }
        }

        return 0.f;
    }
}


bool FNiosCoreSkillTargetResolver::BuildActorTargetData(AActor* TargetActor, FGameplayAbilityTargetDataHandle& OutTargetData)
{
    OutTargetData.Clear();

    if (!TargetActor)
    {
        return false;
    }

    UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
    if (!TargetASC)
    {
        return false;
    }

    FGameplayAbilityTargetData_ActorArray* Data = new FGameplayAbilityTargetData_ActorArray();
    Data->TargetActorArray.Add(TargetActor);
    OutTargetData.Add(Data);

    return OutTargetData.Num() > 0;
}

FNiosCoreSkillTargetContext FNiosCoreSkillTargetResolver::ResolveContext(
    UWorld* World,
    UAbilitySystemComponent* SourceASC,
    AActor* SourceActor,
    AActor* SelectedActor,
    const UNiosCore_SkillDataAsset* Skill,
    const FGameplayAbilityTargetDataHandle& TargetData)
{
    FNiosCoreSkillTargetContext Context;
    Context.SourceActor = SourceActor;
    Context.SourceASC = SourceASC;
    Context.SelectedActor = SelectedActor;

    if (!Skill)
    {
        Context.FailureReason = TEXT("NoSkill");
        return Context;
    }

    if (!SourceActor || !SourceASC)
    {
        Context.FailureReason = TEXT("NoSource");
        return Context;
    }

    switch (Skill->TargetType)
    {
    case ENiosAbilityTargetType::None:
    case ENiosAbilityTargetType::Self:
        Context.PrimaryTargetActor = SourceActor;
        Context.PrimaryTargetASC = SourceASC;
        Context.TargetLocation = SourceActor->GetActorLocation();
        break;

    case ENiosAbilityTargetType::Actor:
        if (TargetData.Num() > 0)
        {
            for (int32 Index = 0; Index < TargetData.Num() && !Context.PrimaryTargetActor; ++Index)
            {
                const FGameplayAbilityTargetData* Data = TargetData.Get(Index);
                if (!Data)
                {
                    continue;
                }

                for (const TWeakObjectPtr<AActor>& ActorPtr : Data->GetActors())
                {
                    AActor* Actor = ActorPtr.Get();
                    if (!Actor)
                    {
                        continue;
                    }

                    UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
                    if (!ASC)
                    {
                        continue;
                    }

                    Context.PrimaryTargetActor = Actor;
                    Context.PrimaryTargetASC = ASC;
                    Context.TargetLocation = Actor->GetActorLocation();
                    break;
                }
            }
        }

        if (!Context.PrimaryTargetActor && SelectedActor)
        {
            Context.PrimaryTargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelectedActor);
            if (Context.PrimaryTargetASC)
            {
                Context.PrimaryTargetActor = SelectedActor;
                Context.TargetLocation = SelectedActor->GetActorLocation();
            }
        }
        break;

    case ENiosAbilityTargetType::GroundPoint:
        Context.PrimaryTargetActor = SelectedActor;
        Context.PrimaryTargetASC = SelectedActor ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelectedActor) : nullptr;
        Context.TargetLocation = ResolvePoint(SourceActor, SelectedActor, TargetData, ENiosSkillPointSource::ProvidedPoint);
        break;

    default:
        break;
    }

    Context.bIsSelf = Context.PrimaryTargetActor && Context.PrimaryTargetActor == SourceActor;

    if (World && Context.PrimaryTargetActor)
    {
        if (UNiosTargetRulesSubsystem* Rules = World->GetSubsystem<UNiosTargetRulesSubsystem>())
        {
            Context.RelationToSource = Rules->GetRelation(SourceActor, Context.PrimaryTargetActor);
        }
    }

    Context.bIsValid = true;
    return Context;
}

bool FNiosCoreSkillTargetResolver::ValidateAbilityTarget(
    UWorld* World,
    UAbilitySystemComponent* SourceASC,
    AActor* SourceActor,
    AActor* SelectedActor,
    const UNiosCore_SkillDataAsset* Skill,
    const FGameplayAbilityTargetDataHandle& TargetData,
    FName& OutFailureReason)
{
    OutFailureReason = NAME_None;

    if (!Skill)
    {
        OutFailureReason = TEXT("NoSkill");
        return false;
    }

    if (!SourceASC || !SourceActor)
    {
        OutFailureReason = TEXT("NoSource");
        return false;
    }

    if (Skill->TargetType == ENiosAbilityTargetType::None || Skill->TargetType == ENiosAbilityTargetType::GroundPoint)
    {
        return true;
    }

    if (Skill->TargetType == ENiosAbilityTargetType::Self)
    {
        return true;
    }

    const FNiosCoreSkillTargetContext Context = ResolveContext(World, SourceASC, SourceActor, SelectedActor, Skill, TargetData);
    if (!Context.PrimaryTargetActor || !Context.PrimaryTargetASC)
    {
        OutFailureReason = TEXT("NoTargetASC");
        return false;
    }

    const float TargetMaxHealth = Context.PrimaryTargetASC->GetNumericAttribute(UNiosCore_AttributeSet::GetMaxHealthAttribute());
    if (TargetMaxHealth > KINDA_SMALL_NUMBER && Context.PrimaryTargetASC->GetNumericAttribute(UNiosCore_AttributeSet::GetHealthAttribute()) <= KINDA_SMALL_NUMBER)
    {
        OutFailureReason = TEXT("TargetDead");
        return false;
    }

    if (!World)
    {
        OutFailureReason = TEXT("NoWorld");
        return false;
    }

    UNiosTargetRulesSubsystem* Rules = World->GetSubsystem<UNiosTargetRulesSubsystem>();
    if (!Rules)
    {
        OutFailureReason = TEXT("NoTargetRulesSubsystem");
        return false;
    }

    if (!Rules->CanUseSkillOnTarget(SourceActor, Context.PrimaryTargetActor, Skill))
    {
        OutFailureReason = TEXT("RelationBlocked");
        return false;
    }

    if (Skill->HasRange())
    {
        const float CurrentDistance = CalculateActorRangeDistance(SourceActor, Context.PrimaryTargetActor, Skill);
        const float AllowedDistance = Skill->GetServerValidatedRange();

        if (CurrentDistance > AllowedDistance + KINDA_SMALL_NUMBER)
        {
            OutFailureReason = TEXT("TargetOutOfRange");
            UE_LOG(LogTemp, Warning, TEXT(">>> SKILL TARGET RANGE FAILED: Skill=%s Source=%s Target=%s Distance=%.2f Range=%.2f Tolerance=%.2f Allowed=%.2f"),
                *GetNameSafe(Skill),
                *GetNameSafe(SourceActor),
                *GetNameSafe(Context.PrimaryTargetActor),
                CurrentDistance,
                Skill->GetRange(),
                Skill->GetServerRangeTolerance(),
                AllowedDistance);
            return false;
        }
    }

    return true;
}

float FNiosCoreSkillTargetResolver::CalculateActorRangeDistance(
    const AActor* SourceActor,
    const AActor* TargetActor,
    const UNiosCore_SkillDataAsset* Skill)
{
    if (!SourceActor || !TargetActor)
    {
        return 0.f;
    }

    const FVector SourceLocation = SourceActor->GetActorLocation();
    const FVector TargetLocation = TargetActor->GetActorLocation();

    float Distance = 0.f;
    if (Skill && Skill->bUse2DRangeCheck)
    {
        Distance = FVector::Dist2D(SourceLocation, TargetLocation);
    }
    else
    {
        Distance = FVector::Distance(SourceLocation, TargetLocation);
    }

    if (Skill && Skill->bSubtractActorCollisionRadiusFromRange)
    {
        Distance -= ResolveActorRangeRadius(SourceActor, Skill);
        Distance -= ResolveActorRangeRadius(TargetActor, Skill);
    }

    return FMath::Max(0.f, Distance);
}

float FNiosCoreSkillTargetResolver::ResolveActorRangeRadius(
    const AActor* Actor,
    const UNiosCore_SkillDataAsset* Skill)
{
    if (!Actor)
    {
        return 0.f;
    }

    // Explicit designer value wins. This lets bosses, dragons, vehicles, etc. define
    // their attackable combat radius independently from pivot/capsule/visual bounds.
    float Radius = ReadDesignerCombatRadiusOverride(Actor);

    if (Radius <= KINDA_SMALL_NUMBER)
    {
        Radius = FMath::Max(0.f, Actor->GetSimpleCollisionRadius());

        if (Skill && Skill->bUseActorBoundsForRangeRadius)
        {
            FVector BoundsOrigin = FVector::ZeroVector;
            FVector BoundsExtent = FVector::ZeroVector;
            Actor->GetActorBounds(false, BoundsOrigin, BoundsExtent, false);

            const float BoundsRadius = Skill->bUse2DRangeCheck
                ? FMath::Max(BoundsExtent.X, BoundsExtent.Y)
                : BoundsExtent.Size();

            Radius = FMath::Max(Radius, BoundsRadius);
        }
    }

    if (Skill && Skill->MaxActorRangeRadiusContribution > KINDA_SMALL_NUMBER)
    {
        Radius = FMath::Min(Radius, Skill->MaxActorRangeRadiusContribution);
    }

    return FMath::Max(0.f, Radius);
}

ENiosSkillEffectTarget FNiosCoreSkillTargetResolver::ResolveEffectiveEffectTarget(
    const UNiosCore_SkillDataAsset* Skill,
    const FNiosSkillEffectDefinition& Effect)
{
    if (Effect.Target != ENiosSkillEffectTarget::Auto)
    {
        return Effect.Target;
    }

    if (!Skill)
    {
        return ENiosSkillEffectTarget::Source;
    }

    switch (Skill->TargetType)
    {
    case ENiosAbilityTargetType::Actor:
        return ENiosSkillEffectTarget::SelectedTarget;
    case ENiosAbilityTargetType::GroundPoint:
        return ENiosSkillEffectTarget::AllCachedTargets;
    case ENiosAbilityTargetType::Self:
    case ENiosAbilityTargetType::None:
    default:
        return ENiosSkillEffectTarget::Source;
    }
}

void FNiosCoreSkillTargetResolver::ResolveEffectTargetASCs(
    UWorld* World,
    UAbilitySystemComponent* SourceASC,
    AActor* SourceActor,
    AActor* SelectedActor,
    const UNiosCore_SkillDataAsset* Skill,
    const FNiosSkillEffectDefinition& Effect,
    const FGameplayAbilityTargetDataHandle& TargetData,
    TArray<UAbilitySystemComponent*>& OutTargetASCs)
{
    OutTargetASCs.Reset();

    const ENiosSkillEffectTarget EffectiveTarget = ResolveEffectiveEffectTarget(Skill, Effect);

    if (EffectiveTarget == ENiosSkillEffectTarget::Source)
    {
        if (SourceASC)
        {
            OutTargetASCs.AddUnique(SourceASC);
        }
        return;
    }

    if (EffectiveTarget == ENiosSkillEffectTarget::SelectedTarget || EffectiveTarget == ENiosSkillEffectTarget::AllCachedTargets)
    {
        for (int32 Index = 0; Index < TargetData.Num(); ++Index)
        {
            const FGameplayAbilityTargetData* Data = TargetData.Get(Index);
            if (!Data)
            {
                continue;
            }

            for (const TWeakObjectPtr<AActor>& ActorPtr : Data->GetActors())
            {
                AActor* Actor = ActorPtr.Get();
                if (!Actor)
                {
                    continue;
                }

                UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
                if (TargetASC)
                {
                    OutTargetASCs.AddUnique(TargetASC);
                }
            }

            if (EffectiveTarget == ENiosSkillEffectTarget::SelectedTarget && OutTargetASCs.Num() > 0)
            {
                return;
            }
        }

        if (OutTargetASCs.Num() == 0 && SelectedActor)
        {
            if (UAbilitySystemComponent* TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SelectedActor))
            {
                OutTargetASCs.AddUnique(TargetASC);
            }
        }
    }
}

FVector FNiosCoreSkillTargetResolver::ResolvePoint(
    AActor* SourceActor,
    AActor* SelectedActor,
    const FGameplayAbilityTargetDataHandle& TargetData,
    ENiosSkillPointSource PointSource)
{
    switch (PointSource)
    {
    case ENiosSkillPointSource::CasterLocation:
        return SourceActor ? SourceActor->GetActorLocation() : FVector::ZeroVector;

    case ENiosSkillPointSource::SelectedTargetLocation:
        return SelectedActor ? SelectedActor->GetActorLocation() : (SourceActor ? SourceActor->GetActorLocation() : FVector::ZeroVector);

    case ENiosSkillPointSource::ProvidedPoint:
    default:
        if (TargetData.Num() > 0)
        {
            const FGameplayAbilityTargetData* Data = TargetData.Get(0);
            if (Data && Data->HasHitResult())
            {
                return Data->GetHitResult()->ImpactPoint;
            }
        }
        return SelectedActor ? SelectedActor->GetActorLocation() : (SourceActor ? SourceActor->GetActorLocation() : FVector::ZeroVector);
    }
}

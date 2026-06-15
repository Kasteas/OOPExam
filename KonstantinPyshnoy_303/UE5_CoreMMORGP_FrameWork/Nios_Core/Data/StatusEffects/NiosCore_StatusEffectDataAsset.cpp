#include "Data/StatusEffects/NiosCore_StatusEffectDataAsset.h"

bool UNiosCore_StatusEffectDataAsset::HasPeriodicOperations() const
{
    if (TickInterval <= 0.f)
    {
        return false;
    }

    for (const FNiosStatusEffectOperation& Operation : Operations)
    {
        if (Operation.Type == ENiosStatusEffectOperationType::PeriodicAttributeDelta)
        {
            return true;
        }
    }

    return false;
}

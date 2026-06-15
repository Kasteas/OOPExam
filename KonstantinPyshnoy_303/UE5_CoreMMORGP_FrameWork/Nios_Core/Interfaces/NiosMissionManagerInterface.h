#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "NiosMissionManagerInterface.generated.h"

class UMOMissionsManager;

/**
 * Lightweight access interface for player-owned mission services.
 *
 * Implemented by Nios PlayerState as the authoritative owner and by Character as a proxy
 * that forwards to its PlayerState. This mirrors the GAS access pattern and keeps gameplay
 * systems from hardcoding where the mission component lives.
 */
UINTERFACE(BlueprintType)
class NIOS_CORE_API UNiosMissionManagerInterface : public UInterface
{
    GENERATED_BODY()
};

class NIOS_CORE_API INiosMissionManagerInterface
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "Nios|Missions")
    UMOMissionsManager* GetNiosMissionManager() const;
};

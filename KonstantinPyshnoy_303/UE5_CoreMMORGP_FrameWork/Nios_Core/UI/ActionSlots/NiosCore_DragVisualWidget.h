#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Core/Nios_CoreStructers.h"
#include "NiosCore_DragVisualWidget.generated.h"

class UImage;

UCLASS()
class NIOS_CORE_API UNiosCore_DragVisualWidget : public UUserWidget
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable)
    void SetVisualData(
        const FNiosActionSlotVisualData& InVisualData
    );

protected:

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UImage> Image_Icon;
};
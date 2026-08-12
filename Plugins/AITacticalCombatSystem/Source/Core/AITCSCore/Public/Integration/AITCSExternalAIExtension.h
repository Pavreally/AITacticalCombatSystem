// Pavel Gornostaev <https://github.com/Pavreally>
#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AITCSExternalAIExtension.generated.h"

/**
 * Extension hook that allows external AI systems to receive AITCS tactical orders.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class AITCSCORE_API UAITCSExternalAIExtension : public UObject
{
    GENERATED_BODY()

public:

    /** Unique identifier for this external AI extension. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AITCS|Integration")
    FName ExtensionId;

    /** Called when the extension receives a tactical order from AITCS. */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "AITCS|Integration")
    void OnTacticalOrderReceived();
    virtual void OnTacticalOrderReceived_Implementation();
};

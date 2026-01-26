#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "GASPAnimInstanceProxy.generated.h"

class AGASPCharacter;
/**
 * 
 */
USTRUCT()
struct FGASPAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGASPAnimInstanceProxy() = default;

	explicit FGASPAnimInstanceProxy(UAnimInstance* InAnimInstance);

protected:
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;
};

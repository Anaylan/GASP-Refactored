#include "Animation/GASPAnimInstanceProxy.h"
#include "Animation/GASPAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPAnimInstanceProxy)

FGASPAnimInstanceProxy::FGASPAnimInstanceProxy(UAnimInstance* InAnimInstance)
	: FAnimInstanceProxy{InAnimInstance}
{
}

void FGASPAnimInstanceProxy::PostUpdate(UAnimInstance* InAnimInstance) const
{
	FAnimInstanceProxy::PostUpdate(InAnimInstance);

	auto* AnimInstance{Cast<UGASPAnimInstance>(InAnimInstance)};
	if (IsValid(AnimInstance))
	{
		AnimInstance->NativePostUpdateAnimation();
	}
}

#include "Animation/GASPAnimInstanceProxy.h"
#include "Actors/GASPCharacter.h"
#include "Animation/GASPAnimInstance.h"
#include "MovementSet/GASPMoverComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPAnimInstanceProxy)

FGASPAnimInstanceProxy::FGASPAnimInstanceProxy(UAnimInstance* InAnimInstance)
	: FAnimInstanceProxy{InAnimInstance}
{
}

void FGASPAnimInstanceProxy::InitializeObjects(UAnimInstance* InAnimInstance)
{
	FAnimInstanceProxy::InitializeObjects(InAnimInstance);

	auto Owner = InAnimInstance->TryGetPawnOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	Character = Cast<AGASPCharacter>(Owner);
	if (Character.IsValid())
	{
		MoverComponent = Character->GetMoverComponent();
	}
}

void FGASPAnimInstanceProxy::PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds)
{
	FAnimInstanceProxy::PreUpdate(InAnimInstance, DeltaSeconds);
	
	if (!Character.IsValid() || !MoverComponent.IsValid())
	{
		return;
	}
}

void FGASPAnimInstanceProxy::Update(float DeltaSeconds)
{
	FAnimInstanceProxy::Update(DeltaSeconds);
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

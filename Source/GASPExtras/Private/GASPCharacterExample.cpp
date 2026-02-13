#include "GASPCharacterExample.h"
#include "Components/GASPTraversalComponent.h"
#include "GameFramework/GameplayCameraComponent.h"

// Sets default values
AGASPCharacterExample::AGASPCharacterExample(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameplayCamera = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCamera"));

	if (GetMesh())
	{
		GameplayCamera->SetupAttachment(GetMesh(), FName{TEXT("root")});
	}
}

void AGASPCharacterExample::PossessedBy(AController* NewController)
{
	if (auto* PC = Cast<APlayerController>(NewController))
	{
		GameplayCamera->ActivateCameraForPlayerController(PC);
	}

	Super::PossessedBy(NewController);
}

void AGASPCharacterExample::OnRep_Controller()
{
	if (auto* PC = Cast<APlayerController>(GetController()))
	{
		GameplayCamera->ActivateCameraForPlayerController(PC);
	}

	Super::OnRep_Controller();
}

void AGASPCharacterExample::SprintAction(bool bPressed)
{
	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (bPressed)
	{
		InputState->DesiredGait = GaitTags::Sprint;
		if (GetStanceMode() == StanceTags::Standing)
		{
			InputState->DesiredStance = StanceTags::Standing;
		}
		return;
	}
	InputState->DesiredGait = GaitTags::Run;
}

void AGASPCharacterExample::WalkAction(bool bPressed)
{
	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (InputState->DesiredGait != GaitTags::Sprint)
	{
		if (InputState->DesiredGait != GaitTags::Walk)
		{
			InputState->DesiredGait = GaitTags::Walk;
		}
		else
		{
			InputState->DesiredGait = GaitTags::Run;
		}
	}
}

void AGASPCharacterExample::CrouchAction(bool bPressed)
{
	if (GetMovementMode() == MovementModeTags::Grounded || GetMovementMode() == MovementModeTags::Slide)
	{
		auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
		if (GetStanceMode() == StanceTags::Crouching)
		{
			InputState->DesiredStance = StanceTags::Standing;
		}
		else
		{
			InputState->DesiredStance = StanceTags::Crouching;
		}
	}
}

void AGASPCharacterExample::JumpAction(bool bPressed)
{
	if (GetLocomotionAction() == LocomotionActionTags::Ragdoll)
	{
		StopRagdolling();
		return;
	}
	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (bPressed && !IsDoingTraversal())
	{
		if (const auto [bTraversalCheckFailed, bMontageSelectionFailed] = TryTraversalAction(); bTraversalCheckFailed ||
			bMontageSelectionFailed)
		{
			if (GetStanceMode() != StanceTags::Standing)
			{
				InputState->DesiredStance = StanceTags::Standing;
			}
			else
			{
				Jump();
				FTimerHandle TimerHandle;
				GetWorldTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateWeakLambda(this, [this]()
				{
					StopJumping();
				}), .1f, false);
			}
		}
	}
}

void AGASPCharacterExample::AimAction(bool bPressed)
{
	
	auto* InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (bPressed)
	{
		InputState->DesiredRotationMode = RotationTags::Aim;
	}
	else
	{
		InputState->DesiredRotationMode = RotationTags::OrientToMovement;
	}
}

void AGASPCharacterExample::RagdollAction(bool bPressed)
{
	if (bPressed)
	{
		StartRagdolling();
	}
	else
	{
		StopRagdolling();
	}
}

void AGASPCharacterExample::StrafeAction(bool bPressed)
{
	auto InputState{PlayerInputState.GetMutablePtr<FGASPInputState>()};
	if (MoverInputs_PostSim.RotationMode != RotationTags::Strafe)
	{
		InputState->DesiredRotationMode = RotationTags::Strafe;
	}
	else
	{
		InputState->DesiredRotationMode = RotationTags::OrientToMovement;
	}
}

void AGASPCharacterExample::MoveAction(const FVector2D& Value)
{
	if (!IsLocallyControlled())
	{
		return;
	}

	AddMovementInput({Value.X, Value.Y, 0.f});
}

void AGASPCharacterExample::LookAction(const FVector2D& Value)
{
	if (!TwinStickMode)
	{
		AddControllerYawInput(Value.X);
		AddControllerPitchInput(-1 * Value.Y);
	}
}

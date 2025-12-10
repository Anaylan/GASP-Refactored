#include "GASPCharacterExample.h"

#include "Components/GASPTraversalComponent.h"
#include "GameFramework/GameplayCameraComponent.h"

// Sets default values
AGASPCharacterExample::AGASPCharacterExample(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bReplicates = true;

	GameplayCamera = CreateDefaultSubobject<UGameplayCameraComponent>(TEXT("GameplayCamera"));
	GameplayCamera->SetIsReplicated(true);

	if (GetMesh())
	{
		GameplayCamera->SetupAttachment(GetMesh(), NAME_None);
		GameplayCamera->SetRelativeLocation(FVector::ZAxisVector * 100.f);
	}
}

void AGASPCharacterExample::PossessedBy(AController* NewController)
{
	if (APlayerController* PC = Cast<APlayerController>(NewController))
	{
		GameplayCamera->ActivateCameraForPlayerController(PC);
	}

	Super::PossessedBy(NewController);
}

void AGASPCharacterExample::OnRep_Controller()
{
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		GameplayCamera->ActivateCameraForPlayerController(PC);
	}

	Super::OnRep_Controller();
}

void AGASPCharacterExample::SprintAction(bool bPressed)
{
	if (bPressed)
	{
		PlayerInputState.DesiredGait = GaitTags::Sprint;
		if (StanceMode == StanceTags::Crouching)
		{
			PlayerInputState.DesiredStance = StanceTags::Standing;
		}
		return;
	}
	PlayerInputState.DesiredGait = GaitTags::Run;
}

void AGASPCharacterExample::WalkAction(bool bPressed)
{
	if (PlayerInputState.DesiredGait != GaitTags::Sprint)
	{
		if (PlayerInputState.DesiredGait != GaitTags::Walk)
		{
			PlayerInputState.DesiredGait = GaitTags::Walk;
		}
		else
		{
			PlayerInputState.DesiredGait = GaitTags::Run;
		}
	}
}

void AGASPCharacterExample::CrouchAction(bool bPressed)
{
	if (MovementMode == MovementModeTags::Grounded || MovementMode == MovementModeTags::Slide)
	{
		if (StanceMode == StanceTags::Crouching)
		{
			PlayerInputState.DesiredStance = StanceTags::Standing;
		}
		else
		{
			PlayerInputState.DesiredStance = StanceTags::Crouching;
		}
	}
}

void AGASPCharacterExample::JumpAction(bool bPressed)
{
	if (LocomotionAction == LocomotionActionTags::Ragdoll)
	{
		StopRagdolling();
		return;
	}

	if (bPressed && !IsDoingTraversal())
	{
		if (const auto [bTraversalCheckFailed, bMontageSelectionFailed] = TryTraversalAction(); bTraversalCheckFailed ||
			bMontageSelectionFailed)
		{
			if (GetStanceMode() != StanceTags::Standing)
			{
				PlayerInputState.DesiredStance = StanceTags::Standing;
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
	if (bPressed)
	{
		PlayerInputState.DesiredRotationMode = RotationTags::Aim;
	}
	else
	{
		PlayerInputState.DesiredRotationMode = RotationTags::OrientToMovement;
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
	if (RotationMode != RotationTags::Strafe)
	{
		PlayerInputState.DesiredRotationMode = RotationTags::Strafe;
	}
	else
	{
		PlayerInputState.DesiredRotationMode = RotationTags::OrientToMovement;
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

#include "Animation/Notifies//AnimNotify_FoleyEvent.h"
#include "BlueprintGameplayTagLibrary.h"
#include "Foley/GASPFoleyWorldSubsystem.h"
#include "Foley/GASPFootstepEffectsSet.h"
#include "Interfaces/GASPFoleyAudioBankInterface.h"
#include "Types/TagTypes.h"
#include "VisualLogger/VisualLoggerKismetLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_FoleyEvent)

namespace FoleyVars
{
	bool bDrawDebug{false};
	FAutoConsoleVariableRef EnabledStateMachineStruct(
		TEXT("gasp.DrawVisLogShapesForFoleySounds.enabled"), bDrawDebug, TEXT("enabled debug for foley"), ECVF_Default);
}

UAnimNotify_FoleyEvent::UAnimNotify_FoleyEvent()
{
	const TArray<FGameplayTag> MovementTagsList = {
		FoleyTags::Run,
		FoleyTags::RunBackwds,
		FoleyTags::RunStrafe,
		FoleyTags::Scuff,
		FoleyTags::ScuffPivot,
		FoleyTags::Walk,
		FoleyTags::WalkBackwds
	};

	MovementTags = FGameplayTagContainer::CreateFromArray(MovementTagsList);
}

void UAnimNotify_FoleyEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                    const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	auto* Owner = MeshComp->GetOwner();
	if (!IsValid(Owner))
	{
		return;
	}

	if (!CanPlayFootstepEffects(Owner) || !IsValid(DefaultBank))
	{
		return;
	}

	auto* WorldContext = Owner->GetWorld();
	const auto SocketTransform{MeshComp->GetSocketTransform(SocketName)};

	if (const auto Subsystem = WorldContext->GetSubsystem<UGASPFoleyWorldSubsystem>())
	{
		Subsystem->PlayFoleyEvent(DefaultBank, MeshComp, SocketName, bSpawnSound, bSpawnDecal, bSpawnParticleSystem,
		                          bSpawnInAir, TraceLength, VolumeMultiplier, PitchMultiplier);

#if WITH_EDITOR && ALLOW_CONSOLE
		if (!FoleyVars::bDrawDebug)
		{
			return;
		}

		Subsystem->DebugLog(SocketTransform, VisLogDebugColor, VisLogDebugText);
#endif
	}
}

FString UAnimNotify_FoleyEvent::GetNotifyName_Implementation() const
{
	const auto TagName = Event.ToString();
	FString RightPart;
	TagName.Split(TEXT("."), nullptr, &RightPart, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

	return FString::Printf(TEXT("FoleyEvent: %s"), RightPart.IsEmpty() ? *TagName : *RightPart);
}

bool UAnimNotify_FoleyEvent::CanPlayFootstepEffects(AActor* Owner) const
{
	if (MovementTags.HasTag(Event) && Owner->Implements<UGASPFoleyAudioBankInterface>())
	{
		return IGASPFoleyAudioBankInterface::Execute_CanPlayFootstepEffects(Owner);
	}

	return true;
}

#include "Animation/Notifies//AnimNotify_FoleyEvent.h"
#include "BlueprintGameplayTagLibrary.h"
#include "NiagaraFunctionLibrary.h"
#include "Components/DecalComponent.h"
#include "Foley/GASPFootstepEffectsSet.h"
#include "Interfaces/GASPFoleyAudioBankInterface.h"
#include "Kismet/GameplayStatics.h"
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

	FCollisionQueryParams QueryParams{__FUNCTION__, true, Owner};
	QueryParams.bReturnPhysicalMaterial = true;

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(Owner);
	FHitResult Hit;
	WorldContext->LineTraceSingleByChannel(Hit, SocketTransform.GetLocation(),
	                                       SocketTransform.GetLocation() - FVector::ZAxisVector * TraceLength,
	                                       ECC_Visibility, QueryParams);

	if (!Hit.bBlockingHit && !bSpawnInAir)
	{
		return;
	}

	const auto SurfaceType{Hit.PhysMaterial.IsValid() ? Hit.PhysMaterial->SurfaceType.GetValue() : SurfaceType_Default};
	const auto FootstepSettings{DefaultBank->GetFootstepSettingsFromSurface(SurfaceType)};
	if (!FAnimWeight::IsRelevant(VolumeMultiplier) || !FootstepSettings)
	{
		return;
	}

	const auto FootstepRotation{
		FRotationMatrix::MakeFromZY(Hit.ImpactNormal,
		                            SocketTransform.TransformVectorNoScale(FVector::UpVector)).Rotator()
	};

	if (bSpawnSound)
	{
		SpawnSound(MeshComp, FootstepSettings->SoundSettings, Hit.ImpactPoint);
	}
	if (bSpawnDecal)
	{
		SpawnDecal(MeshComp, FootstepSettings->DecalSettings, Hit.ImpactPoint, FootstepRotation, Hit);
	}
	if (bSpawnParticleSystem)
	{
		SpawnParticleSystem(MeshComp, FootstepSettings->ParticleSettings, Hit.ImpactPoint,
		                    FootstepRotation);
	}

#if WITH_EDITOR && ALLOW_CONSOLE
	if (!FoleyVars::bDrawDebug)
	{
		return;
	}

	FVisualLogger::SphereLogf(WorldContext, FName(TEXT("VisLogFoley")), ELogVerbosity::Log,
	                          SocketTransform.GetLocation(), 5.f, VisLogDebugColor.ToFColor(true), false, TEXT("%s"),
	                          *VisLogDebugText);
	DrawDebugSphere(WorldContext, SocketTransform.GetLocation(), 10.f, 12, VisLogDebugColor.ToRGBE(),
	                false, 4.f);
#endif
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

void UAnimNotify_FoleyEvent::SpawnSound(const USkeletalMeshComponent* Mesh,
                                        const FGASPFootstepSoundSettings& SoundSettings,
                                        const FVector& FootstepLocation) const
{
	if (!IsValid(SoundSettings.Sound.LoadSynchronous()))
	{
		return;
	}

	if (const auto* World{Mesh->GetWorld()}; World->WorldType == EWorldType::EditorPreview)
	{
		UGameplayStatics::PlaySoundAtLocation(World, SoundSettings.Sound.Get(), Mesh->GetComponentLocation(),
		                                      VolumeMultiplier, PitchMultiplier);
	}
	else
	{
		UGameplayStatics::SpawnSoundAtLocation(World, SoundSettings.Sound.Get(), FootstepLocation,
		                                       FootstepLocation.ToOrientationRotator(), VolumeMultiplier,
		                                       PitchMultiplier);
	}
}

void UAnimNotify_FoleyEvent::SpawnDecal(const USkeletalMeshComponent* Mesh,
                                        const FGASPFootstepDecalSettings& DecalSettings,
                                        const FVector& FootstepLocation, const FRotator& FootstepRotation,
                                        const FHitResult& FootstepHit) const
{
	if (!IsValid(DecalSettings.DecalMaterial.LoadSynchronous()))
	{
		return;
	}

	const auto DecalRotation{
		FootstepRotation.Quaternion() * FQuat{
			SocketName.ToString().EndsWith("_l")
				? DecalSettings.FootLeftRotationOffset.Quaternion()
				: DecalSettings.FootRightRotationOffset.Quaternion()
		}
	};

	const auto MeshScale{Mesh->GetComponentScale().Z};

	const auto DecalLocation{
		FootstepLocation + DecalRotation.RotateVector(FVector{DecalSettings.LocationOffset} * MeshScale)
	};

	auto* Decal = UGameplayStatics::SpawnDecalAttached(DecalSettings.DecalMaterial.Get(),
	                                                   FVector{DecalSettings.Size} * MeshScale,
	                                                   FootstepHit.Component.Get(), NAME_None, DecalLocation,
	                                                   DecalRotation.Rotator(),
	                                                   EAttachLocation::KeepWorldPosition);

	if (IsValid(Decal))
	{
		Decal->SetFadeOut(DecalSettings.Duration, DecalSettings.FadeOutDuration, false);
	}
}

void UAnimNotify_FoleyEvent::SpawnParticleSystem(const USkeletalMeshComponent* Mesh,
                                                 const FGASPFootstepParticleSettings& ParticleSystemSettings,
                                                 const FVector& FootstepLocation,
                                                 const FRotator& FootstepRotation) const
{
	if (!IsValid(ParticleSystemSettings.ParticleSystem.LoadSynchronous()))
	{
		return;
	}

	const auto MeshScale{Mesh->GetComponentScale().Z};

	const auto ParticleSystemLocation{
		FootstepLocation + FootstepRotation.RotateVector(FVector{ParticleSystemSettings.LocationOffset} * MeshScale)
	};

	UNiagaraFunctionLibrary::SpawnSystemAtLocation(Mesh->GetWorld(), ParticleSystemSettings.ParticleSystem.Get(),
	                                               ParticleSystemLocation, FootstepRotation,
	                                               FVector::OneVector * MeshScale, true, true,
	                                               ENCPoolMethod::AutoRelease);
}

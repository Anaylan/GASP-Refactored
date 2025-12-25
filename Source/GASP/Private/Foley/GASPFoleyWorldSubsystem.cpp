#include "Foley/GASPFoleyWorldSubsystem.h"

#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/DecalComponent.h"
#include "Foley/GASPFootstepEffectsSet.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPFoleyWorldSubsystem)

void UGASPFoleyWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	World = GetWorld();
}

FGASPFoleyOut UGASPFoleyWorldSubsystem::PlayFoleyEvent(UGASPFootstepEffectsSet* AudioBank,
                                                       const USkinnedMeshComponent* Mesh,
                                                       const FName SocketName, const bool bSpawnSound,
                                                       const bool bSpawnDecal,
                                                       const bool bSpawnParticleSystem, const bool bSpawnInAir,
                                                       const float TraceLength, const float VolumeMultiplier,
                                                       const float PitchMultiplier)
{
	const auto Owner = Mesh->GetOwner();
	if (!IsValid(Owner) || !World)
	{
		return FGASPFoleyOut{};
	}
	const auto SocketTransform{Mesh->GetSocketTransform(SocketName)};

	FCollisionQueryParams QueryParams{__FUNCTION__, true, Owner};
	QueryParams.bReturnPhysicalMaterial = true;

	TArray<AActor*> IgnoredActors;
	IgnoredActors.Add(Owner);
	FHitResult Hit;
	World->LineTraceSingleByChannel(Hit, SocketTransform.GetLocation(),
	                                SocketTransform.GetLocation() - FVector::ZAxisVector * TraceLength,
	                                ECC_Visibility, QueryParams);

	if (!Hit.bBlockingHit && !bSpawnInAir)
	{
		return FGASPFoleyOut{};
	}

	const auto SurfaceType{Hit.PhysMaterial.IsValid() ? Hit.PhysMaterial->SurfaceType.GetValue() : SurfaceType_Default};
	const auto FootstepSettings{AudioBank->GetFootstepSettingsFromSurface(SurfaceType)};
	if (!FAnimWeight::IsRelevant(VolumeMultiplier) || !FootstepSettings)
	{
		return FGASPFoleyOut{};
	}

	const auto SoundLocation{Hit.bBlockingHit ? Hit.ImpactPoint : SocketTransform.GetLocation()};
	const auto FootstepRotation{
		FRotationMatrix::MakeFromZY(Hit.ImpactNormal,
		                            SocketTransform.TransformVectorNoScale(FVector::UpVector)).Rotator()
	};

	FGASPFoleyOut FoleyOut{};
	if (bSpawnSound)
	{
		FoleyOut.AudioComponent = SpawnSound(Mesh, FootstepSettings->SoundSettings, SoundLocation, VolumeMultiplier,
		                                     PitchMultiplier);
	}
	if (bSpawnDecal)
	{
		FoleyOut.DecalComponent = SpawnDecal(Mesh, FootstepSettings->DecalSettings, SoundLocation, FootstepRotation,
		                                     Hit,
		                                     SocketName);
	}
	if (bSpawnParticleSystem)
	{
		FoleyOut.NiagaraComponent = SpawnParticleSystem(Mesh, FootstepSettings->ParticleSettings, SoundLocation,
		                                                FootstepRotation);
	}

	return FoleyOut;
}

UAudioComponent* UGASPFoleyWorldSubsystem::SpawnSound(const USkinnedMeshComponent* Mesh,
                                                      const FGASPFootstepSoundSettings& SoundSettings,
                                                      const FVector& FootstepLocation, const float VolumeMultiplier,
                                                      const float PitchMultiplier) const
{
	if (!IsValid(SoundSettings.Sound.LoadSynchronous()) || !World)
	{
		return nullptr;
	}

	if (World->WorldType == EWorldType::EditorPreview)
	{
		UGameplayStatics::PlaySoundAtLocation(World, SoundSettings.Sound.Get(), Mesh->GetComponentLocation(),
		                                      VolumeMultiplier, PitchMultiplier, 0.f, SoundSettings.SoundAttenuation,
		                                      SoundSettings.SoundConcurrency);
	}
	else
	{
		return UGameplayStatics::SpawnSoundAtLocation(World, SoundSettings.Sound.Get(), FootstepLocation,
		                                              FootstepLocation.ToOrientationRotator(), VolumeMultiplier,
		                                              PitchMultiplier, 0.f, SoundSettings.SoundAttenuation,
		                                              SoundSettings.SoundConcurrency);
	}
	return nullptr;
}

UDecalComponent* UGASPFoleyWorldSubsystem::SpawnDecal(const USkinnedMeshComponent* Mesh,
                                                      const FGASPFootstepDecalSettings& DecalSettings,
                                                      const FVector& FootstepLocation, const FRotator& FootstepRotation,
                                                      const FHitResult& FootstepHit, const FName& SocketName) const
{
	if (!IsValid(DecalSettings.DecalMaterial.LoadSynchronous()))
	{
		return nullptr;
	}

	const auto DecalRotation{
		FootstepRotation.Quaternion() * FQuat{
			SocketName.ToString().EndsWith("_l", ESearchCase::IgnoreCase)
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
		return Decal;
	}
	return nullptr;
}

UNiagaraComponent* UGASPFoleyWorldSubsystem::SpawnParticleSystem(const USkinnedMeshComponent* Mesh,
                                                                 const FGASPFootstepParticleSettings&
                                                                 ParticleSystemSettings,
                                                                 const FVector& FootstepLocation,
                                                                 const FRotator& FootstepRotation) const
{
	if (!IsValid(ParticleSystemSettings.ParticleSystem.LoadSynchronous()) || !World)
	{
		return nullptr;
	}

	const auto MeshScale{Mesh->GetComponentScale().Z};

	const auto ParticleSystemLocation{
		FootstepLocation + FootstepRotation.RotateVector(FVector{ParticleSystemSettings.LocationOffset} * MeshScale)
	};

	return UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, ParticleSystemSettings.ParticleSystem.Get(),
	                                                      ParticleSystemLocation, FootstepRotation,
	                                                      FVector::OneVector * MeshScale, true, true,
	                                                      ENCPoolMethod::AutoRelease);
}

#if WITH_EDITOR && ALLOW_CONSOLE
void UGASPFoleyWorldSubsystem::DebugLog(const FTransform& Transform, const FLinearColor& VisLogDebugColor,
                                        const FString& VisLogDebugText)
{
	if (!World)
	{
		return;
	}

	FVisualLogger::SphereLogf(World, FName(TEXT("VisLogFoley")), ELogVerbosity::Log,
	                          Transform.GetLocation(), 5.f, VisLogDebugColor.ToFColor(true), false,
	                          TEXT("%s"), *VisLogDebugText);
	DrawDebugSphere(World, Transform.GetLocation(), 10.f, 12, VisLogDebugColor.ToRGBE(),
	                false, 4.f);
}
#endif

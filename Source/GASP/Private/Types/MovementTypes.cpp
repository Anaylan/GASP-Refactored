#include "Types/MovementTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MovementTypes)

bool FGASPMoverInputs::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	bOutSuccess = FCharacterDefaultInputs::NetSerialize(Ar, Map, bOutSuccess);

	RotationMode.NetSerialize(Ar, Map, bOutSuccess);
	Gait.NetSerialize(Ar, Map, bOutSuccess);
	Stance.NetSerialize(Ar, Map, bOutSuccess);

	Ar << ControlRotationRate;
	Ar << RotationOffset;
	Ar << MovementDirection;

	FloorLocation.NetSerialize(Ar, Map, bOutSuccess);
	FloorNormal.NetSerialize(Ar, Map, bOutSuccess);
	AimingRotation.SerializeCompressedShort(Ar);
	Ar << RagdollTransform;

	return bOutSuccess;
}

void FGASPMoverInputs::ToString(FAnsiStringBuilderBase& Out) const
{
	FCharacterDefaultInputs::ToString(Out);

	Out.Appendf("Gait: %s\n", TCHAR_TO_ANSI(*Gait.ToString()));
	Out.Appendf("RotationMode: %s\n", TCHAR_TO_ANSI(*RotationMode.ToString()));
	Out.Appendf("Stance: %s\n", TCHAR_TO_ANSI(*Stance.ToString()));
	Out.Appendf("ControlRotationRate: %.3f\n", ControlRotationRate);
	Out.Appendf("RotationOffset: %.3f\n", RotationOffset);
	Out.Appendf("MovementDirection: %i\n", static_cast<int32>(MovementDirection));
	Out.Appendf("FloorLocation: X=%.1f Y=%.1f Z=%.1f\n", FloorLocation.X, FloorLocation.Y, FloorLocation.Z);
	Out.Appendf("FloorNormal: X=%.2f Y=%.2f Z=%.2f\n", FloorNormal.X, FloorNormal.Y, FloorNormal.Z);
	Out.Appendf("AimingRotation: P=%.2f Y=%.2f R=%.2f\n", AimingRotation.Pitch, AimingRotation.Yaw,
	            AimingRotation.Roll);
	Out.Appendf("RagdollTransform: %s\n", TCHAR_TO_ANSI(*RagdollTransform.ToString()));
}

bool FGASPMoverInputs::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	const auto& TypedAuthority = static_cast<const FGASPMoverInputs&>(AuthorityState);

	if (Super::ShouldReconcile(AuthorityState))
	{
		return true;
	}

	// Discrete state has to match exactly.
	if (Gait != TypedAuthority.Gait || RotationMode != TypedAuthority.RotationMode ||
		Stance != TypedAuthority.Stance || MovementDirection != TypedAuthority.MovementDirection)
	{
		return true;
	}

	// Continuous state is compared with a tolerance: everything below travels over the wire
	// quantized (FVector_NetQuantize, SerializeCompressedShort), so the authority copy never
	// matches the locally predicted full-precision value bit for bit.
	return !FMath::IsNearlyEqual(ControlRotationRate, TypedAuthority.ControlRotationRate,
	                             GASPInputTolerance::ControlRotationRate) ||
		!FMath::IsNearlyEqual(RotationOffset, TypedAuthority.RotationOffset, GASPInputTolerance::RotationOffset) ||
		!FloorLocation.Equals(TypedAuthority.FloorLocation, GASPInputTolerance::FloorLocation) ||
		!FloorNormal.Equals(TypedAuthority.FloorNormal, GASPInputTolerance::FloorNormal) ||
		!AimingRotation.Equals(TypedAuthority.AimingRotation, GASPInputTolerance::AimingRotation) ||
		!RagdollTransform.Equals(TypedAuthority.RagdollTransform, 5.0f);
}

void FGASPMoverInputs::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	FCharacterDefaultInputs::Interpolate(From, To, Pct);

	const auto& TypedFrom = static_cast<const FGASPMoverInputs&>(From);
	const auto& TypedTo = static_cast<const FGASPMoverInputs&>(To);

	const auto* ClosestInputs = Pct < 0.5f ? &TypedFrom : &TypedTo;
	MovementDirection = ClosestInputs->MovementDirection;
	Gait = ClosestInputs->Gait;
	RotationMode = ClosestInputs->RotationMode;
	Stance = ClosestInputs->Stance;

	ControlRotationRate = FMath::Lerp(TypedFrom.ControlRotationRate, TypedTo.ControlRotationRate, Pct);
	RotationOffset = FMath::Lerp(TypedFrom.RotationOffset, TypedTo.RotationOffset, Pct);

	FloorLocation = FMath::Lerp(TypedFrom.FloorLocation, TypedTo.FloorLocation, Pct);

	// Lerping two unit vectors does not preserve unit length, and downstream slope checks
	// treat this as a normal. A zero result is the established "no floor" sentinel, so it is
	// passed through untouched rather than normalized into an arbitrary direction.
	const FVector LerpedNormal{FMath::Lerp(TypedFrom.FloorNormal, TypedTo.FloorNormal, Pct)};
	FloorNormal = LerpedNormal.IsNearlyZero() ? FVector::ZeroVector : LerpedNormal.GetSafeNormal();

	AimingRotation = FMath::Lerp(TypedFrom.AimingRotation, TypedTo.AimingRotation, Pct);
	RagdollTransform.Blend(TypedFrom.RagdollTransform, TypedTo.RagdollTransform, Pct);
}

bool FGASPMoverSyncState::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// Every step reports through the same bOutSuccess reference, so it is folded into a running
	// result after each call. FGameplayTag::NetSerialize clears it when a tag index cannot be
	// resolved on the receiving side, and that failure must reach the caller.
	bool bSuccess = Super::NetSerialize(Ar, Map, bOutSuccess) && bOutSuccess;

	RotationMode.NetSerialize(Ar, Map, bOutSuccess);
	bSuccess &= bOutSuccess;
	Gait.NetSerialize(Ar, Map, bOutSuccess);
	bSuccess &= bOutSuccess;
	Stance.NetSerialize(Ar, Map, bOutSuccess);
	bSuccess &= bOutSuccess;

	bOutSuccess = bSuccess;
	return bSuccess;
}

void FGASPMoverSyncState::ToString(FAnsiStringBuilderBase& Out) const
{
	Super::ToString(Out);
	Out.Appendf("RotationMode: %s\tStance: %s\tGait: %s\n",
	            TCHAR_TO_ANSI(*RotationMode.ToString()), TCHAR_TO_ANSI(*Stance.ToString()),
	            TCHAR_TO_ANSI(*Gait.ToString()));
}

bool FGASPMoverSyncState::ShouldReconcile(const FMoverDataStructBase& AuthorityState) const
{
	auto& TypedAuthority = static_cast<const FGASPMoverSyncState&>(AuthorityState);
	return *this != TypedAuthority;
}

void FGASPMoverSyncState::Interpolate(const FMoverDataStructBase& From, const FMoverDataStructBase& To, float Pct)
{
	auto& TypedFrom = static_cast<const FGASPMoverSyncState&>(From);
	auto& TypedTo = static_cast<const FGASPMoverSyncState&>(To);

	const auto* ClosestInputs = Pct < 0.5f ? &TypedFrom : &TypedTo;

	Gait = ClosestInputs->Gait;
	RotationMode = ClosestInputs->RotationMode;
	Stance = ClosestInputs->Stance;
}

void FGASPMoverSyncState::Merge(const FMoverDataStructBase& From)
{
	auto& TypedFrom = static_cast<const FGASPMoverSyncState&>(From);

	RotationMode = TypedFrom.RotationMode;
	Stance = TypedFrom.Stance;
	Gait = TypedFrom.Gait;
}

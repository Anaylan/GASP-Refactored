#pragma once

#include "GASPPhysicsControlLibrary.generated.h"

/**
 * This library is duplicate of UPhysicsControlBPLibrary. Once necessary methods become public, this class can be removed
 */
UCLASS()
class GASP_API UGASPPhysicsControlLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// Bulk variant of DisableCollisionBetweenBodies. Disables collision for every
	// (FirstBoneNames[i], SecondBoneNames[j]) combination in a single call. An empty bone array, or
	// if the bone name is blank/None, means "use the component's single body" (e.g. for a
	// static-mesh component) and is treated as a one-element array of NAME_None. Self-pairs (the
	// same body on both sides) are skipped. See DisableCollisionBetweenBodies for the constraints
	// that apply per pair.
	//
	// Returns true only when every pair was applied successfully. Per-pair validation failures log
	// a warning and are skipped. The remaining valid pairs are still applied, and the return value
	// is false to signal partial application.
	UFUNCTION(BlueprintCallable, Category = PhysicsControl, Meta = (ReturnDisplayName = "Success"))
	static bool DisableCollisionBetweenBodyArrays(
		UPrimitiveComponent* FirstComponent, const TArray<FName>& FirstBoneNames,
		UPrimitiveComponent* SecondComponent, const TArray<FName>& SecondBoneNames);

	// Bulk variant of EnableCollisionBetweenBodies. Re-enables collision for every
	// (FirstBoneNames[i], SecondBoneNames[j]) combination in a single call. An empty bone array, or
	// if the bone name is blank/None, means "use the component's single body" (e.g. for a
	// static-mesh component) and is treated as a one-element array of NAME_None. Self-pairs and
	// pairs that aren't currently disabled are no-ops. See EnableCollisionBetweenBodies for the
	// constraints that apply per pair.
	//
	// Returns true only when every pair was applied successfully. Per-pair validation failures log
	// a warning and are skipped. The remaining valid pairs are still applied, and the return value
	// is false to signal partial application.
	UFUNCTION(BlueprintCallable, Category = PhysicsControl, Meta = (ReturnDisplayName = "Success"))
	static bool EnableCollisionBetweenBodyArrays(
		UPrimitiveComponent* FirstComponent, const TArray<FName>& FirstBoneNames,
		UPrimitiveComponent* SecondComponent, const TArray<FName>& SecondBoneNames);
};

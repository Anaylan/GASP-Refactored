#pragma once

#include "PoseSearch/PoseSearchResult.h"

#include "GASPInteractionTypes.generated.h"

class AActor;
class UAnimMontage;
class UPoseSearchDatabase;
class UPoseSearchInteractionAsset;

/** Chooser input row. Kept as the table contract; only InteractionType changed from FName to a tag. */
USTRUCT(BlueprintType)
struct GASP_API FCharacterInteractionInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	FName InteractionType{NAME_None};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GASP")
	float Speed{0.f};
};

/** Chooser output row: the roles each side of the interaction is allowed to take. */
USTRUCT(BlueprintType)
struct GASP_API FCharacterInteractionOutput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	TArray<FName> InitiatorRoles{};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GASP")
	TArray<FName> TargetRoles{};
};

/**
 * One participant's share of a running interaction.
 *
 * Result travels whole, because Blueprint reads it back out of the character's task state through
 * IGASPInteractionInterface::GetInteractionResult and expects a complete FPoseSearchBlueprintResult.
 *
 * Actor is carried alongside it because it cannot be recovered from Result on a remote machine:
 * UPoseSearchLibrary::GetActor resolves the participant through Result.AnimContexts, which holds
 * anim instances. Those are created at runtime by USkeletalMeshComponent::InitAnim with generated
 * names, so FNetGUIDCache::SupportsObject rejects them and they arrive null. The receiving machine
 * rebuilds AnimContexts locally from Actor instead.
 */
USTRUCT(BlueprintType)
struct GASP_API FGASPInteractionRep
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "GASP")
	TObjectPtr<AActor> Actor{nullptr};

	UPROPERTY(BlueprintReadOnly, Category = "GASP")
	FPoseSearchBlueprintResult Result{};
};

#pragma once

#include "Components/ActorComponent.h"
#include "Interaction/GASPInteractionTypes.h"
#include "GASPCharacterInteractionComponent.generated.h"

class UMoverComponent;
class AGASPCharacter;
class UAnimInstance;
class UAnimMontage;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GASP_API UGASPCharacterInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UGASPCharacterInteractionComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);
	/**
	 * Asks for an interaction of this type against these candidates.
	 *
	 * The authority decides: chooser, multi-character pose search, then whatever it found, and
	 * OutFailureReason carries that decision. Called on a client it only forwards the request, so
	 * OutFailureReason stays empty there - the answer arrives later through InteractionReps.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract(FName InteractionType, const TArray<AActor*>& Candidates);

	/** Resolves the anim instance to use for interaction on the given actor. */
	static UAnimInstance* GetAnimContext(const AActor* Actor);

	/** Runs the interaction currently held in InteractionReps. Mirrors PerformTraversalAction. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void PerformInteraction();

	/**
	 * Applies an authoritative interaction and hands it to every client.
	 *
	 * There is no multicast: replicating InteractionReps carries the interaction by itself, and
	 * OnRep_InteractionReps starts it on each client exactly once.
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact_ServerImplementation(const TArray<FGASPInteractionRep>& Reps);

	/** Replication callback for InteractionReps. Mirrors OnRep_TraversalResult. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnRep_InteractionReps();

	/**
	 * Carries the request, not the answer: the authority runs the search itself.
	 *
	 * Sending the result instead was measured at 1334.9 B per interaction against 22.1 B for the
	 * request, and the client's uplink is the scarce direction.
	 */
	UFUNCTION(Reliable, Server, Category = "Interaction")
	void Server_Interact(FName InteractionType, const TArray<AActor*>& Candidates);

	/** Participants of the running interaction, with the pose search result Blueprint reads back. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction",
		ReplicatedUsing = OnRep_InteractionReps, Transient)
	TArray<FGASPInteractionRep> InteractionReps{};

private:
	/** Undoes everything TryInteract applied. Idempotent: the montage delegates can fire more than once. */
	void FinishInteraction();

	TWeakObjectPtr<AGASPCharacter> CharacterOwner{};

	/** Participants we linked ticks and collision with for the running interaction. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGASPCharacter>> Participants{};

	/**
	 * Chooser plus multi-character pose search, paired with the actor each result belongs to.
	 * Runs only on the authority, and screens the request before touching the chooser.
	 */
	bool RunInteractionSearch(FName InteractionType, const TArray<AActor*>& Candidates,
	                          TArray<FGASPInteractionRep>& OutReps);

	/**
	 * The role-indexed anim contexts of the given participants, resolved locally from their Actor
	 * references.
	 *
	 * Anim instances are not net addressable, so they are deliberately kept out of InteractionReps
	 * and never replicated. Every machine derives them here instead, and PerformInteraction puts
	 * them back into the result it stores for Blueprint.
	 */
	static TArray<TObjectPtr<const UObject>> MakeAnimContexts(TArrayView<const FGASPInteractionRep> Reps);
};

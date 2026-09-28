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

	/** Handles cleanup when an interaction montage ends or blends out. */
	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	/** Requests an interaction. Evaluates on authority or forwards request to server if called on client. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract(FName InteractionType, const TArray<AActor*>& Candidates);

	/** Resolves the interaction anim instance for the given actor via interface or skeletal mesh. */
	static UAnimInstance* GetAnimContext(const AActor* Actor);

	/** Executes the interaction for all participants using current InteractionReps. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void PerformInteraction();

	/** Sets replicated InteractionReps on the server and initiates local execution. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void Interact_ServerImplementation(const TArray<FGASPInteractionRep>& Reps);

	/** Replication callback for InteractionReps to trigger PerformInteraction on clients. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OnRep_InteractionReps();

	/** Server RPC to validate and perform interaction search on the authority. */
	UFUNCTION(Reliable, Server, Category = "Interaction")
	void Server_Interact(FName InteractionType, const TArray<AActor*>& Candidates);

	/** Replicated interaction data and pose search results for all participants. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction",
		ReplicatedUsing = OnRep_InteractionReps, Transient)
	TArray<FGASPInteractionRep> InteractionReps{};

private:
	/** Cleans up participant task states, mesh tick prerequisites, and collision ignores. */
	void FinishInteraction();

	TWeakObjectPtr<AGASPCharacter> CharacterOwner{};

	/** Active participants tracked for collision ignore and tick dependency cleanup. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AGASPCharacter>> Participants{};

	/** Evaluates interaction Chooser and executes multi-character pose search queries. */
	bool RunInteractionSearch(FName InteractionType, const TArray<AActor*>& Candidates,
	                          TArray<FGASPInteractionRep>& OutReps);

	/** Builds an array of participant anim instances indexed by role for pose search results. */
	static TArray<TObjectPtr<const UObject>> MakeAnimContexts(TArrayView<const FGASPInteractionRep> Reps);
};

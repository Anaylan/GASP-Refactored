#pragma once

#include "Actors/GASPCharacter.h"
#include "GASPCharacterExample.generated.h"

UCLASS()
class GASPEXTRAS_API AGASPCharacterExample : public AGASPCharacter
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess))
	TObjectPtr<class UGameplayCameraComponent> GameplayCamera{};

protected:
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_Controller() override;

public:
	// Sets default values for this character's properties
	AGASPCharacterExample(const FObjectInitializer& ObjectInitializer);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void SprintAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void WalkAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void CrouchAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void JumpAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void AimAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void RagdollAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	virtual void StrafeAction(bool bPressed);

	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	void MoveAction(const FVector2D& Value);
	UFUNCTION(BlueprintCallable, Category="Input|Actions")
	void LookAction(const FVector2D& Value);
};

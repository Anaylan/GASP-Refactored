#pragma once

#include "GameFramework/PlayerController.h"
#include "GASPPlayerControllerExample.generated.h"

UCLASS()
class GASPEXTRAS_API AGASPPlayerControllerExample : public APlayerController
{
	GENERATED_BODY()

public:
	AGASPPlayerControllerExample() = default;

	UFUNCTION(BlueprintPure)
	class AGASPCharacterExample* GetPossessedPlayer();
	UFUNCTION(BlueprintPure)
	TSoftObjectPtr<class UInputMappingContext> GetDefaultInputMapping();

protected:
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnRep_Pawn() override;
	virtual void OnRep_Owner() override;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, BlueprintGetter=GetPossessedPlayer)
	TObjectPtr<AGASPCharacterExample> PossessedPlayer{};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, BlueprintGetter=GetDefaultInputMapping)
	TSoftObjectPtr<UInputMappingContext> DefaultInputMapping{};

	void SetupInput() const;
};

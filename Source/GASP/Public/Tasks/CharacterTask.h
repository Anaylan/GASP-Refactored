#pragma once

#include "GameplayTask.h"
#include "CharacterTask.generated.h"

class AGASPCharacter;
class AController;

/**
 * Base class for all character action tasks in GASP.
 */
UCLASS(Abstract, Blueprintable, DefaultToInstanced, EditInlineNew)
class GASP_API UCharacterTask : public UGameplayTask
{
	GENERATED_BODY()

public:
	UCharacterTask(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** Debugging hook for GameplayDebugger and Visual Logger */
	virtual FString GetDebugString() const override;

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
	UPROPERTY(BlueprintReadOnly, Transient)
	TWeakObjectPtr<AGASPCharacter> Character;
};

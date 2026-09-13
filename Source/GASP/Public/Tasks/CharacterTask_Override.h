#pragma once

#include "CharacterTask.h"
#include "CharacterTask_Override.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCharacterOverrideSimpleSignature);

/**
 * Task that dynamically links and unlinks an animation layer class to the character mesh.
 */
UCLASS()
class GASP_API UCharacterTask_Override : public UCharacterTask
{
	GENERATED_BODY()

public:
	UCharacterTask_Override();

	UPROPERTY(BlueprintAssignable)
	FCharacterOverrideSimpleSignature OnApplied;

	UPROPERTY(BlueprintAssignable)
	FCharacterOverrideSimpleSignature OnRemoved;

	UFUNCTION(BlueprintCallable, Category="Tasks", meta = (AdvancedDisplay = "TaskOwner, Priority",
		DefaultToSelf = "TaskOwner", BlueprintInternalUseOnly = "TRUE" ))
	static UCharacterTask_Override* CreateOverrideTask(TScriptInterface<IGameplayTaskOwnerInterface> TaskOwner,
	                                                   const TSubclassOf<UAnimInstance> OverrideAnimClass,
	                                                   const uint8 Priority = 192);

protected:
	virtual void Activate() override;
	virtual void OnDestroy(bool bInOwnerFinished) override;

private:
	UPROPERTY(Transient)
	TSubclassOf<UAnimInstance> OverrideAnimClass;

	bool bLayerLinked{false};
};

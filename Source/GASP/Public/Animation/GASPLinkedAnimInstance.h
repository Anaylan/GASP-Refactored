#pragma once

#include "Animation/AnimInstance.h"
#include "Types/StructTypes.h"
#include "GASPLinkedAnimInstance.generated.h"

UCLASS()
class GASP_API UGASPLinkedAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

protected:
	UPROPERTY(Category = "State", BlueprintGetter = GetParent, Transient, meta = (NeverAsPin))
	TObjectPtr<class UGASPAnimInstance> Parent;

	UPROPERTY(Category = "State", BlueprintGetter = GetCharacter, Transient,
		meta = (NeverAsPin))
	TObjectPtr<class AGASPCharacter> Character;

public:
	UGASPLinkedAnimInstance();

	virtual void NativeInitializeAnimation() override;

	UFUNCTION(BlueprintPure, meta = (HideSelfPin, BlueprintThreadSafe, ReturnDisplayName = "Parent"))
	UGASPAnimInstance* GetParent() const;

	UFUNCTION(BlueprintPure, meta = (HideSelfPin, BlueprintThreadSafe, ReturnDisplayName = "Character"))
	AGASPCharacter* GetCharacter() const;

	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FGameplayTag GetGait() const;

	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FGameplayTag GetMovementState() const;

	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FGameplayTag GetMovementMode() const;

	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FGameplayTag GetStanceMode() const;

	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FGameplayTag GetRotationMode() const;

	UFUNCTION(BlueprintGetter, meta = (BlueprintThreadSafe))
	FCharacterInfo GetCharacterInfo() const;

	UFUNCTION(BlueprintPure, meta = (BlueprintThreadSafe))
	float GetAimSweep() const;
};

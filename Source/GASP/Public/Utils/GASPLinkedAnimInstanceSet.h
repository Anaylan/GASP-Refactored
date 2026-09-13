#pragma once

#include "Engine/DataAsset.h"
#include "GASPLinkedAnimInstanceSet.generated.h"

UCLASS()
class GASP_API UGASPLinkedAnimInstanceSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName Name{};
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<UAnimInstance> LinkedAnimInstance;

public:
	UFUNCTION(BlueprintGetter)
	FORCEINLINE TSubclassOf<UAnimInstance> GetAnimInstance() const
	{
		return LinkedAnimInstance;
	}
};

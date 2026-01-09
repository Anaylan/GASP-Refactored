#pragma once

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Types/EnumTypes.h"
#include "GASPBlueprintLibrary.generated.h"

struct FGASPMoverInputs;
/**
 * 
 */
UCLASS()
class GASP_API UGASPBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UGASPBlueprintLibrary() = default;

	UFUNCTION(BlueprintPure, Category = "GASP|Utility",
		meta = (AutoCreateRefTerm = "Tag", ReturnDisplayName = "Tag Name"))
	static FName GetShortTagName(const FGameplayTag& GameplayTag);

	UFUNCTION(BlueprintPure, Category = "GASP|Utility",
		meta = (AutoCreateRefTerm = "GameplayTag", ReturnDisplayName = "All Child Tags"))
	static FGameplayTagContainer GetAllChildTags(const FGameplayTag& GameplayTag);
};

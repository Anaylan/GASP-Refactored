#pragma once

#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "GASPGameplayCameraInterface.generated.h"

class UCameraRigAsset;

namespace CameraStyleTags
{
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Close);
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Medium);
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Far);
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Debug);
}

namespace CameraModeTags
{
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(FreeCam);
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Strafe);
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Aim);
	GASPCAMERA_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TwinStick);
}

USTRUCT(BlueprintType)
struct FCameraProperties
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer GameplayTags{};
};

/**
 * 
 */
UINTERFACE()
class UGASPGameplayCameraInterface : public UInterface
{
	GENERATED_BODY()
};

class GASPCAMERA_API IGASPGameplayCameraInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Camera|Properties")
	FCameraProperties GetCameraProperties();
};

UCLASS(meta = (BlueprintThreadSafe))
class GASPCAMERA_API UGASPGameplayCameraBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Camera|Helpers")
	static FCameraProperties GetCameraProperties(AActor* CameraActor);

	UFUNCTION(BlueprintCallable, Category="Camera|Helpers")
	static UCameraRigAsset* GetCameraRigAsset(FCameraProperties CameraProperties, class UChooserTable* ChooserTable);
};

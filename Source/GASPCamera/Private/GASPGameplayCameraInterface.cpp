#include "GASPGameplayCameraInterface.h"
#include "ChooserFunctionLibrary.h"
#include "Core/CameraRigAsset.h"
#include "IObjectChooser.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPGameplayCameraInterface)

namespace CameraStyleTags
{
	UE_DEFINE_GAMEPLAY_TAG(Close, FName{TEXTVIEW("GASP.Camera.Style.Close")});
	UE_DEFINE_GAMEPLAY_TAG(Medium, FName{TEXTVIEW("GASP.Camera.Style.Medium")});
	UE_DEFINE_GAMEPLAY_TAG(Far, FName{TEXTVIEW("GASP.Camera.Style.Far")});
	UE_DEFINE_GAMEPLAY_TAG(Debug, FName{TEXTVIEW("GASP.Camera.Style.Debug")});
}

namespace CameraModeTags
{	
	UE_DEFINE_GAMEPLAY_TAG(FreeCam, FName{TEXTVIEW("GASP.Camera.Mode.FreeCam")});
	UE_DEFINE_GAMEPLAY_TAG(Strafe, FName{TEXTVIEW("GASP.Camera.Mode.Strafe")});
	UE_DEFINE_GAMEPLAY_TAG(Aim, FName{TEXTVIEW("GASP.Camera.Mode.Aim")});
	UE_DEFINE_GAMEPLAY_TAG(TwinStick, FName{TEXTVIEW("GASP.Camera.Mode.TwinStick")});
}


FCameraProperties UGASPGameplayCameraBlueprintFunctionLibrary::GetCameraProperties(AActor* CameraActor)
{
	if (IsValid(CameraActor) && CameraActor->Implements<UGASPGameplayCameraInterface>())
	{
		return IGASPGameplayCameraInterface::Execute_GetCameraProperties(CameraActor);
	}
	return FCameraProperties();
}

UCameraRigAsset* UGASPGameplayCameraBlueprintFunctionLibrary::GetCameraRigAsset(
	FCameraProperties CameraProperties, UChooserTable* ChooserTable)
{
	if (IsValid(ChooserTable))
	{
		auto Context = UChooserFunctionLibrary::MakeChooserEvaluationContext();
		Context.AddStructParam(CameraProperties);

		auto* Result{
			UChooserFunctionLibrary::EvaluateObjectChooserBase(
				Context, UChooserFunctionLibrary::MakeEvaluateChooser(ChooserTable), UCameraRigAsset::StaticClass())
		};

		return static_cast<UCameraRigAsset*>(Result);
	}
	return nullptr;
}

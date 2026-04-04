#pragma once

#include "Animation/AnimInstanceProxy.h"
#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Types/StructTypes.h"
#include "GASPAnimInstanceProxy.generated.h"

class AGASPCharacter;
class UGASPMoverComponent;

/**
 * 
 */
USTRUCT()
struct FGASPAnimInstanceProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FGASPAnimInstanceProxy() = default;

	explicit FGASPAnimInstanceProxy(UAnimInstance* InAnimInstance);

protected:
	virtual void InitializeObjects(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void Update(float DeltaSeconds) override;
	virtual void PostUpdate(UAnimInstance* InAnimInstance) const override;

public:
	TWeakObjectPtr<AGASPCharacter> Character{};
	TWeakObjectPtr<UGASPMoverComponent> MoverComponent{};
	
	FPoseSearchTrajectory_WorldCollisionResults TrajectoryCollision{};
	FTrajectoryInfo TrajectoryInfo;
};

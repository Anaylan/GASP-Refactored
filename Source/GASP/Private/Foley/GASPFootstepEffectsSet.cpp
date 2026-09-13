#include "Foley/GASPFootstepEffectsSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(GASPFootstepEffectsSet)

FGASPFootstepEffectsSettings* UGASPFootstepEffectsSet::GetFootstepSettingsFromSurface(const EPhysicalSurface Surface)
{
	return FootstepSettings.Find(Surface);
}


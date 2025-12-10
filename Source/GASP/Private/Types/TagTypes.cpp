#include "Types/TagTypes.h"

namespace MovementModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Grounded, FName{TEXTVIEW("GASP.Movement.Mode.OnGrounded")});
	UE_DEFINE_GAMEPLAY_TAG(InAir, FName{TEXTVIEW("GASP.Movement.Mode.InAir")});
	UE_DEFINE_GAMEPLAY_TAG(Slide, FName{TEXTVIEW("GASP.Movement.Mode.Slide")});
	UE_DEFINE_GAMEPLAY_TAG(Traverse, FName{TEXTVIEW("GASP.Movement.Mode.Traverse")});
}

namespace PoseModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Default, FName{TEXTVIEW("GASP.Pose.Default")})
	UE_DEFINE_GAMEPLAY_TAG(Normal, FName{TEXTVIEW("GASP.Pose.Normal")})
	UE_DEFINE_GAMEPLAY_TAG(Masculine, FName{TEXTVIEW("GASP.Pose.Masculine")})
	UE_DEFINE_GAMEPLAY_TAG(Feminine, FName{TEXTVIEW("GASP.Pose.Feminine")})
}

namespace OverlayModeTags
{
	UE_DEFINE_GAMEPLAY_TAG(Default, FName{TEXTVIEW("GASP.Overlay.Default")})
	UE_DEFINE_GAMEPLAY_TAG(Injured, FName{TEXTVIEW("GASP.Overlay.Injured")})
	UE_DEFINE_GAMEPLAY_TAG(HandsTied, FName{TEXTVIEW("GASP.Overlay.HandsTied")})
	UE_DEFINE_GAMEPLAY_TAG(Rifle, FName{TEXTVIEW("GASP.Overlay.Rifle")})
	UE_DEFINE_GAMEPLAY_TAG(PistolOneHanded, FName{TEXTVIEW("GASP.Overlay.PistolOneHanded")})
	UE_DEFINE_GAMEPLAY_TAG(PistolTwoHanded, FName{TEXTVIEW("GASP.Overlay.PistolTwoHanded")})
	UE_DEFINE_GAMEPLAY_TAG(Bow, FName{TEXTVIEW("GASP.Overlay.Bow")})
	UE_DEFINE_GAMEPLAY_TAG(Torch, FName{TEXTVIEW("GASP.Overlay.Torch")})
	UE_DEFINE_GAMEPLAY_TAG(Binoculars, FName{TEXTVIEW("GASP.Overlay.Binoculars")})
	UE_DEFINE_GAMEPLAY_TAG(Box, FName{TEXTVIEW("GASP.Overlay.Box")})
	UE_DEFINE_GAMEPLAY_TAG(Barrel, FName{TEXTVIEW("GASP.Overlay.Barrel")})
}

namespace LocomotionActionTags
{
	UE_DEFINE_GAMEPLAY_TAG(Ragdoll, FName{TEXTVIEW("GASP.Locomotion.Action.Ragdoll")})
	UE_DEFINE_GAMEPLAY_TAG(Vault, FName{TEXTVIEW("GASP.Locomotion.Action.Vault")})
	UE_DEFINE_GAMEPLAY_TAG(Mantle, FName{TEXTVIEW("GASP.Locomotion.Action.Mantle")})
	UE_DEFINE_GAMEPLAY_TAG(Hurdle, FName{TEXTVIEW("GASP.Locomotion.Action.Hurdle")})
}

namespace FoleyTags
{
	UE_DEFINE_GAMEPLAY_TAG(Handplant, FName{TEXTVIEW("Foley.Event.Handplant")})
	UE_DEFINE_GAMEPLAY_TAG(Run, FName{TEXTVIEW("Foley.Event.Run")})
	UE_DEFINE_GAMEPLAY_TAG(RunBackwds, FName{TEXTVIEW("Foley.Event.RunBackwds")})
	UE_DEFINE_GAMEPLAY_TAG(RunStrafe, FName{TEXTVIEW("Foley.Event.RunStrafe")})
	UE_DEFINE_GAMEPLAY_TAG(Scuff, FName{TEXTVIEW("Foley.Event.Scuff")})
	UE_DEFINE_GAMEPLAY_TAG(ScuffPivot, FName{TEXTVIEW("Foley.Event.ScuffPivot")})
	UE_DEFINE_GAMEPLAY_TAG(Walk, FName{TEXTVIEW("Foley.Event.Walk")})
	UE_DEFINE_GAMEPLAY_TAG(WalkBackwds, FName{TEXTVIEW("Foley.Event.WalkBackwds")})
	UE_DEFINE_GAMEPLAY_TAG(Jump, FName{TEXTVIEW("Foley.Event.Jump")})
	UE_DEFINE_GAMEPLAY_TAG(Land, FName{TEXTVIEW("Foley.Event.Land")})
	UE_DEFINE_GAMEPLAY_TAG(ScuffWall, FName{TEXTVIEW("Foley.Event.ScuffWall")})
	UE_DEFINE_GAMEPLAY_TAG(Tumble, FName{TEXTVIEW("Foley.Event.Tumble")})
}

namespace GaitTags
{
	UE_DEFINE_GAMEPLAY_TAG(Walk, FName{TEXTVIEW("GASP.Movement.Gait.Walk")});
	UE_DEFINE_GAMEPLAY_TAG(Run, FName{TEXTVIEW("GASP.Movement.Gait.Run")});
	UE_DEFINE_GAMEPLAY_TAG(Sprint, FName{TEXTVIEW("GASP.Movement.Gait.Sprint")});
}

namespace RotationTags
{
	UE_DEFINE_GAMEPLAY_TAG(OrientToMovement, FName{TEXTVIEW("GASP.Movement.Rotation.OrientToMovement")});
	UE_DEFINE_GAMEPLAY_TAG(Strafe, FName{TEXTVIEW("GASP.Movement.Rotation.Strafe")});
	UE_DEFINE_GAMEPLAY_TAG(Aim, FName{TEXTVIEW("GASP.Movement.Rotation.Aim")});
}

namespace StanceTags
{
	UE_DEFINE_GAMEPLAY_TAG(Standing, FName{TEXTVIEW("GASP.Stance.Standing")})
	UE_DEFINE_GAMEPLAY_TAG(Crouching, FName{TEXTVIEW("GASP.Stance.Crouching")})
}

namespace MovementStateTags
{
	UE_DEFINE_GAMEPLAY_TAG(Moving, FName{TEXTVIEW("GASP.Movement.State.Moving")});
	UE_DEFINE_GAMEPLAY_TAG(Idle, FName{TEXTVIEW("GASP.Movement.State.Idle")});
}

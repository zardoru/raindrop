

/* Matrix size constants */

// 4:3
const uint16_t ScreenWidthDefault = 1024;
const uint16_t ScreenHeightDefault = 768;

enum KeyType
{
	// General stuff
	KT_Unknown,
	KT_Escape,
	KT_Select,
	KT_Enter,
	KT_BSPC,
	KT_SelectRight,
	KT_Up,
	KT_Down,
	KT_Left,
	KT_Right,
	KT_ReloadScreenScripts,
	KT_Debug,
	KT_ReloadCFG,

	KT_P1_SCRATCH_UP,
	KT_P1_SCRATCH_DOWN,
	KT_P2_SCRATCH_UP,
	KT_P2_SCRATCH_DOWN,

    // Anything above ChannelStart is a lane signal
	KT_ChannelStart
};
        
extern const char* KeytypeNames[];


/* Program itself consts */
#define RAINDROP_WINDOWTITLE "raindrop ver: "
#define RAINDROP_VERSION "0.600"
#ifdef NDEBUG
#define RAINDROP_BUILDTYPE " "
#else
#define RAINDROP_BUILDTYPE " (debug) "
#endif

#define RAINDROP_VERSIONTEXT RAINDROP_VERSION RAINDROP_BUILDTYPE __DATE__

#include "BindingsManager.h"
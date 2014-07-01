-- self explainatory
Fullscreen = 0

-- folder under Gamefiles to look for resources and scripts
Skin = "default"

-- Uncomment these and set as you like, otherwise it'll be automatically set to fit your desktop
-- WindowWidth = 428
-- WindowHeight = 240

-- Enable Widescreen
-- 16:9 is 1, 16:10 is 2, anything else is 4:3
Widescreen = 1

-- Mode offsets, adds this much time in seconds to all notes
OffsetDC = 0
Offset7K = 0

-- Finish all OpenGL commands before swapping
FlushVideo = 0

-- Enable Vertical Syncronization
VSync = 0

-- Set this to 1 to try to use the real-time timer instead of
-- trying to calculate from the data uploaded to the audio card.
-- If on, may improve scrolling smoothness and reduce accuracy depending on the Error Tolerance
-- If off, may cause choppier scrolling, but increase the accuracy in game. Best results when off come from low latency audio output.
InterpolateTime = 0

-- How many MS off can Audio non-interpolated playback be from real-time?
-- leave this as is if you have no idea what this is
-- Also, it's irrelevant when InterpolateTime is deactivated.
ErrorTolerance = 10

-- Enable usage of WASAPI, offering lower audio latency (but possibly distorting audio)
-- if you don't know what wasapi is you probably don't care anyway, but if it rings a bell, ASIO
-- also this is only relevant to windows
-- seriously if you think this does anything on any other system
-- you're under the placebo effect
UseWasapi = 1
WasapiDontUseExclusiveMode = 0

-- Get the audio latency for the default device and try to automatically sync charts
-- not recommended when WASAPI is enabled
AudioCompensation = 0

-- Use audio latency compensation only on non-keysounded charts.
UseAudioCompensationKeysounds = 1
UseAudioCompensationNonKeysounded = 0

-- Set to 1 to prefer high-latency audio output.
-- May prevent underruns.
DontUseLowLatency = 0


-- Set this to 1 to put the decoder in its own thread
-- if you don't know what this means just leave this as is
UseThreadedDecoder = 1

-- Show .ogg files in the dotcur mode song selection for editing (0 is disabled)
OggListing = 0

-- This are the only set of keys to modify. Don't add stuff like Keys8K, as these are bound to the skin
-- by saying KeyQ = BindingN where N is one of the numbers formated as KeyN in here.
-- basically if Key1Binding = 7 the first lane will use the Key7 entry of this structure
Keys7K = {
	Key1 = 'S',
	Key2 = 'D',
	Key3 = 'F',
	Key4 = ' ',
	Key5 = 'L',
	Key6 = '96',
	Key7 = '39',
	Key8 = 'A'
}

--[[
Fill this in like


SongDirectories =
{
   "my_great_song_collection",
   "../oh_wow_relative_paths",
   "C:/some_directory_with_songs_yes_thats_a_forward_slash",
   "~/gee_how_many_songs",
   "/usr/lib/why_would_there_be_any_songs_here",
   "./holy/shit/nested/directories/why_would_you_do_that",
   "D:/oh/lol/you/mean/all/slashes/have/to/be/forward/slashes/ok"
}

etc... remember to add the comma after each entry except for the last one
or else it'll crash and burn and I won't make myself responsible for that
]]

SongDirectories = {
	"./examplesongsdirectory/"
}

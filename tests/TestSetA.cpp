#include "pch.h"
#include "../src/GameGlobal.h"
#include "../src/Song.h"
#include "../src/SongDC.h"
#include "../src/Song7K.h"
#include "../src/SongLoader.h"
#include "../src/BackgroundAnimation.h"

#include "../src/Logging.h"
#include "../src/ext/catch.hpp"

/*
	So much of the testing was already done by debugging the main .exe
	further new code will comply for use in test-driven development (2016-05-23)
	but for now I'll use this to make debugging easier. -az
*/

TEST_CASE("osu storyboard compliance")
{
	Interruptible stub;
	auto song = LoadSong7KFromFilename("tests/files/esb.osu");
	
	REQUIRE(song != nullptr);
	REQUIRE(song->Difficulties.size() != 0);

	Log::Printf("Loading BGA...\n");
	auto bga = BackgroundAnimation::CreateBGAFromSong(0, *song, &stub, true);

	bga->SetAnimationTime(65.0f);
}
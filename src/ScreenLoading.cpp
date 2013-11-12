#include "Global.h"
#include "GraphObject2D.h"
#include "ScreenLoading.h"
#include "ImageLoader.h"
#include "GameWindow.h"

void LoadFunction(void* Screen)
{
	IScreen *S = (IScreen*)Screen;
	S->Init();
}

ScreenLoading::ScreenLoading(IScreen *Parent, IScreen *_Next)
{
	Next = _Next;
	LoadThread = NULL;
	Running = true;
	Acceleration = 0;
}

void ScreenLoading::Init()
{
	mLogoCore.SetImage(ImageLoader::LoadSkin("logo_core.png"));
	/* Force revalidation of image 
	(fix sometimes being a white square instead of pic, there's ought to be a better way though) */
	mLogoCore.GetImage()->IsValid = false;
	mLogoCore.Centered = true;
	mLogoCore.ColorInvert = true;
	mLogoCore.AffectedByLightning = true;
	mLogoCore.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	mLogoCore.SetSize(400);

	mLogoSides.SetImage(ImageLoader::LoadSkin("logo_sides.png"));
	mLogoSides.GetImage()->IsValid = false;
	mLogoSides.Centered = true;
	mLogoSides.ColorInvert = true;
	mLogoSides.AffectedByLightning = true;
	mLogoSides.SetPosition(ScreenWidth / 2, ScreenHeight / 2);
	mLogoSides.SetSize(400);

	LoadThread = new boost::thread(LoadFunction, Next);
	WindowFrame.SetLightMultiplier(0.8);
	WindowFrame.SetLightPosition(glm::vec3(0,-0.5,1));
}

bool ScreenLoading::Run(double TimeDelta)
{
	if (!LoadThread)
		return RunNested(TimeDelta);

	Acceleration += 180 * TimeDelta;		 
	if (Acceleration > 720)
		Acceleration = 720;

	mLogoSides.AddRotation(TimeDelta * Acceleration);
	mLogoSides.Render();
	mLogoCore.Render();

	if (LoadThread->timed_join(boost::posix_time::seconds(0)))
	{
		delete LoadThread;
		LoadThread = NULL;
		WindowFrame.SetLightMultiplier(1);
		WindowFrame.SetLightPosition(glm::vec3(0,0,1));
	}
	boost::this_thread::sleep(boost::posix_time::millisec(16));
	return true;
}

void ScreenLoading::HandleInput(int32 key, KeyEventType code, bool isMouseInput)
{
	if (!LoadThread)
		Next->HandleInput(key, code, isMouseInput);
}

void ScreenLoading::Cleanup()
{
}
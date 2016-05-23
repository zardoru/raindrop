#include "pch.h"
#include "GameGlobal.h"
#include "BackgroundAnimation.h"
#include "Song.h"
#include "Song7K.h"
#include "ImageList.h"
#include "osuBackgroundAnimation.h"

#include <boost/algorithm/string/case_conv.hpp>
#include "Image.h"
#include "Logging.h"

const float OSB_WIDTH = 640;
const float OSB_HEIGHT = 480;
CfgVar OSBDebug("OSB", "Debug");

// Must match PP_* ordering. Displacement to apply to a top-left origin quad with size 1 depending on pivot.
// X first, then Y.
struct pivot
{
	float x, y;
};
pivot OriginPivots[] = {
	{0.f   ,  0.f},
	{-0.5f ,  0.f},
	{-1.f  ,  0.f},
	{0.f   , -0.5f},
	{-0.5f , -0.5f},
	{-1.f  , -0.5f},
	{0.f   , -1.f},
	{-0.5f , -1.f},
	{-1.f  , -1.f},
};

std::function<float(float)> EasingFuncs[] = {
	ease_none,
	ease_out,
	ease_in,
	ease_out,
	ease_in,
	ease_inout,
	ease_3in,
	ease_3out,
	ease_3inout,
	ease_4in,
	ease_4out,
	ease_4inout,
	ease_5in,
	ease_5out,
	ease_5inout
};


namespace osb {
	Event::Event(EEventType typ) :
		mEvtType(typ), mEase(EASE_NONE)
	{
		Time = 0; EndTime = 0;
	}

	EEventType Event::GetEventType() const
	{
		return mEvtType;
	}

	float Event::GetTime() const
	{
		return Time;
	}

	float Event::GetEndTime() const
	{
		return EndTime;
	}

	float Event::GetDuration() const
	{
		return EndTime - Time;
	}

	int Event::GetEase() const
	{
		return static_cast<int>(mEase);
	}

	void Event::SetTime(float time)
	{
		Time = time;
	}

	void Event::SetEndTime(float EndTime)
	{
		this->EndTime = EndTime;
	}

	void Event::SetEase(int val)
	{
		mEase = static_cast<EEase>(Clamp(val, static_cast<int>(EEase::EASE_NONE), static_cast<int>(EEase::EASE_COUNT) - 1));
	}

	float SingleValEvent::LerpValue(float At) const
	{
		return Lerp(Value, EndValue, EasingFuncs[GetEase()](Clamp((At - Time) / GetDuration(), 0.f, 1.f)));
	}

	float SingleValEvent::GetValue() const
	{
		return Value;
	}

	void SingleValEvent::SetValue(float value)
	{
		Value = value;
	}

	float SingleValEvent::GetEndValue() const
	{
		return EndValue;
	}

	void SingleValEvent::SetEndValue(float EndValue)
	{
		this->EndValue = EndValue;
	}

	Vec2 TwoValEvent::GetValue() const
	{
		return Value;
	}

	Vec2 TwoValEvent::GetEndValue() const
	{
		return EndValue;
	}

	void TwoValEvent::SetValue(Vec2 val)
	{
		Value = val;
	}

	void TwoValEvent::SetEndValue(Vec2 val)
	{
		EndValue = val;
	}

	Vec2 TwoValEvent::LerpValue(float At) const
	{
		return Lerp(Value, EndValue, EasingFuncs[GetEase()](Clamp((At - Time) / GetDuration(), 0.f, 1.f)));
	}

	Vec3 ColorizeEvent::GetValue() const
	{
		return Value;
	}

	Vec3 ColorizeEvent::GetEndValue() const
	{
		return EndValue;
	}

	void ColorizeEvent::SetValue(Vec3 val)
	{
		Value = val;
	}

	void ColorizeEvent::SetEndValue(Vec3 val)
	{
		EndValue = val;
	}

	Vec3 ColorizeEvent::LerpValue(float At) const
	{
		float factor = 1.f / 255.f;
		return Lerp(Value, EndValue, EasingFuncs[GetEase()](Clamp((At - Time) / GetDuration(), 0.f, 1.f))) * factor;
	}

	BGASprite::BGASprite(std::string file, EOrigin origin, Vec2 start_pos, ELayer layer) : EventComponent(EVT_COUNT)
	{
		mFile = file;
		mOrigin = origin;
		mStartPos = start_pos;
		mLayer = layer;

		mSprite = nullptr;
		mParent = nullptr;
		mImageIndex = -1;
		mUninitialized = true;
	}

	void BGASprite::SetSprite(Sprite* sprite)
	{
		mSprite = sprite;
	}


	void BGASprite::InitializeSprite()
	{
		if (mUninitialized) // We haven't initialized from the parent's data yet? Alright.
		{
			// -> == then
			assert(mParent != nullptr);
			// Starts from the sprite, at the bottom. Read bottom to top to see how transformations are applied.

			// Steamlined, in order
			// mFlip
			// Flip: scale by negative, then translate (would change without assumptions to quad being top-left on origin!)

			// mPivot
			// pivot: translate quad with top-left origin to specified pivot

			// mTransform
			// scale item up by scale and vscale command
			// apply rotation
			// apply position

			// rotation -> scale + vecscale -> position.
			mTransform.ChainTransformation(&mParent->GetScreenTransformation());

			// flip -> pivot
			mPivot.ChainTransformation(&mTransform);
			
			// sprite -> flip vertices
			mFlip.ChainTransformation(&mPivot);

			// No op from sprite.
			mSprite->ChainTransformation(&mFlip);
			mPivot.SetPosition(OriginPivots[mOrigin].x, OriginPivots[mOrigin].y);

			// Set the image.
			mSprite->SetImage(mParent->GetImageFromIndex(mImageIndex), false);

			mUninitialized = false;
		}
	}

	template <class T>
	typename T::iterator GetEvent(double Time, T& vec)
	{
		return lower_bound(vec.begin(), vec.end(), Time, [](const typename T::value_type & v, const double &T)
		{
			return v.Time < T;
		});
	}

	template <class T>
	bool clamp_iter(typename T::iterator &it, T& vec)
	{
		if (vec.begin() == vec.end()) return false; // don't use this iter
		if (it != vec.begin()) it--;
		return true;
	}

	void BGASprite::Update(float Time)
	{
		assert (mSprite != nullptr);
		assert (mParent != nullptr); // We need this.
		// Now get the values for all the different stuff.
		
		// Okay, a pretty long function follows. Fade first.
		auto fade_evt = GetEvent(Time, evFade);
		
		if (clamp_iter(fade_evt, evFade)) {
			if (WithinEvents(Time))
			{
				if (fade_evt->GetTime() == 0 && mLayer == LAYER_SP_BACKGROUND)
					mSprite->Alpha = 1;
				else
					mSprite->Alpha = fade_evt->LerpValue(Time);
			}
			else
				mSprite->Alpha = 0;
		}
		else {
			if (WithinEvents(Time))
				mSprite->Alpha = 1;
			else
				mSprite->Alpha = 0;
		}

		// Don't bother updating unless we're visible.
		if (mSprite->Alpha == 0)
			return;

		// Now position.	
		auto movx_evt = GetEvent(Time, evMoveX);
		if (clamp_iter(movx_evt, evMoveX))
			mTransform.SetPositionX(movx_evt->LerpValue(Time));
		else mTransform.SetPositionX(mStartPos.x);

		auto movy_evt = GetEvent(Time, evMoveY);
		if (clamp_iter(movy_evt, evMoveY))
			mTransform.SetPositionY(movy_evt->LerpValue(Time));
		else mTransform.SetPositionY(mStartPos.y);

		// We already unpacked move events, so no need for this next snip.
		/* auto mov_evt = GetEvent(Time, EVT_MOVE);
		if (IsValidEvent(mov_evt, EVT_MOVE))
			mTransform.SetPosition(event_cast<MoveEvent>(mov_evt)->LerpValue(Time)); */

		// Now scale and rotation.
		float scale = 1;
		auto scale_evt = GetEvent(Time, evScale);
		if (clamp_iter(scale_evt, evScale))
			scale = scale_evt->LerpValue(Time);
		else scale = 1;

		// we want to scale it to fit - but we don't want to alter the scale set by the user
		// scales just get multiplied so we'll do that
		if (mLayer == osb::LAYER_SP_BACKGROUND && mSprite->GetImage())
			scale *= OSB_HEIGHT / mSprite->GetImage()->h;

		// Since scale is just applied to size straight up, we can use this extra scale
		// defaulting at 1,1 to be our vector scale. That way they'll pile up.
		Vec2 vscale;
		auto vscale_evt = GetEvent(Time, evScaleVec);
		if (clamp_iter(vscale_evt, evScaleVec))
			vscale = vscale_evt->LerpValue(Time);
		else vscale = Vec2(1, 1);

		auto rot_evt = GetEvent(Time, evRotate);
		float rot = 0;
		if (clamp_iter(rot_evt, evRotate)) {
			rot = rot_evt->LerpValue(Time);
			mTransform.SetRotation(glm::degrees(rot));
		}
		else mTransform.SetRotation(0);

		if (mSprite->GetImage())
		{
			auto i = mSprite->GetImage();

			// Move, then scale (is the way transformations are set up
			// therefore, pivot is applied, then scale
			// then both size and scale on mTransform are free for usage.

			// For some reason it's not applying scale then rotate - but rotate then scale.
			// To get around this, we undo the transformation on the object's x-axis
		    // by multiplying this value to X.
			mTransform.SetSize(i->w * scale * vscale.x, i->h * scale * vscale.y);
		}


		auto colorization_evt = GetEvent(Time, evColorize);
		if (clamp_iter(colorization_evt, evColorize)) {
			auto lerp = colorization_evt->LerpValue(Time);
			mSprite->Red = lerp.r;
			mSprite->Green = lerp.g;
			mSprite->Blue = lerp.b;
		}

		// The effects after this don't set values before they begin. (Parameter)
		auto additive_evt = GetEvent(Time, evAdditive);
		if (additive_evt != evAdditive.begin()
			&& evAdditive.begin() != evAdditive.end()
			&& (additive_evt - 1)->GetEndTime() <= Time)
			mSprite->SetBlendMode(BLEND_ADD);
		else mSprite->SetBlendMode(BLEND_ALPHA);

		auto hflip_evt = GetEvent(Time, evFlipH);
		if (hflip_evt != evFlipH.begin() 
			&& evFlipH.begin() != evFlipH.end())
		{
			if ((hflip_evt - 1)->GetEndTime() <= Time)
			{
				mFlip.SetScaleX(-1);
				mFlip.SetPositionX(1);
			} else
			{
				mFlip.SetScaleX(1);
				mFlip.SetPositionX(0);
			}
		}

		auto vflip_evt = GetEvent(Time, evFlipV);
		if (vflip_evt != evFlipV.begin()
			&& evFlipV.begin() != evFlipV.end())
		{
			if ((vflip_evt - 1)->GetEndTime() <= Time)
			{
				mFlip.SetScaleY(-1);
				mFlip.SetPositionY(1);
			} else
			{
				mFlip.SetScaleY(1);
				mFlip.SetPositionY(0);
			}
		}
	}

	std::string BGASprite::GetImageFilename() const
	{
		return mFile;
	}

	ELayer BGASprite::GetLayer() const
	{
		return mLayer;
	}

	void BGASprite::SetParent(osuBackgroundAnimation* parent)
	{
		mParent = parent;
	}

	void BGASprite::SetImageIndex(int index)
	{
		mImageIndex = index;
	}

	template <class T>
	void VecSort(T& vec)
	{
		auto cmp = [](const T::value_type& A, const T::value_type& B)
		{
			return A.GetTime() < B.GetTime();
		};

		stable_sort(vec.begin(), vec.end(), cmp);
		vec.shrink_to_fit();
	}

	void EventComponent::SortEvents()
	{
		VecSort(evMoveX);
		VecSort(evMoveY);
		VecSort(evScale);
		VecSort(evScaleVec);
		VecSort(evRotate);
		VecSort(evColorize);
		VecSort(evFade);
		VecSort(evFlipH);
		VecSort(evFlipV);
		VecSort(evAdditive);
	}

	bool EventComponent::WithinEvents(float Time) const
	{
		return Time >= GetStartTime() && Time <= GetEndTime();
	}

	void EventComponent::CopyEventsFrom(EventComponent &ec)
	{
		evMoveX = ec.evMoveX;
		evMoveY = ec.evMoveY;
		evScale = ec.evScale;
		evScaleVec = ec.evScaleVec;
		evRotate = ec.evRotate;
		evColorize = ec.evColorize;
		evFade = ec.evFade;
		evFlipH = ec.evFlipH;
		evFlipV = ec.evFlipV;
		evAdditive = ec.evAdditive;
	}

	EventComponent::EventComponent(EEventType evt): 
		Event(evt), 
		StartPeriod(std::numeric_limits<float>::infinity()), 
		EndPeriod(-StartPeriod)
	{}

	float EventComponent::GetStartTime() const
	{
		return StartPeriod;
	}

	float EventComponent::GetEndTime() const
	{
		return EndPeriod;
	}

	void EventComponent::AddEvent(std::shared_ptr<Event> event)
	{
		if (event->GetEventType() != EVT_MOVE) {
			switch (event->GetEventType()) {
			case EVT_MOVEX:
				evMoveX.push_back(*std::static_pointer_cast<MoveXEvent>(event));
				break;
			case EVT_MOVEY:
				evMoveY.push_back(*std::static_pointer_cast<MoveYEvent>(event));
				break;
			case EVT_SCALE:
				evScale.push_back(*std::static_pointer_cast<ScaleEvent>(event));
				break;
			case EVT_SCALEVEC:
				evScaleVec.push_back(*std::static_pointer_cast<VectorScaleEvent>(event));
				break;
			case EVT_ROTATE:
				evRotate.push_back(*std::static_pointer_cast<RotateEvent>(event));
				break;
			case EVT_COLORIZE:
				evColorize.push_back(*std::static_pointer_cast<ColorizeEvent>(event));
				break;
			case EVT_FADE:
				evFade.push_back(*std::static_pointer_cast<FadeEvent>(event));
				break;
			case EVT_ADDITIVE:
				evAdditive.push_back(*std::static_pointer_cast<AdditiveEvent>(event));
				break;
			case EVT_HFLIP:
				evFlipH.push_back(*std::static_pointer_cast<FlipHorizontalEvent>(event));
				break;
			case EVT_VFLIP:
				evFlipV.push_back(*std::static_pointer_cast<FlipVerticalEvent>(event));
				break;
			}
		}
		else // Unpack move events.
		{
			auto mov = std::static_pointer_cast<MoveEvent>(event);
			auto mxe = MoveXEvent();
			auto mye = MoveYEvent();

			mxe.SetEase(mov->GetEase());
			mye.SetEase(mov->GetEase());
			
			mxe.SetTime(mov->GetTime());
			mye.SetTime(mov->GetTime());
			mxe.SetEndTime(mov->GetEndTime());
			mye.SetEndTime(mov->GetEndTime());

			mxe.SetValue(mov->GetValue().x);
			mye.SetValue(mov->GetValue().y);
			mxe.SetEndValue(mov->GetEndValue().x);
			mye.SetEndValue(mov->GetEndValue().y);

			evMoveX.push_back(mxe);
			evMoveY.push_back(mye);
		}
		if (event->GetTime() < StartPeriod)
			StartPeriod = event->GetTime();
		if (event->GetEndTime() > EndPeriod)
			EndPeriod = event->GetEndTime();
	}

	void EventComponent::ClearEvents()
	{
		evMoveX.clear();
		evMoveY.clear();
		evScale.clear();
		evScaleVec.clear();
		evRotate.clear();
		evColorize.clear();
		evFade.clear();
		evFlipH.clear();
		evFlipV.clear();
		evAdditive.clear();
	}

	// super obtuse way: member functions.
	// somewhat obtuse way: this.

	template <class T>
	void minimize(T vec, float &min)
	{
		for (auto evt : vec)
			min = std::min(min, evt.GetTime());
	}


	template <class T>
	void maximize(T vec, float &max)
	{
		for (auto evt : vec)
			max = std::max(max, evt.GetEndTime());
	}

	float Loop::GetIterationDuration()
	{	
		auto min_end = std::numeric_limits<float>::infinity(); // a mouthful to type.
		minimize(evMoveX, min_end);
		minimize(evMoveY, min_end);
		minimize(evScale, min_end);
		minimize(evScaleVec, min_end);
		minimize(evRotate, min_end);
		minimize(evColorize, min_end);
		minimize(evFade, min_end);
		minimize(evFlipH, min_end);
		minimize(evFlipV, min_end);
		minimize(evAdditive, min_end);

		auto max_end = -std::numeric_limits<float>::infinity(); // a mouthful to type.
		maximize(evMoveX, max_end);
		maximize(evMoveY, max_end);
		maximize(evScale, max_end);
		maximize(evScaleVec, max_end);
		maximize(evRotate, max_end);
		maximize(evColorize, max_end);
		maximize(evFade, max_end);
		maximize(evFlipH, max_end);
		maximize(evFlipV, max_end);
		maximize(evAdditive, max_end);

		return max_end - min_end;
		/*float dur = 0;
				for (auto k = 0; k < EVT_COUNT; k++)
			for (auto evt : mEventList[k])
				dur = std::max(evt->GetEndTime(), dur);

		return dur;*/
	}

	
	template <class T>
	void UnrollEvents(double iter_duration, double Time, uint32_t LoopCount, T& dst, T& src)
	{
		for (auto &evt : src) {
			for (auto i = 0; i < LoopCount; i++) {
				auto el = evt;
				el.SetTime(evt.GetTime() + iter_duration * i + Time);
				el.SetEndTime(evt.GetEndTime() + iter_duration * i + Time);

				// Add into the unrolled events list
				dst.push_back(el);
			}
		}
	}

	void Loop::Unroll(EventComponent *ec)
	{
		if (!ec) throw std::runtime_error("No event component to unroll to.");

		double iter_duration = GetIterationDuration();


		// okay, osu loops are super funky.
		// a loop's iteration is calculated by dur = last event's end time - first event's start time
		// not just last event's end time, so events are repeated as soon as the last one of the previous one ends
		// that means, the time of the next event is not loop start time + max last time of event * iter
		// but that dur previously mentioned instead.
		UnrollEvents(iter_duration, Time, LoopCount, ec->evMoveX, evMoveX);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evMoveY, evMoveY);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evScale, evScale);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evScaleVec, evScaleVec);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evRotate, evRotate);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evColorize, evColorize);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evFade, evFade);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evFlipH, evFlipH);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evFlipV, evFlipV);
		UnrollEvents(iter_duration, Time, LoopCount, ec->evAdditive, evAdditive);
	}
}

osb::EOrigin OriginFromString(std::string str)
{
	boost::algorithm::to_lower(str);
	if (str == "topleft") return osb::PP_TOPLEFT;
	if (str == "topcenter" || str == "topcentre" || str == "top") return osb::PP_TOP;
	if (str == "topright") return osb::PP_TOPRIGHT;
	if (str == "left") return osb::PP_LEFT;
	if (str == "right") return osb::PP_RIGHT;
	if (str == "center" || str == "centre") return osb::PP_CENTER;
	if (str == "bottomleft") return osb::PP_BOTTOMLEFT;
	if (str == "bottom" || str == "bottomcenter" || str == "bottomcentre") return osb::PP_BOTTOM;
	if (str == "bottomright") return osb::PP_BOTTOMRIGHT;

	if (OSBDebug)
		Log::LogPrintf("OSB: Unknown origin type: %s.\n", str.c_str());
	return osb::PP_TOPLEFT;
}

osb::ELayer LayerFromString(std::string str)
{
	if (str == "background") return osb::LAYER_BACKGROUND;
	if (str == "pass") return osb::LAYER_PASS;
	if (str == "fail") return osb::LAYER_FAIL;
	if (str == "foreground") return osb::LAYER_FOREGROUND;

	if (OSBDebug)
		Log::LogPrintf("OSB: Unknown layer type: %s.\n", str.c_str());
	return osb::LAYER_BACKGROUND;
}

std::shared_ptr<osb::Event> ParseEvent(std::vector<std::string> split)
{
	auto ks = split[0];
	std::shared_ptr<osb::Event> evt;
	boost::algorithm::to_upper(ks);

	if (ks == "V"){
		auto xvt = std::make_shared<osb::VectorScaleEvent>();
		xvt->SetValue(Vec2(latof(split[4]), latof(split[5])));
		if (split.size() > 6)
			xvt->SetEndValue(Vec2(latof(split[6]), latof(split[7])));
		else 
			xvt->SetEndValue(Vec2(latof(split[4]), latof(split[5])));
		evt = xvt;
	}
	if (ks == "S") {
		auto xvt = std::make_shared<osb::ScaleEvent>();
		xvt->SetValue(latof(split[4]));
		if (split.size() > 5)
			xvt->SetEndValue(latof(split[5]));
		else
			xvt->SetEndValue(latof(split[4]));
		evt = xvt;
	}
	if (ks == "M") {
		auto xvt = std::make_shared<osb::MoveEvent>();
		xvt->SetValue(Vec2(latof(split[4]), latof(split[5])));
		if (split.size() > 6)
			xvt->SetEndValue(Vec2(latof(split[6]), latof(split[7])));
		else
			xvt->SetEndValue(Vec2(latof(split[4]), latof(split[5])));
		evt = xvt;
	}
	if (ks == "MX") {
		auto xvt = std::make_shared<osb::MoveXEvent>();
		xvt->SetValue(latof(split[4]));
		if (split.size() > 5)
			xvt->SetEndValue(latof(split[5]));
		else
			xvt->SetEndValue(latof(split[4]));
		evt = xvt;
	}
	if (ks == "MY") {
		auto xvt = std::make_shared<osb::MoveYEvent>();
		xvt->SetValue(latof(split[4]));
		if (split.size() > 5)
			xvt->SetEndValue(latof(split[5]));
		else
			xvt->SetEndValue(latof(split[4]));
		evt = xvt;
	}
	if (ks == "C") {
		auto xvt = std::make_shared<osb::ColorizeEvent>();
		xvt->SetValue(Vec3(latof(split[4]), latof(split[5]), latof(split[6])));
		if (split.size() > 7)
			xvt->SetEndValue(Vec3(latof(split[7]), latof(split[8]), latof(split[9])));
		else
			xvt->SetEndValue(Vec3(latof(split[4]), latof(split[5]), latof(split[6])));
		evt = xvt;
	}
	if (ks == "P") {
		if (split[4] == "a") {
			auto xvt = std::make_shared<osb::AdditiveEvent>();
			evt = xvt;
		} else if (split[4] == "h")
		{
			evt = std::make_shared<osb::HFlipEvent>();
		}
		else if (split[4] == "v")
			evt = std::make_shared<osb::VFlipEvent>();
	}
	if (ks == "F") {
		auto xvt = std::make_shared<osb::FadeEvent>();
		xvt->SetValue(latof(split[4]));
		if (split.size() > 5)
			xvt->SetEndValue(latof(split[5]));
		else
			xvt->SetEndValue(latof(split[4]));
		evt = xvt;
	}
	if (ks == "L") {
		auto xvt = std::make_shared<osb::Loop>(latof(split[2]));
		evt = xvt;
	}
	if (ks == "R")
	{
		auto xvt = std::make_shared<osb::RotateEvent>();
		xvt->SetValue((latof(split[4])));
		if (split.size() > 5)
			xvt->SetEndValue((latof(split[5])));
		else
			xvt->SetEndValue((latof(split[4])));
		evt = xvt;
	}
	if (ks == "T")
	{
		throw std::runtime_error("OSB: Storyboards using triggers is not supported.");
	}

	if (evt) {
		if (evt->GetEventType() != osb::EVT_LOOP) {
			evt->SetEase(latof(split[1]));
			evt->SetTime(latof(split[2].length() ? split[2] : split[3]) / 1000.0);
			evt->SetEndTime(latof(split[3].length() ? split[3] : split[2]) / 1000.0);
		}
		else
		{
			evt->SetTime(latof(split[1]) / 1000.0);
		}
	}
	return evt;
}

osb::SpriteList ReadOSBEvents(std::istream& event_str)
{
	auto line_nm = 1;
	auto list = osb::SpriteList();
	int previous_lead = 100;
	int loop_lead = -1;
	osb::BGASprite* previous_background = nullptr;
	osb::BGASprite* sprite = nullptr;
	std::shared_ptr<osb::Loop> loop = nullptr;

	std::string line;
	/*
		Though there's an implicit tree structure given by whitespace or underscores
		a la python, it isn't infinitely nested. As such, it's pretty easy to just
		do what is being done here.
	*/

	std::regex fnreg("\"?(.*?)\"?$");
	auto strip_quotes = [&](std::string s)
	{
		std::string striped_filename;
		std::smatch sm;
		if (std::regex_match(s, sm, fnreg))
		{
			striped_filename = sm[1];
		}
		else { striped_filename = s; }
		return striped_filename;
	};

	auto unroll_loop_into_sprite = [&]()
	{
		osb::EventVector vec;
		loop->Unroll(sprite);
		loop = nullptr;
	};

	while (std::getline(event_str, line))
	{
		int lead_spaces;
		line = line.substr(0, line.find("//")); // strip comments

		auto sst = line.find_first_not_of("\t _");
		lead_spaces = sst != std::string::npos ? sst : 0;
		line = line.substr(lead_spaces);

		// TODO: a proper csv parse for this.
		std::vector<std::string> split_result = Utility::TokenSplit(line, ",");
		for (auto &&s : split_result) boost::algorithm::to_lower(s);

		if (!line.length() || split_result.size() < 2) continue;

		

		auto parse_sprite = [&]()
		{
			if (split_result[0] == "sprite")
			{
				Vec2 new_position(latof(split_result[4]), latof(split_result[5]));
				std::string striped_filename = strip_quotes(split_result[3]);

				if (sprite)
					sprite->SortEvents(); // we're done. clean up and get ready

				list.push_back(osb::BGASprite(striped_filename, 
											  OriginFromString(split_result[2]), 
											  new_position, 
											  LayerFromString(split_result[1])));

				sprite = &list.back();
				return true;
			}

			return false;
		};

		auto add_special_event = [&]()
		{
			if (split_result[0] == "0")
			{
				float time = latof(split_result[1]);
				auto bgsprite = osb::BGASprite(strip_quotes(split_result[2]), 
					osb::PP_CENTER, 
					Vec2(320, 240),
					osb::LAYER_SP_BACKGROUND);

				auto evt = std::make_shared<osb::FadeEvent>();
				evt->SetEndValue(1);
				evt->SetValue(1);
				evt->SetTime(0);
				evt->SetEndTime(std::numeric_limits<float>::infinity());

				bgsprite.AddEvent(evt);

				/*if (previous_background) {
					float T = 1;
					auto prev_evt = previous_background->GetEvent(T, osb::EVT_FADE);
					if (previous_background->IsValidEvent(prev_evt, osb::EVT_FADE))
						previous_background->GetEventList(osb::EVT_FADE)))->SetEndTime(time);
				}*/

				previous_background = &bgsprite;

				list.insert(list.begin(), bgsprite);
				
				return true;
			}

			if (split_result[0] == "2")
				return true;

			return false;
		};

		
		
		if (lead_spaces < previous_lead && !loop)
		{
			if (!parse_sprite())
				add_special_event();
		} else {

			// If it's a loop, check if we're out of it.
			// If we're out of it, read a regular event, otherwise, read an event to the loop
			if (loop)
			{
				if (lead_spaces < previous_lead || lead_spaces == loop_lead) {
					// We're done reading the loop - unroll it.
					unroll_loop_into_sprite();
				}
				else
					loop->AddEvent(ParseEvent(split_result));
			}
			
			// It's not a command on the loop, or we weren't reading a loop in the first place.
			// Read a regular command.

			// Not "else" because we do want to execute this if we're no longer reading the loop.
			if (!loop) {
				if (!parse_sprite()) { // Not a sprite, okay...
					if (!add_special_event()) { // Not a special event, like a background or whatever.

						auto ev = ParseEvent(split_result);

						if (!ev) {
							if (OSBDebug)
								Log::LogPrintf("OSB: Event line %i does not name a known event type.\n", line_nm);
							continue;
						}

						if (!sprite)
							throw std::runtime_error("OSB command unpaired with sprite.");

						// A loop began - set that we are reading a loop and set this loop as where to add the following commands.
						if (ev->GetEventType() == osb::EVT_LOOP)
						{
							loop = std::static_pointer_cast<osb::Loop>(ev);
							loop_lead = lead_spaces;
						}
						else // add this event, if not a loop to this mSprite. It'll be unrolled once outside.
						{
							sprite->AddEvent(ev);
						}
					}
				}
			}
		}

		previous_lead = lead_spaces;
	}

	if (sprite)
		sprite->SortEvents();
	// we have a pending loop? oops sorry
	if (sprite && loop)
		unroll_loop_into_sprite();

	return list;
}

int osuBackgroundAnimation::AddImageToList(std::string image_filename)
{
	std::filesystem::path fn = image_filename;
	auto idx = mFileIndices.size() + 1;
	auto full_path = Song->SongDirectory / image_filename;

	if (std::filesystem::exists(full_path)) {
		// Okay, full path is definitely not in.
		if (mFileIndices.find(full_path.string()) == mFileIndices.end()) {
			mFileIndices[full_path.string()] = idx;
			return idx;
		} // It is? Okay. Give back the full path's index here.
		else
			return mFileIndices[full_path.string()];
	} else
	{
		// Couldn't find this file? Perhaps it's missing its extension.
		auto dir = full_path.parent_path();
		for (auto i: std::filesystem::directory_iterator(dir))
		{
			// Okay, filenames are the same...
			if (i.path().filename() == full_path.filename())
			{
				fn.replace_extension(i.path().extension());
				if (mFileIndices.find(fn.string()) == mFileIndices.end())
					mFileIndices[fn.string()] = idx;
				else
					return mFileIndices[fn.string()];
			}
		}
	}

	return -1; // everything has failed
}

osuBackgroundAnimation::osuBackgroundAnimation(Interruptible* parent, osb::SpriteList* existing_mSprites, VSRG::Song* song)
	: BackgroundAnimation(parent),
		mImageList(this)
{
	Transform.SetSize(OSB_WIDTH, OSB_HEIGHT);
	mScreenTransformation.SetSize(1 / OSB_WIDTH, 1 / OSB_HEIGHT);
	mScreenTransformation.ChainTransformation(&Transform);
	Song = song;
	CanValidate = false;

	if (existing_mSprites) {
		for (auto sp : *existing_mSprites) {
			sp.SetParent(this);
			sp.SetImageIndex(AddImageToList(sp.GetImageFilename()));
			mSprites.push_back(sp);
		}
	}
}

Transformation& osuBackgroundAnimation::GetScreenTransformation()
{
	return mScreenTransformation;
}

Image* osuBackgroundAnimation::GetImageFromIndex(int m_image_index)
{
	return mImageList.GetFromIndex(m_image_index);
}

int osuBackgroundAnimation::GetIndexFromFilename(std::string filename)
{
	return mFileIndices[filename];
}

void osuBackgroundAnimation::Load()
{
	// Read the osb file from the song's directory.
	auto fl = Utility::GetFileListing(Song->SongDirectory);
	auto candidates = filter([](std::filesystem::path p) {
		return p.extension() == ".osb";
	}, fl);

	if (candidates.size())
	{
		std::string head;
		std::fstream s(candidates.at(0), std::ios::in);

		if (std::getline(s, head) && head == "[Events]")
		{
			auto mSprite_list = ReadOSBEvents(s);

			// at this point i honestly forgot the type of this thing
			auto newlist = osb::SpriteList();

			for (auto sp : mSprite_list)
			{
				bool moved = false;
				// Who'd define a background on the .osb?
				// I assume no one.
				for (auto &&s: mSprites)
				{
					if (s.GetLayer() == osb::LAYER_SP_BACKGROUND)
					{
						if (s.GetImageFilename() == sp.GetImageFilename())
						{
							s.CopyEventsFrom(sp);
							moved = true;
						}
					}
				}

				if (!moved) {
					sp.SetParent(this);
					sp.SetImageIndex(AddImageToList(sp.GetImageFilename()));
					newlist.push_back(sp);
				}
			}
			
			newlist.insert(newlist.end(), mSprites.begin(), mSprites.end());
			mSprites = newlist;
		}
	}

	for (auto &&i : mFileIndices) {
		mImageList.AddToListIndex(i.first, "", i.second);
		CheckInterruption();
	}

	CanValidate = true;
}

void osuBackgroundAnimation::Validate()
{
	if (!CanValidate)
	{
		Log::LogPrintf("Can't validate osu! storyboard.");
		return;
	}

	mImageList.LoadAll();
	mImageList.ForceFetch();

	// Count items/layer
	std::map<int, int> cnt;
	for (auto &&i : mSprites)
		cnt[i.GetLayer()]++;

	// Set size of sprite containers
	mAutoBGLayer.resize(cnt[osb::LAYER_SP_BACKGROUND], Sprite(false));
	mBackgroundLayer.resize(cnt[osb::LAYER_BACKGROUND] + cnt[osb::LAYER_PASS] + cnt[osb::LAYER_FAIL], Sprite(false));
	mForegroundLayer.resize(cnt[osb::LAYER_FOREGROUND], Sprite(false));

	// counters for each layer (only first three used atm)
	auto i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;

	// assign pre-reserved space to these layers
	for (auto &&i : mSprites)
	{
		Sprite* spr = nullptr;
		switch (i.GetLayer())
		{
		case osb::LAYER_SP_BACKGROUND:
			spr = &mAutoBGLayer[i1];
			i1++;
			break;
		case osb::LAYER_BACKGROUND:
		case osb::LAYER_PASS:
		case osb::LAYER_FAIL:
			spr = &mBackgroundLayer[i2];
			i2++;
			break;
		case osb::LAYER_FOREGROUND:
			spr = &mForegroundLayer[i3];
			i3++;
			break;
		default:
			if (OSBDebug)
				Log::LogPrintf("Discarding sprite %s for being in layer %i.", i.GetImageFilename().c_str(), i.GetLayer());
		}

		if (spr) {
			i.SetSprite(spr);
			i.InitializeSprite();
		} else
		{
			if (OSBDebug)
				Log::LogPrintf(" Warning: Non-existing layer.\n");
		}
	}
}

void osuBackgroundAnimation::SetAnimationTime(double Time)
{
	for (auto&& item: mSprites)
	{
		item.Update(Time);
	}
}

void osuBackgroundAnimation::Update(float Delta)
{
}

void osuBackgroundAnimation::Render()
{
	for (auto&& item : mAutoBGLayer)
		item.Render();
	for (auto&& item: mBackgroundLayer)
		item.Render();
	for (auto&& item: mForegroundLayer)
		item.Render();
}

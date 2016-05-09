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
	template<class T>
	std::shared_ptr<T> cp_event_cast(const std::shared_ptr<Event> &evt)
	{
		return std::make_shared<T>(*std::static_pointer_cast<T>(evt));
	}

	auto copy_event = [](const std::shared_ptr<Event>& evt) -> std::shared_ptr<Event>
	{
		// we've got to use the appropiate type copy-constructor, then we can treat as generic event.
		switch (evt->GetEventType())
		{
		case EVT_MOVEX:
			return cp_event_cast<MoveXEvent>(evt);
		case EVT_MOVEY:
			return cp_event_cast<MoveYEvent>(evt);
		case EVT_SCALE:
			return cp_event_cast<FadeEvent>(evt);
		case EVT_SCALEVEC:
			return cp_event_cast<VectorScaleEvent>(evt);
		case EVT_ROTATE:
			return cp_event_cast<RotateEvent>(evt);
		case EVT_COLORIZE:
			return cp_event_cast<ColorizeEvent>(evt);
		case EVT_FADE:
			return cp_event_cast<FadeEvent>(evt);
		case EVT_ADDITIVE:
			return cp_event_cast<AdditiveEvent>(evt);
		case EVT_HFLIP:
			return cp_event_cast<FlipHorizontalEvent>(evt);
		case EVT_VFLIP:
			return cp_event_cast<FlipVerticalEvent>(evt);
		default:
			throw std::runtime_error("Invalid event type %d\n");
		}
	};


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


	EventList::iterator BGASprite::GetEvent(float& Time, EEventType evt)
	{
		return lower_bound(mEventList[evt].begin(), mEventList[evt].end(), Time, 
			[](const std::shared_ptr<osb::Event>& A, float t) -> bool
		{ 
			return A->Time < t;
		});

	}

	bool BGASprite::IsValidEvent(EventList::iterator& evt_iter, EEventType evt)
	{
		bool end_is_not_start = mEventList[evt].begin() != mEventList[evt].end();
		return end_is_not_start;
	}

	EventList& BGASprite::GetEventList(EEventType evt)
	{
		return mEventList[evt];
	}

	template <class T>
	std::shared_ptr<T> event_cast(EventList::iterator& evt, EventList& lst)
	{
		if (evt != lst.begin())
			return std::static_pointer_cast<T>(*(evt - 1));
		return std::static_pointer_cast<T>(*evt);
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

	void BGASprite::Update(float Time)
	{
		assert (mSprite != nullptr);
		assert (mParent != nullptr); // We need this.
		// Now get the values for all the different stuff.
		
		// Okay, a pretty long function follows. Fade first.
		auto fade_evt = GetEvent(Time, EVT_FADE);
		
		if (IsValidEvent(fade_evt, EVT_FADE)) {
			if (WithinEvents(Time))
				mSprite->Alpha = event_cast<FadeEvent>(fade_evt, mEventList[EVT_FADE])->LerpValue(Time);
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
		auto movx_evt = GetEvent(Time, EVT_MOVEX);
		if (IsValidEvent(movx_evt, EVT_MOVEX))
			mTransform.SetPositionX(event_cast<MoveXEvent>(movx_evt, mEventList[EVT_MOVEX])->LerpValue(Time));
		else mTransform.SetPositionX(mStartPos.x);

		auto movy_evt = GetEvent(Time, EVT_MOVEY);
		if (IsValidEvent(movy_evt, EVT_MOVEY))
			mTransform.SetPositionY(event_cast<MoveYEvent>(movy_evt, mEventList[EVT_MOVEY])->LerpValue(Time));
		else mTransform.SetPositionY(mStartPos.y);

		// We already unpacked move events, so no need for this next snip.
		/* auto mov_evt = GetEvent(Time, EVT_MOVE);
		if (IsValidEvent(mov_evt, EVT_MOVE))
			mTransform.SetPosition(event_cast<MoveEvent>(mov_evt)->LerpValue(Time)); */

		// Now scale and rotation.
		float scale = 1;
		auto scale_evt = GetEvent(Time, EVT_SCALE);
		if (IsValidEvent(scale_evt, EVT_SCALE))
			scale = event_cast<ScaleEvent>(scale_evt, mEventList[EVT_SCALE])->LerpValue(Time);
		else scale = 1;

		// we want to scale it to fit - but we don't want to alter the scale set by the user
		// scales just get multiplied so we'll do that
		if (mLayer == osb::LAYER_SP_BACKGROUND && mSprite->GetImage())
			scale *= OSB_HEIGHT / mSprite->GetImage()->h;

		// Since scale is just applied to size straight up, we can use this extra scale
		// defaulting at 1,1 to be our vector scale. That way they'll pile up.
		Vec2 vscale;
		auto vscale_evt = GetEvent(Time, EVT_SCALEVEC);
		if (IsValidEvent(vscale_evt, EVT_SCALEVEC))
			vscale = event_cast<VectorScaleEvent>(vscale_evt, mEventList[EVT_SCALEVEC])->LerpValue(Time);
		else vscale = Vec2(1, 1);

		auto rot_evt = GetEvent(Time, EVT_ROTATE);
		float rot = 0;
		if (IsValidEvent(rot_evt, EVT_ROTATE)) {
			rot = event_cast<RotateEvent>(rot_evt, mEventList[EVT_ROTATE])->LerpValue(Time);
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


		auto colorization_evt = GetEvent(Time, EVT_COLORIZE);
		if (IsValidEvent(colorization_evt, EVT_COLORIZE)) {
			auto lerp = event_cast<ColorizeEvent>(colorization_evt, mEventList[EVT_COLORIZE])->LerpValue(Time);
			mSprite->Red = lerp.r;
			mSprite->Green = lerp.g;
			mSprite->Blue = lerp.b;
		}

		// The effects after this don't set values before they begin. (Parameter)
		auto additive_evt = GetEvent(Time, EVT_ADDITIVE);
		if (additive_evt != mEventList[EVT_ADDITIVE].begin()
			&& mEventList[EVT_ADDITIVE].begin() != mEventList[EVT_ADDITIVE].end()
			&& (additive_evt - 1)->get()->GetEndTime() <= Time)
			mSprite->SetBlendMode(BLEND_ADD);
		else mSprite->SetBlendMode(BLEND_ALPHA);

		auto hflip_evt = GetEvent(Time, EVT_HFLIP);
		if (hflip_evt != mEventList[EVT_HFLIP].begin() 
			&& mEventList[EVT_HFLIP].begin() != mEventList[EVT_HFLIP].end())
		{
			if ((hflip_evt - 1)->get()->GetEndTime() <= Time)
			{
				mFlip.SetScaleX(-1);
				mFlip.SetPositionX(1);
			} else
			{
				mFlip.SetScaleX(1);
				mFlip.SetPositionX(0);
			}
		}

		auto vflip_evt = GetEvent(Time, EVT_VFLIP);
		if (vflip_evt != mEventList[EVT_VFLIP].begin()
			&& mEventList[EVT_VFLIP].begin() != mEventList[EVT_VFLIP].end())
		{
			if ((vflip_evt - 1)->get()->GetEndTime() <= Time)
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

	void EventComponent::SortEvents()
	{
		auto ecmp = [](const std::shared_ptr<Event> &A, const std::shared_ptr<Event> &B) {
			return A->GetTime() < B->GetTime();
		};

		for (auto i = 0; i < EVT_COUNT; i++)
			stable_sort(mEventList[i].begin(), mEventList[i].end(), ecmp );
	}

	bool EventComponent::WithinEvents(float Time) const
	{
		return Time >= GetStartTime() && Time <= GetEndTime();
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
		if (event->GetEventType() != EVT_MOVE)
			mEventList[event->GetEventType()].push_back(event);
		else // Unpack move events.
		{
			auto mov = std::static_pointer_cast<MoveEvent>(event);
			auto mxe = std::make_shared<MoveXEvent>();
			auto mye = std::make_shared<MoveYEvent>();

			mxe->SetEase(mov->GetEase());
			mye->SetEase(mov->GetEase());
			
			mxe->SetTime(event->GetTime());
			mye->SetTime(event->GetTime());
			mxe->SetEndTime(event->GetEndTime());
			mye->SetEndTime(event->GetEndTime());

			mxe->SetValue(mov->GetValue().x);
			mye->SetValue(mov->GetValue().y);
			mxe->SetEndValue(mov->GetEndValue().x);
			mye->SetEndValue(mov->GetEndValue().y);

			mEventList[mxe->GetEventType()].push_back(mxe);
			mEventList[mye->GetEventType()].push_back(mye);
		}
		if (event->GetTime() < StartPeriod)
			StartPeriod = event->GetTime();
		if (event->GetEndTime() > EndPeriod)
			EndPeriod = event->GetEndTime();
	}

	float Loop::GetIterationDuration()
	{	
		auto min_start = std::numeric_limits<float>::infinity(); // a mouthful to type.
		for (auto k = 0; k < EVT_COUNT; k++)
			for (auto evt : mEventList[k])
				min_start = std::min(min_start, evt->GetTime());

		auto max_end = -std::numeric_limits<float>::infinity(); // a mouthful to type.
		for (auto k = 0; k < EVT_COUNT; k++)
			for (auto evt : mEventList[k])
				max_end = std::max(max_end, evt->GetEndTime());

		return max_end - min_start;
		/*float dur = 0;
				for (auto k = 0; k < EVT_COUNT; k++)
			for (auto evt : mEventList[k])
				dur = std::max(evt->GetEndTime(), dur);

		return dur;*/
	}

	

	void Loop::Unroll(EventVector& ret)
	{
		double iter_duration = GetIterationDuration();
		

		// okay, osu loops are super funky.
		// a loop's iteration is calculated by dur = last event's end time - first event's start time
		// not just last event's end time, so events are repeated as soon as the last one of the previous one ends
		// that means, the time of the next event is not loop start time + max last time of event * iter
		// but that dur previously mentioned instead.
		for (auto k = 0; k < EVT_COUNT; k++)
		{
			for (auto evt : mEventList[k]) {
				for (auto i = 0; i < LoopCount; i++)
				{
					std::shared_ptr<Event> new_evt = copy_event(evt);
					new_evt->SetTime(evt->GetTime() + iter_duration * i + Time);
					new_evt->SetEndTime(evt->GetEndTime() + iter_duration * i + Time);

					// Add into the unrolled events list
					ret[k].push_back(new_evt);
				}
			}
		}
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
		loop->Unroll(vec);
		for (auto i = 0; i < osb::EVT_COUNT; i++)
			for (auto evt : vec[i])
			{
				if (!sprite)
					throw std::runtime_error("OSB command unpaired with sprite.");
				sprite->AddEvent(evt);
			}
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

				if (previous_background) {
					float T = 1;
					auto prev_evt = previous_background->GetEvent(T, osb::EVT_FADE);
					if (previous_background->IsValidEvent(prev_evt, osb::EVT_FADE))
						(osb::event_cast<osb::FadeEvent>(prev_evt, previous_background->GetEventList(osb::EVT_FADE)))->SetEndTime(time);
				}

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

osuBackgroundAnimation::osuBackgroundAnimation(VSRG::Song* song, osb::SpriteList* existing_mSprites)
	: mImageList(this)
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
							// Move events from sp into the existing background (that is s)
							for (auto i = 0; i < osb::EVT_COUNT; i++)
							{
								auto el = sp.GetEventList(osb::EEventType(i));
								for (auto &&ev : el)
									s.AddEvent(osb::copy_event(ev));
							}

							s.SortEvents();
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

	for (auto &&i : mFileIndices)
		mImageList.AddToListIndex(i.first, "", i.second);

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

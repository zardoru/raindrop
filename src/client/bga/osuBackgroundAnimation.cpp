#include <thread>
#include <fstream>
#include <functional>
#include <regex>
#include <rmath.h>
#include <game/Timing.h>

#include <game/Easing.h>
#include "Transformation.h"
#include "Rendering.h"
#include "Sprite.h"

#include "BackgroundAnimation.h"
#include "ImageList.h"
#include "osuBackgroundAnimation.h"

#include <boost/algorithm/string/case_conv.hpp>
#include <utility>
#include <text_and_file_util.h>
#include "Texture2D.h"
#include "Logging.h"

#include "VideoPlayback.h"

#include "../structure/Configuration.h"

constexpr float OSB_WIDTH = 640;
constexpr float OSB_WIDTH_WIDE = 853;
constexpr float OSB_HEIGHT = 480;
CfgVar OSBDebug("OSB", "Debug");

// Must match PP_* ordering. Displacement to apply to a top-left origin quad with size 1 depending on pivot.
// X first, then Y.
struct pivot
{
	float x, y;
};

constexpr pivot OriginPivots[] = {
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

const std::function<float(float)> EasingFuncs[] = {
    passthrough,
    pow_ease_out<2>,
    pow_ease_in<2>,
    pow_ease_out<2>,
    pow_ease_in<2>,
    pow_ease_inout<2>,
    pow_ease_in<3>,
    pow_ease_out<3>,
    pow_ease_inout<3>,
    pow_ease_in<4>,
    pow_ease_out<4>,
    pow_ease_inout<4>,
    pow_ease_in<5>,
    pow_ease_out<5>,
    pow_ease_inout<5>,
};


namespace osb {
	Event::Event(const EEventType typ) :
        mEvtType(typ), mEase(EASE_NONE)
    {
        Time = 0; EndTime = 0;
    }

	EEventType Event::get_event_type() const
    {
        return mEvtType;
    }

	float Event::get_time() const
    {
        return Time;
    }

	float Event::get_end_time() const
    {
        return EndTime;
    }

	float Event::get_duration() const
    {
        return EndTime - Time;
    }

	int Event::get_ease() const
    {
        return static_cast<int>(mEase);
    }

	void Event::set_time(const float time)
    {
        Time = time;
    }

	void Event::set_end_time(const float EndTime)
    {
        this->EndTime = EndTime;
    }

	void Event::set_ease(const int val)
    {
        mEase = static_cast<EEase>(clamp(val, static_cast<int>(EEase::EASE_NONE), static_cast<int>(EEase::EASE_COUNT) - 1));
    }

	float SingleValEvent::get_value() const
    {
        return Value;
    }

	void SingleValEvent::set_value(const float value)
    {
        Value = value;
    }

	auto SingleValEvent::get_end_value() const -> float {
        return EndValue;
    }

	void SingleValEvent::set_end_value(const float EndValue)
    {
        this->EndValue = EndValue;
    }

	float SingleValEvent::lerp_value(const float At) const
    {
        return Lerp(Value, EndValue, EasingFuncs[get_ease()](clamp((At - Time) / get_duration(), 0.f, 1.f)));
    }

	Vec2 TwoValEvent::get_value() const
    {
        return Value;
    }

	Vec2 TwoValEvent::get_end_value() const
    {
        return EndValue;
    }

	void TwoValEvent::set_value(const Vec2 val) {
        Value = val;
    }

	void TwoValEvent::set_end_value(const Vec2 val)
    {
        EndValue = val;
    }

	Vec2 TwoValEvent::lerp_value(const float At) const
    {
        return Lerp(Value, EndValue, EasingFuncs[get_ease()](clamp((At - Time) / get_duration(), 0.f, 1.f)));
    }

	Vec3 ColorizeEvent::get_value() const
    {
        return Value;
    }

	Vec3 ColorizeEvent::get_end_value() const
    {
        return EndValue;
    }

	void ColorizeEvent::set_value(const Vec3 val)
    {
        Value = val;
    }

	void ColorizeEvent::set_end_value(const Vec3 val)
    {
        EndValue = val;
    }

	Vec3 ColorizeEvent::lerp_value(const float At) const
    {
        float factor = 1.f / 255.f;
        return Lerp(Value, EndValue, EasingFuncs[get_ease()](clamp((At - Time) / get_duration(), 0.f, 1.f))) * factor;
    }

	BGASprite::BGASprite(std::string file, const EOrigin origin, const Vec2 start_pos, const ELayer layer) : EventComponent(EVT_COUNT)
    {
        mFile = std::move(file);
        mOrigin = origin;
        mStartPos = start_pos;
        mLayer = layer;

        mSprite = nullptr;
        mParent = nullptr;
        mImageIndex = -1;
        mUninitialized = true;
    }

	void BGASprite::set_sprite(Sprite* sprite)
    {
        mSprite = sprite;
    }


	void BGASprite::initialize_sprite()
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
            mTransform.chain_transformation(&mParent->get_screen_transformation());

            // flip -> pivot
            mPivot.set_position(OriginPivots[mOrigin].x, OriginPivots[mOrigin].y);
            mPivot.chain_transformation(&mTransform);
			
            // sprite -> flip vertices
            mFlip.chain_transformation(&mPivot);

            // No op from sprite.
            mSprite->chain_transformation(&mFlip);

            // Set the image.
            mSprite->set_image(mParent->get_image_from_index(mImageIndex), false);

            mUninitialized = false;
        }
    }

	template <class T>
    T::iterator get_event(double time, T& vec)
	{
		return lower_bound(vec.begin(), vec.end(), time, [](const typename T::value_type & v, const double &TT)
		{
			return v.Time < TT;
		});
	}

	template <class T>
	bool validate_event_iterator(typename T::iterator &it, T& vec)
	{
		if (vec.begin() == vec.end()) return false; // don't use this iter
		if (it != vec.begin()) it--;
		return true;
	}

	void BGASprite::update(const float time)
	{
		assert (mSprite != nullptr);
		assert (mParent != nullptr); // We need this.
		// Now get the values for all the different stuff.
		
		// Okay, a pretty long function follows. Fade first.
		auto fade_evt = get_event(time, evFade);
		
		if (validate_event_iterator(fade_evt, evFade)) {
			if (is_time_in_event_bounds(time))
					mSprite->color.alpha = fade_evt->lerp_value(time);
			else {
				if (fade_evt->get_time() == 0 && mLayer == LAYER_SP_BACKGROUND)
					mSprite->color.alpha = 1;
				else
					mSprite->color.alpha = 0;
			}
		}
		else {
			if (is_time_in_event_bounds(time))
				mSprite->color.alpha = 1;
			else
				mSprite->color.alpha = 0;
		}

		// Don't bother updating unless we're visible.
		if (mSprite->color.alpha == 0)
			return;

		// Now position.	
		auto movx_evt = get_event(time, evMoveX);
		if (validate_event_iterator(movx_evt, evMoveX))
			mTransform.set_position_x(movx_evt->lerp_value(time));
		else mTransform.set_position_x(mStartPos.x);

		auto movy_evt = get_event(time, evMoveY);
		if (validate_event_iterator(movy_evt, evMoveY))
			mTransform.set_position_y(movy_evt->lerp_value(time));
		else mTransform.set_position_y(mStartPos.y);

		// We already unpacked move events, so no need for this next snip.
		/* auto mov_evt = GetEvent(Time, EVT_MOVE);
		if (IsValidEvent(mov_evt, EVT_MOVE))
			mTransform.SetPosition(event_cast<MoveEvent>(mov_evt)->LerpValue(Time)); */

		// Now scale and rotation.
		float scale = 1;
		auto scale_evt = get_event(time, evScale);
		if (validate_event_iterator(scale_evt, evScale))
			scale = scale_evt->lerp_value(time);
		else if (mLayer == osb::LAYER_SP_BACKGROUND && mSprite->get_image())
			scale *= OSB_WIDTH_WIDE / mSprite->get_image()->w;
		else scale = 1;
		// we want to scale it to fit - but we don't want to alter the scale set by the user
		// scales just get multiplied so we'll do that

		// Since scale is just applied to size straight up, we can use this extra scale
		// defaulting at 1,1 to be our vector scale. That way they'll pile up.
		Vec2 vscale;
		auto vscale_evt = get_event(time, evScaleVec);
		if (validate_event_iterator(vscale_evt, evScaleVec))
			vscale = vscale_evt->lerp_value(time);
		else vscale = Vec2(1, 1);

		auto rot_evt = get_event(time, evRotate);
		float rot = 0;
		if (validate_event_iterator(rot_evt, evRotate)) {
			rot = rot_evt->lerp_value(time);
			mTransform.set_rotation(rot);
		}
		else mTransform.set_rotation(0);

		if (mSprite->get_image())
		{
			auto i = mSprite->get_image();

			// Move, then scale (is the way transformations are set up
			// therefore, pivot is applied, then scale
			// then both size and scale on mTransform are free for usage.

			// Set active scales.
			mTransform.set_size(i->w * scale * vscale.x, i->h * scale * vscale.y);

            if (const auto vid = dynamic_cast<VideoPlayback*>(i)) {
				vid->update_clock(time - evFade.begin()->get_time());
			}
		}


		auto colorization_evt = get_event(time, evColorize);
		if (validate_event_iterator(colorization_evt, evColorize)) {
			auto lerp = colorization_evt->lerp_value(time);
			mSprite->color.red = lerp.r;
			mSprite->color.green = lerp.g;
			mSprite->color.blue = lerp.b;
		}

		// The effects after this don't set values before they begin. (Parameter)
		auto additive_evt = get_event(time, evAdditive);
		if (additive_evt != evAdditive.begin()
			&& evAdditive.begin() != evAdditive.end()
			&& (additive_evt - 1)->get_end_time() <= time)
			mSprite->set_blend_mode(BLEND_ADD);
		else mSprite->set_blend_mode(BLEND_ALPHA);

		auto hflip_evt = get_event(time, evFlipH);
		if (hflip_evt != evFlipH.begin() 
			&& evFlipH.begin() != evFlipH.end())
		{
			if ((hflip_evt - 1)->get_end_time() <= time)
			{
				mFlip.set_scale_x(-1);
				mFlip.set_position_x(1);
			} else
			{
				mFlip.set_scale_x(1);
				mFlip.set_position_x(0);
			}
		}

		auto vflip_evt = get_event(time, evFlipV);
		if (vflip_evt != evFlipV.begin()
			&& evFlipV.begin() != evFlipV.end())
		{
			if ((vflip_evt - 1)->get_end_time() <= time)
			{
				mFlip.set_scale_y(-1);
				mFlip.set_position_y(1);
			} else
			{
				mFlip.set_scale_y(1);
				mFlip.set_position_y(0);
			}
		}
	}

	std::string BGASprite::get_image_filename() const
	{
		return mFile;
	}

	ELayer BGASprite::get_layer() const
	{
		return mLayer;
	}

	void BGASprite::set_parent(osuBackgroundAnimation* parent)
	{
		mParent = parent;
	}

	void BGASprite::set_image_index(const int index)
	{
		mImageIndex = index;
	}

	template <class T>
	void sort_event_list(T& vec)
	{
		auto cmp = [](const typename T::value_type& A, const typename T::value_type& B)
		{
			return A.get_time() < B.get_time();
		};

		stable_sort(vec.begin(), vec.end(), cmp);
		vec.shrink_to_fit();
	}

	void EventComponent::sort_events()
	{
		sort_event_list(evMoveX);
		sort_event_list(evMoveY);
		sort_event_list(evScale);
		sort_event_list(evScaleVec);
		sort_event_list(evRotate);
		sort_event_list(evColorize);
		sort_event_list(evFade);
		sort_event_list(evFlipH);
		sort_event_list(evFlipV);
		sort_event_list(evAdditive);

		get_duration();
	}

	bool EventComponent::is_time_in_event_bounds(const float Time) const
	{
		return Time >= get_start_time() && Time <= get_end_time();
	}

	void EventComponent::copy_events_from(const EventComponent &ec)
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

		get_duration(); // recalc. start/end periods
	}

	EventComponent::EventComponent(const EEventType evt):
		Event(evt), 
		StartPeriod(std::numeric_limits<float>::infinity()), 
		EndPeriod(-StartPeriod)
	{}

	float EventComponent::get_start_time() const
	{
		return StartPeriod;
	}

	float EventComponent::get_end_time() const
	{
		return EndPeriod;
	}

	void EventComponent::add_event(const std::shared_ptr<Event> &event)
	{
		if (event->get_event_type() != EVT_MOVE) {
			switch (event->get_event_type()) {
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
			default:
				break;
			}
		}
		else // Unpack move events.
		{
			auto mov = std::static_pointer_cast<MoveEvent>(event);
			auto mxe = MoveXEvent();
			auto mye = MoveYEvent();

			mxe.set_ease(mov->get_ease());
			mye.set_ease(mov->get_ease());
			
			mxe.set_time(mov->get_time());
			mye.set_time(mov->get_time());
			mxe.set_end_time(mov->get_end_time());
			mye.set_end_time(mov->get_end_time());

			mxe.set_value(mov->get_value().x);
			mye.set_value(mov->get_value().y);
			mxe.set_end_value(mov->get_end_value().x);
			mye.set_end_value(mov->get_end_value().y);

			evMoveX.push_back(mxe);
			evMoveY.push_back(mye);
		}
		if (event->get_time() < StartPeriod)
			StartPeriod = event->get_time();
		if (event->get_end_time() > EndPeriod)
			EndPeriod = event->get_end_time();
	}

	void EventComponent::clear_events()
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
		for (const auto &evt : vec)
			min = std::min(min, evt.get_time());
	}


	template <class T>
	void maximize(T vec, float &max)
	{
		for (const auto &evt : vec)
			max = std::max(max, evt.get_end_time());
	}

	float EventComponent::get_duration()
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

		StartPeriod = min_end;
		EndPeriod = max_end;
		return max_end - min_end;
	}

	
	template <class T>
	void unroll_events(const double iter_duration, double time, const uint32_t loop_count, T& dst, T& src)
	{
		for (auto &evt : src) {
			for (uint32_t i = 0; i < loop_count; i++) {
				auto el = evt;
				el.set_time(evt.get_time() + iter_duration * i + time);
				el.set_end_time(evt.get_end_time() + iter_duration * i + time);

				// Add into the unrolled events list
				dst.push_back(el);
			}
		}
	}

	void Loop::Unroll(EventComponent *ec)
	{
		if (!ec) throw std::runtime_error("No event component to unroll to.");

		double iter_duration = get_duration();


		// okay, osu loops are super funky.
		// a loop's iteration is calculated by dur = last event's end time - first event's start time
		// not just last event's end time, so events are repeated as soon as the last one of the previous one ends
		// that means, the time of the next event is not loop start time + max last time of event * iter
		// but that dur previously mentioned instead.
		unroll_events(iter_duration, Time, LoopCount, ec->evMoveX, evMoveX);
		unroll_events(iter_duration, Time, LoopCount, ec->evMoveY, evMoveY);
		unroll_events(iter_duration, Time, LoopCount, ec->evScale, evScale);
		unroll_events(iter_duration, Time, LoopCount, ec->evScaleVec, evScaleVec);
		unroll_events(iter_duration, Time, LoopCount, ec->evRotate, evRotate);
		unroll_events(iter_duration, Time, LoopCount, ec->evColorize, evColorize);
		unroll_events(iter_duration, Time, LoopCount, ec->evFade, evFade);
		unroll_events(iter_duration, Time, LoopCount, ec->evFlipH, evFlipH);
		unroll_events(iter_duration, Time, LoopCount, ec->evFlipV, evFlipV);
		unroll_events(iter_duration, Time, LoopCount, ec->evAdditive, evAdditive);
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
		xvt->set_value(Vec2(latof(split[4]), latof(split[5])));
		if (split.size() > 6)
			xvt->set_end_value(Vec2(latof(split[6]), latof(split[7])));
		else 
			xvt->set_end_value(Vec2(latof(split[4]), latof(split[5])));
		evt = xvt;
	}
	if (ks == "S") {
		auto xvt = std::make_shared<osb::ScaleEvent>();
		xvt->set_value(latof(split[4]));
		if (split.size() > 5)
			xvt->set_end_value(latof(split[5]));
		else
			xvt->set_end_value(latof(split[4]));
		evt = xvt;
	}
	if (ks == "M") {
		auto xvt = std::make_shared<osb::MoveEvent>();
		xvt->set_value(Vec2(latof(split[4]), latof(split[5])));
		if (split.size() > 6)
			xvt->set_end_value(Vec2(latof(split[6]), latof(split[7])));
		else
			xvt->set_end_value(Vec2(latof(split[4]), latof(split[5])));
		evt = xvt;
	}
	if (ks == "MX") {
		auto xvt = std::make_shared<osb::MoveXEvent>();
		xvt->set_value(latof(split[4]));
		if (split.size() > 5)
			xvt->set_end_value(latof(split[5]));
		else
			xvt->set_end_value(latof(split[4]));
		evt = xvt;
	}
	if (ks == "MY") {
		auto xvt = std::make_shared<osb::MoveYEvent>();
		xvt->set_value(latof(split[4]));
		if (split.size() > 5)
			xvt->set_end_value(latof(split[5]));
		else
			xvt->set_end_value(latof(split[4]));
		evt = xvt;
	}
	if (ks == "C") {
		auto xvt = std::make_shared<osb::ColorizeEvent>();
		xvt->set_value(Vec3(latof(split[4]), latof(split[5]), latof(split[6])));
		if (split.size() > 7)
			xvt->set_end_value(Vec3(latof(split[7]), latof(split[8]), latof(split[9])));
		else
			xvt->set_end_value(Vec3(latof(split[4]), latof(split[5]), latof(split[6])));
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
		xvt->set_value(latof(split[4]));
		if (split.size() > 5)
			xvt->set_end_value(latof(split[5]));
		else
			xvt->set_end_value(latof(split[4]));
		evt = xvt;
	}
	if (ks == "L") {
		auto xvt = std::make_shared<osb::Loop>(latof(split[2]));
		evt = xvt;
	}
	if (ks == "R")
	{
		auto xvt = std::make_shared<osb::RotateEvent>();
		xvt->set_value((latof(split[4])));
		if (split.size() > 5)
			xvt->set_end_value((latof(split[5])));
		else
			xvt->set_end_value((latof(split[4])));
		evt = xvt;
	}
	if (ks == "T")
	{
		throw std::runtime_error("OSB: Storyboards using triggers is not supported.");
	}

	if (evt) {
		if (evt->get_event_type() != osb::EVT_LOOP) {
			evt->set_ease(latof(split[1]));
			evt->set_time(latof(split[2].length() ? split[2] : split[3]) / 1000.0);
			evt->set_end_time(latof(split[3].length() ? split[3] : split[2]) / 1000.0);
		}
		else
		{
			evt->set_time(latof(split[1]) / 1000.0);
		}
	}
	return evt;
}

osb::SpriteList read_osb_events(std::istream& event_str)
{
	auto line_nm = 1;
	auto list = osb::SpriteList();
	int previous_lead = 100;
	int loop_lead = -1;
	osb::BGASprite* sprite = nullptr;
	std::shared_ptr<osb::Loop> loop = nullptr;

	std::string line;
	/*
		Though there's an implicit tree structure given by whitespace or underscores
		a la python, it isn't infinitely nested. As such, it's pretty easy to just
		do what is being done here.
	*/

	const std::regex fnreg("\"?(.*?)\"?$");
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

	auto unroll_loop_into_sprite = [&]() {
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

		std::vector<std::string> split_result = otoworm::util::token_split(line, ",");
		for (auto &&s : split_result) boost::algorithm::to_lower(s);

		if (!line.length() || split_result.size() < 2) continue;

		auto parse_sprite = [&]()
		{
			if (split_result[0] == "sprite")
			{
				Vec2 new_position(latof(split_result[4]), latof(split_result[5]));
				std::string striped_filename = strip_quotes(split_result[3]);

				if (sprite)
					sprite->sort_events(); // we're done. clean up and get ready

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
				evt->set_end_value(1);
				evt->set_value(1);
				evt->set_time(0);
				evt->set_end_time(std::numeric_limits<float>::infinity());

				bgsprite.add_event(evt);

				list.insert(list.begin(), bgsprite);
				
				return true;
			}

			if (split_result[0] == "2")
				return true;

			if (split_result[0] == "video") {
				float time = latof(split_result[1]);
				auto bgsprite = osb::BGASprite(
					strip_quotes(split_result[2]), 
					osb::PP_CENTER, 
					Vec2(320, 240),
					osb::LAYER_SP_BACKGROUND);

				auto evt = std::make_shared<osb::FadeEvent>();
				evt->set_end_value(1);
				evt->set_value(1);
				evt->set_time(latof(split_result[1]) / 1000.0f);
				evt->set_end_time(std::numeric_limits<float>::infinity());

				bgsprite.add_event(evt);

				list.insert(list.end(), bgsprite);
			}

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
					loop->add_event(ParseEvent(split_result));
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
						if (ev->get_event_type() == osb::EVT_LOOP)
						{
							loop = std::static_pointer_cast<osb::Loop>(ev);
							loop_lead = lead_spaces;
						}
						else // add this event, if not a loop to this mSprite. It'll be unrolled once outside.
						{
							sprite->add_event(ev);
						}
					}
				}
			}
		}

		previous_lead = lead_spaces;
	}

	if (sprite)
		sprite->sort_events();
	// we have a pending loop? oops sorry
	if (sprite && loop)
		unroll_loop_into_sprite();

	return list;
}

int osuBackgroundAnimation::add_image_to_list(std::string image_filename)
{
	std::filesystem::path fn = image_filename;
	auto idx = m_file_indices_.size() + 1;
	auto full_path = song_directory_ / image_filename;

	if (std::filesystem::exists(full_path)) {
		// Okay, full path is definitely not in.
		if (m_file_indices_.find(full_path.string()) == m_file_indices_.end()) {
			m_file_indices_[full_path.string()] = idx;
			return idx;
		} // It is? Okay. Give back the full path's index here.
		else
			return m_file_indices_[full_path.string()];
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
				if (m_file_indices_.find(fn.string()) == m_file_indices_.end())
					m_file_indices_[fn.string()] = idx;
				else
					return m_file_indices_[fn.string()];
			}
		}
	}

	return -1; // everything has failed
}

osuBackgroundAnimation::osuBackgroundAnimation(
		Interruptible* parent,
		const osb::SpriteList& existing_mSprites,
		std::filesystem::path song_directory)
	: BackgroundAnimation(parent),
		m_image_list_(this)
{
	set_size(OSB_WIDTH_WIDE, OSB_HEIGHT);
	m_screen_transformation_.set_position_x( (OSB_WIDTH_WIDE - OSB_WIDTH) / 2 / OSB_WIDTH_WIDE);
	m_screen_transformation_.set_size(1 / OSB_WIDTH_WIDE, 1 / OSB_HEIGHT);
	m_screen_transformation_.chain_transformation(this);
	song_directory_ = std::move(song_directory);
	can_validate_ = false;

	int video_index = 0;
    for (auto sp : existing_mSprites) {
        sp.set_parent(this);

        auto vpath = song_directory_ / sp.get_image_filename();
        if (is_video_path(vpath)) {
            video_index--;
            auto vid = m_video_list_[video_index] = new VideoPlayback();
            if (vid->open(vpath)) {
                vid->start_decode_thread();
                m_image_list_.add_to_list_index(vid, video_index);
            }
        } else {
            sp.set_image_index(add_image_to_list(sp.get_image_filename()));
        }
        m_sprites_.push_back(sp);
	}
}

osuBackgroundAnimation::~osuBackgroundAnimation()
{
	for (auto v: m_video_list_) {
		delete v.second;
	}
}
	
Transformation& osuBackgroundAnimation::get_screen_transformation()
{
	return m_screen_transformation_;
}

Texture2D* osuBackgroundAnimation::get_image_from_index(const int m_image_index)
{
	if (m_image_index >= 0)
		return m_image_list_.get_from_index(m_image_index);
	else
		return m_video_list_[m_image_index];
}

int osuBackgroundAnimation::get_index_from_filename(std::string filename)
{
	return m_file_indices_[filename];
}

void osuBackgroundAnimation::load()
{
	// Read the osb file from the song's directory.
	std::vector<std::filesystem::path> candidates;
	for (const auto& entry : std::filesystem::directory_iterator(song_directory_)) {
		if (entry.path().extension() == ".osb")
			candidates.push_back(entry.path());
	}

	if (candidates.size())
	{
		std::string head;
		std::fstream s(candidates.at(0).string(), std::ios::in);

		if (std::getline(s, head) && head == "[Events]")
		{
			auto mSprite_list = read_osb_events(s);

			// at this point i honestly forgot the type of this thing
			auto newlist = osb::SpriteList();
			bool bgOverwritten = false;

			for (auto sp : mSprite_list)
			{
				bool moved = false;
				// Who'd define a background on the .osb?
				// I assume no one.
				// We only want to do this check once.
				if (!bgOverwritten) {
					for (auto &&s: m_sprites_) {
						// we only want to possibly move these once
						if (s.get_layer() == osb::LAYER_SP_BACKGROUND) {
							if (s.get_image_filename() == sp.get_image_filename()) {
								s.copy_events_from(sp);
								moved = true;
								bgOverwritten = true;
							}
						}
					}
				}
			

				if (!moved) {
					sp.set_parent(this);
					sp.set_image_index(add_image_to_list(sp.get_image_filename()));
					newlist.push_back(sp);
				}
			}
			
			newlist.insert(newlist.end(), m_sprites_.begin(), m_sprites_.end());
			m_sprites_ = newlist;
		}
	}

	for (auto &&i : m_file_indices_) {
		m_image_list_.add_to_list_index(i.first, i.second);
		CheckInterruption();
	}

	can_validate_ = true;
}

void osuBackgroundAnimation::finalize_loading()
{
	if (!can_validate_)
	{
		Log::LogPrintf("Can't validate osu! storyboard.");
		return;
	}

	m_image_list_.load_all();
	m_image_list_.force_fetch();

	// Count items/layer
	std::map<int, int> cnt;
	for (auto &&i : m_sprites_)
		cnt[i.get_layer()]++;

	// Set size of sprite containers
	m_auto_bg_layer_.resize(cnt[osb::LAYER_SP_BACKGROUND], Sprite(false));
	m_background_layer_.resize(cnt[osb::LAYER_BACKGROUND] + cnt[osb::LAYER_PASS] + cnt[osb::LAYER_FAIL], Sprite(false));
	m_foreground_layer_.resize(cnt[osb::LAYER_FOREGROUND], Sprite(false));

	// counters for each layer (only first three used atm)
	auto i1 = 0, i2 = 0, i3 = 0, i4 = 0, i5 = 0;

	// assign pre-reserved space to these layers
	for (auto &&i : m_sprites_)
	{
		Sprite* spr = nullptr;
		switch (i.get_layer())
		{
		case osb::LAYER_SP_BACKGROUND:
			spr = &m_auto_bg_layer_[i1];
			i1++;
			break;
		case osb::LAYER_BACKGROUND:
		case osb::LAYER_PASS:
		case osb::LAYER_FAIL:
			spr = &m_background_layer_[i2];
			i2++;
			break;
		case osb::LAYER_FOREGROUND:
			spr = &m_foreground_layer_[i3];
			i3++;
			break;
		default:
			if (OSBDebug)
				Log::LogPrintf("Discarding sprite %s for being in layer %i.", i.get_image_filename().c_str(), i.get_layer());
		}

		if (spr) {
			i.set_sprite(spr);
			i.initialize_sprite();
		} else
		{
			if (OSBDebug)
				Log::LogPrintf(" Warning: Non-existing layer.\n");
		}
	}
}

void osuBackgroundAnimation::set_animation_time(const double time)
{
	if (!can_validate_)
		return;

	for (auto&& item: m_sprites_)
	{
        item.update(time);
	}
}

void osuBackgroundAnimation::update(float delta)
{
}

void osuBackgroundAnimation::emit_draw_calls(DrawCallSink &sink)
{
	for (auto&& item : m_auto_bg_layer_)
		item.emit_draw_calls(sink);
	for (auto&& item: m_background_layer_)
		item.emit_draw_calls(sink);
	for (auto&& item: m_foreground_layer_)
		item.emit_draw_calls(sink);
}

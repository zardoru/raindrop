#include "pch.h"
#include "GameGlobal.h"
#include "BackgroundAnimation.h"
#include "Song.h"
#include "Song7K.h"
#include "ImageList.h"
#include "osuBackgroundAnimation.h"

#include <boost/algorithm/string/case_conv.hpp>
#include "Image.h"

const double OSB_WIDTH = 640;
const double OSB_HEIGHT = 480;

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
		mEase = static_cast<EEase>(Clamp(val, (int)EEase::EASE_NONE, (int)EEase::EASE_4INOUT));
	}

	float SingleValEvent::LerpValue(float At) const
	{
		return Lerp(Value, EndValue, Clamp((At - Time) / GetDuration(), 0.f, 1.f));
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
		return Lerp(Value, EndValue, Clamp((At - Time) / GetDuration(), 0.f, 1.f));
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

	Vec3 ColorizeEvent::LerpValue(float At)
	{
		return Lerp(Value, EndValue, Clamp((At - Time) / GetDuration(), 0.f, 1.f)) * 1.f / 255.f;
	}

	BGASprite::BGASprite(std::string file, EOrigin origin, Vec2 start_pos) : EventComponent(EVT_COUNT)
	{
		mFile = file;
		mOrigin = origin;
		mStartPos = start_pos;

		mParent = nullptr;
		mImageIndex = -1;
	}

	void BGASprite::SetSprite(std::shared_ptr<Sprite> sprite)
	{
		mSprite = sprite;
	}


	EventList::iterator BGASprite::GetEvent(float& Time, EEventType evt)
	{
		return lower_bound(mEventList[evt].begin(), mEventList[evt].end(), Time, 
			[](const std::shared_ptr<osb::Event>& A, float Time) -> bool
		{ 
			return A->Time < Time;
		});

	}

	bool BGASprite::IsValidEvent(EventList::iterator& evt_iter, EEventType evt)
	{
		bool end_is_not_start = mEventList[evt].begin() != mEventList[evt].end();
		return end_is_not_start;
	}

	template <class T>
	std::shared_ptr<T> event_cast(EventList::iterator& evt, EventList& lst)
	{
		if (evt != lst.begin())
			return std::static_pointer_cast<T>(*(evt - 1));
		return std::static_pointer_cast<T>(*evt);
	}
	
	void BGASprite::Update(float Time)
	{
		assert(mSprite);
		if (!mParent) return; // We need this.
		if (mImageIndex == -1) // We haven't initialized from the parent's data yet? Alright.
		{
			// -> == then
			assert(mParent != nullptr);
			mImageIndex = mParent->GetIndexFromFilename(mFile);

			// Starts from the sprite, at the bottom. Read bottom to top to see how transformations are applied.

			// Steamlined, in order
			// mFlip
			// Flip: scale by negative, then translate (would change without assumptions to quad being top-left on origin!)

			// mPivot
			// pivot: translate quad with top-left origin to specified pivot

			// mRotate
			// rotate this resized item with new pivot and size (order between rot/scale matters - it's set up like scale then rot internally)

			// mItemScale
			// scale by texture relative to osu!'s storyboard 640x480 space into a 1x1 space
			// what should matter is that it should be done after pivoting
			// osu! wiki states scale and implicitly vecscale are affected by pivot.

			// mTransform
			// scale item up by scale and vscale command
			// apply position

			// rotate then scale to sb space then scale to command is the overall order on osu
			// scale then rotate is our internal order, so we must add extra transforms.
			// maybe in the future just use matrices directly, or make a class
			// that "chains" matrices arbitrarily.
			
			// rotation -> scale + vecscale -> position.
			mTransform.ChainTransformation(&mParent->GetTransformation());

			// shouldnt matter if size or rotation comes first
			// rotation -> item scale
			mItemScale.ChainTransformation(&mTransform);

			// pivot -> rotation
			mRotation.ChainTransformation(&mItemScale);

			// flip -> pivot
			mPivot.ChainTransformation(&mRotation);
			
			// sprite -> flip vertices
			mFlip.ChainTransformation(&mPivot);

			// No op from sprite.
			mSprite->ChainTransformation(&mFlip);
			mPivot.SetPosition(OriginPivots[mOrigin].x, OriginPivots[mOrigin].y);
		}

		// Now get the values for all the different stuff.
		// Set the image and reset size since we're using the pivot.
		// Also forces the matrix to be recalculated.
		mSprite->SetImage(mParent->GetImageFromIndex(mImageIndex), false);
		mSprite->SetSize(1, 1);

		if (mSprite->GetImage())
		{
			auto i = mSprite->GetImage();

			// Move, then scale (is the way transformations are set up
			// therefore, pivot is applied, then scale
			// then both size and scale on mTransform are free for usage.
			mItemScale.SetSize(i->w / OSB_WIDTH, i->h / OSB_HEIGHT);
		}

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

		// Now position.	
		auto movx_evt = GetEvent(Time, EVT_MOVEX);
		if (IsValidEvent(movx_evt, EVT_MOVEX))
			mTransform.SetPositionX(event_cast<MoveXEvent>(movx_evt, mEventList[EVT_MOVEX])->LerpValue(Time));
		else mTransform.SetPositionX(mStartPos.x / OSB_WIDTH);

		auto movy_evt = GetEvent(Time, EVT_MOVEY);
		if (IsValidEvent(movy_evt, EVT_MOVEY))
			mTransform.SetPositionY(event_cast<MoveYEvent>(movy_evt, mEventList[EVT_MOVEY])->LerpValue(Time));
		else mTransform.SetPositionY(mStartPos.y / OSB_HEIGHT);

		// We already unpacked move events, so no need for this next snip.
		/* auto mov_evt = GetEvent(Time, EVT_MOVE);
		if (IsValidEvent(mov_evt, EVT_MOVE))
			mTransform.SetPosition(event_cast<MoveEvent>(mov_evt)->LerpValue(Time)); */

		// Now scale and rotation.
		auto scale_evt = GetEvent(Time, EVT_SCALE);
		if (IsValidEvent(scale_evt, EVT_SCALE))
			mTransform.SetScale(event_cast<ScaleEvent>(scale_evt, mEventList[EVT_SCALE])->LerpValue(Time));
		else mTransform.SetScale(1);

		// Since scale is just applied to size straight up, we can use this extra scale
		// defaulting at 1,1 to be our vector scale. That way they'll pile up.
		auto vscale_evt = GetEvent(Time, EVT_SCALEVEC);
		if (IsValidEvent(vscale_evt, EVT_SCALEVEC))
			mTransform.SetSize(event_cast<VectorScaleEvent>(vscale_evt, mEventList[EVT_SCALEVEC])->LerpValue(Time));
		else mTransform.SetSize(1, 1);

		auto rot_evt = GetEvent(Time, EVT_ROTATE);
		if (IsValidEvent(rot_evt, EVT_ROTATE))
			mRotation.SetRotation(glm::degrees(event_cast<RotateEvent>(rot_evt, mEventList[EVT_ROTATE])->LerpValue(Time)));
		else mRotation.SetRotation(0);


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
				mFlip.SetPositionX(1);
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
				mFlip.SetPositionY(1);
			}
		}
	}

	std::string BGASprite::GetImageFilename() const
	{
		return mFile;
	}

	void BGASprite::SetParent(osuBackgroundAnimation* parent)
	{
		mParent = parent;
	}

	void EventComponent::SortEvents()
	{
		for (auto i = 0; i < EVT_COUNT; i++)
			sort(mEventList[i].begin(), mEventList[i].end());
	}

	bool EventComponent::WithinEvents(float Time)
	{
		return Time >= GetStartTime() && Time <= GetEndTime();
	}

	EventComponent::EventComponent(EEventType evt): 
		Event(evt), 
		StartPeriod(std::numeric_limits<float>::infinity()), 
		EndPeriod(-StartPeriod)
	{}

	float EventComponent::GetStartTime()
	{
		return StartPeriod;
	}

	float EventComponent::GetEndTime()
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
		float dur = 0;
		for (auto k = 0; k < EVT_COUNT; k++)
			for (auto evt : mEventList[k])
				dur = std::max(evt->GetEndTime(), dur);

		return dur;
	}

	void Loop::Unroll(EventVector& ret)
	{
		double iter_duration = GetIterationDuration();

		for (auto i = 0; i < LoopCount; i++)
			for (auto k = 0; k < EVT_COUNT; k++)
				for (auto evt : mEventList[k])
				{
					Event new_evt(*evt);
					// Iteration + Base Loop Time + Event Time
					new_evt.SetTime(new_evt.Time + iter_duration * i + Time);
					new_evt.SetEndTime(new_evt.GetEndTime() + iter_duration * i + Time);

					// Add into the unrolled events list
					ret[k].push_back(std::make_shared<Event>(new_evt));
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

	return osb::PP_TOPLEFT;
}

std::shared_ptr<osb::Event> ParseEvent(std::vector<std::string> split)
{
	auto ks = split[0];
	std::shared_ptr<osb::Event> evt;
	boost::algorithm::to_upper(ks);

	// Calculate all of these WRT their osu! storyboard sizes
	if (ks == "V"){
		// Exception being this.
		auto xvt = std::make_shared<osb::VectorScaleEvent>();
		xvt->SetValue(Vec2(latof(split[4]), latof(split[5])));
		if (split.size() > 6)
			xvt->SetEndValue(Vec2(latof(split[6]), latof(split[7])));
		else 
			xvt->SetEndValue(Vec2(latof(split[4]), latof(split[5])));
		evt = xvt;
	}
	if (ks == "S") {
		// And this.
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
		xvt->SetValue(Vec2(latof(split[4]) / OSB_WIDTH, latof(split[5]) / OSB_HEIGHT));
		if (split.size() > 6)
			xvt->SetEndValue(Vec2(latof(split[6]) / OSB_WIDTH, latof(split[7]) / OSB_HEIGHT));
		else
			xvt->SetEndValue(Vec2(latof(split[4]) / OSB_WIDTH, latof(split[5]) / OSB_HEIGHT));
		evt = xvt;
	}
	if (ks == "MX") {
		auto xvt = std::make_shared<osb::MoveXEvent>();
		xvt->SetValue(latof(split[4]) / OSB_WIDTH);
		if (split.size() > 5)
			xvt->SetEndValue(latof(split[5]) / OSB_WIDTH);
		else
			xvt->SetEndValue(latof(split[4]) / OSB_WIDTH);
		evt = xvt;
	}
	if (ks == "MY") {
		auto xvt = std::make_shared<osb::MoveYEvent>();
		xvt->SetValue(latof(split[4]) / OSB_WIDTH);
		if (split.size() > 5)
			xvt->SetEndValue(latof(split[5]) / OSB_HEIGHT);
		else
			xvt->SetEndValue(latof(split[4]) / OSB_HEIGHT);
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
			xvt->SetValue((latof(split[4])));
		evt = xvt;
	}

	if (evt->GetEventType() != osb::EVT_LOOP) {
		evt->SetEase(latof(split[1]));
		evt->SetTime(latof(split[2].length() ? split[2] : split[3]) / 1000.0);
		evt->SetEndTime(latof(split[3].length() ? split[3] : split[2]) / 1000.0);
	} else
	{
		evt->SetTime(latof(split[1]) / 1000.0);
	}

	return evt;
}

std::shared_ptr<osb::SpriteList> ReadOSBEvents(std::istream& event_str)
{
	auto list = std::make_shared<osb::SpriteList>();
	int previous_lead = 100;
	std::shared_ptr<osb::BGASprite> sprite = nullptr;
	std::shared_ptr<osb::Loop> loop = nullptr;
	bool readingLoop = false;

	std::string line;
	/*
		Though there's an implicit tree structure given by whitespace or underscores
		a la python, it isn't infinitely nested. As such, it's pretty easy to just
		do what is being done here.
	*/

	std::regex fnreg("\"?(.*?)\"?");
	while (std::getline(event_str, line))
	{
		int lead_spaces;
		line = line.substr(0, line.find("//")); // strip comments

		auto sst = line.find_first_not_of("\t _");
		lead_spaces = sst != std::string::npos ? sst : 0;
		line = line.substr(lead_spaces);

		std::vector<std::string> split_result = Utility::TokenSplit(line, ",");
		for (auto &&s : split_result) boost::algorithm::to_lower(s);

		if (!line.length() || split_result.size() < 2) continue;

		auto parse_sprite = [&]()
		{
			if (split_result[0] == "sprite")
			{
				Vec2 new_position(latof(split_result[4]), latof(split_result[5]));
				std::string striped_filename;
				std::smatch sm;

				if (std::regex_match(split_result[3], sm, fnreg))
				{
					striped_filename = sm[1];
				}
				else { striped_filename = split_result[3]; }

				sprite = std::make_shared<osb::BGASprite>(striped_filename, OriginFromString(split_result[2]), new_position);
				list->push_back(sprite);
				return true;
			}

			return false;
		};

		if (lead_spaces < previous_lead && !readingLoop)
		{
			parse_sprite();
		} else {
			if (!sprite)
				throw std::runtime_error("OSB command unpaired with sprite.");

			// If it's a loop, check if we're out of it.
			// If we're out of it, read a regular event, otherwise, read an event to the loop
			if (readingLoop)
			{
				if (lead_spaces < previous_lead) {
					readingLoop = false;

					// We're done reading the loop - unroll it.
					osb::EventVector vec;
					loop->Unroll(vec);
					for (auto i = 0; i < osb::EVT_COUNT; i++)
						for (auto evt : vec[i])
							sprite->AddEvent(evt);
				}
				else
					loop->AddEvent(ParseEvent(split_result));
			}
			
			// It's not a command on the loop, or we weren't reading a loop in the first place.
			// Read a regular command.

			// Not "else" because we do want to execute this if we're no longer reading the loop.
			if (!readingLoop) {
				if (!parse_sprite()) { // Not a sprite, okay...
					auto ev = ParseEvent(split_result);

					// A loop began - set that we are reading a loop and set this loop as where to add the following commands.
					if (ev->GetEventType() == osb::EVT_LOOP)
					{
						loop = std::static_pointer_cast<osb::Loop>(ev);
						readingLoop = true;
					}
					else // add this event, if not a loop to this mSprite. It'll be unrolled once outside.
						sprite->AddEvent(ev);
				}
			}
		}

		previous_lead = lead_spaces;
	}

	return list;
}

void osuBackgroundAnimation::AddImageToList(std::string image_filename)
{
	if (mFileIndices.find(image_filename) == mFileIndices.end())
		mFileIndices[image_filename] = mFileIndices.size() + 1;
}

osuBackgroundAnimation::osuBackgroundAnimation(VSRG::Song* song, std::shared_ptr<osb::SpriteList> existing_mSprites)
	: mImageList(this)
{
	if (existing_mSprites) {
		for (auto sp : *existing_mSprites)
		{
			mSprites.push_back(sp);
			AddImageToList(sp->GetImageFilename());
		}
	}

	Transform.SetSize(OSB_WIDTH, OSB_HEIGHT);
	Song = song;
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
			for (auto sp : *mSprite_list)
			{
				AddImageToList(sp->GetImageFilename());
				sp->SetParent(this);
				mSprites.push_back(sp);
			}
		}
	}

	for (auto &&i : mFileIndices)
	{
		mImageList.AddToListIndex(i.first, Song->SongDirectory, i.second);
	}

	mImageList.LoadAll();
}

void osuBackgroundAnimation::Validate()
{
	for (auto &&i: mSprites)
	{
		auto sprite = std::make_shared<Sprite>();
		i->SetSprite(sprite);
		sprite->SetImage(mImageList.GetFromIndex(GetIndexFromFilename(i->GetImageFilename())), false);
		mDrawObjects.push_back(sprite);
	}
	mImageList.ForceFetch();
}

void osuBackgroundAnimation::SetAnimationTime(double Time)
{
	for (auto&& item: mSprites)
	{
		item->Update(Time);
	}
}

void osuBackgroundAnimation::Update(float Delta)
{
}

void osuBackgroundAnimation::Render()
{
	for (auto&& item: mDrawObjects)
	{
		item->Render();
	}
}

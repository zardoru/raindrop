#include <fstream>

#include "GameGlobal.h"
#include "BackgroundAnimation.h"
#include "Song.h"
#include "Song7K.h"
#include "ImageList.h"
#include "osuBackgroundAnimation.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include "Image.h"

// Must match PP_* ordering. Displacement to apply to a top-left origin quad with size 1 depending on pivot.
// X first, then Y.
struct pivot
{
	float x, y;
};
pivot OriginPivots[] = {
	{0.f, 0.f},
	{-0.5f, 0.f},
	{-1.f, 0.f},
	{0.f, -0.5f},
	{-0.5f, -0.5f},
	{-1.f, -0.5f},
	{0.f, -1.f},
	{-0.5f -1.f},
	{-1.f, -1.f},
};

Transformation pivotTransform[9];

void initializeTransforms()
{
	static bool inited = false;
	if (inited) return;

	for (int i = 0; i < 9; i++)
		pivotTransform->SetPosition(OriginPivots[i].x, OriginPivots[i].y);

	inited = true;
}

namespace osb {
	Event::Event(EEventType typ) :
		mEvtType(typ)
	{
		Time = 0; EndTime = 0;
	}

	EEventType Event::GetEventType()
	{
		return mEvtType;
	}

	float Event::GetTime()
	{
		return Time;
	}

	float Event::GetEndTime()
	{
		return EndTime;
	}

	float Event::GetDuration()
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

	float SingleValEvent::LerpValue(float At)
	{
		return Lerp(Value, EndValue, Clamp((At - Time) / GetDuration(), 0.f, 1.f));
	}

	float SingleValEvent::GetValue()
	{
		return Value;
	}

	void SingleValEvent::SetValue(float value)
	{
		Value = value;
	}

	float SingleValEvent::GetEndValue()
	{
		return EndValue;
	}

	void SingleValEvent::SetEndValue(float EndValue)
	{
		this->EndValue = EndValue;
	}

	Vec2 TwoValEvent::GetValue()
	{
		return Value;
	}

	Vec2 TwoValEvent::GetEndValue()
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

	Vec2 TwoValEvent::LerpValue(float At)
	{
		return Lerp(Value, EndValue, Clamp((At - Time) / GetDuration(), 0.f, 1.f));
	}

	Vec3 ColorizeEvent::GetValue()
	{
		return Value;
	}

	Vec3 ColorizeEvent::GetEndValue()
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

	BGASprite::BGASprite(GString file, EOrigin origin, Vec2 start_pos) : EventComponent(EVT_COUNT)
	{
		mFile = file;
		mOrigin = origin;
		mStartPos = start_pos;

		mParent = nullptr;
		mImageIndex = -1;
	}

	EventList::iterator BGASprite::GetEvent(float& Time, EEventType evt)
	{
		return lower_bound(mEventList[evt].begin(), mEventList[evt].end(), Time, 
			[](const osb::Event& A, const osb::Event &B) -> bool
		{ 
			return A.Time < B.Time; 
		});

	}

	bool BGASprite::IsValidEvent(EventList::iterator& evt_iter, EEventType evt)
	{
		return evt_iter != mEventList[evt].end() && evt_iter != mEventList[evt].begin();
	}

	template <class T>
	shared_ptr<T> event_cast(EventList::iterator& evt)
	{
		return static_pointer_cast<T>(*(evt - 1));
	}

	void BGASprite::Setup(float Time, Sprite& sprite)
	{
		auto T = &pivotTransform[mOrigin];

		if (!mParent) return; // We need this.
		if (mImageIndex == -1) // We haven't initialized from the parent's data yet? Alright.
		{
			assert(mParent != nullptr);
			mImageIndex = mParent->GetIndexFromFilename(mFile);
			mTransform.ChainTransformation(&mParent->GetTransformation());
		}

		sprite.ChainTransformation(T);
		T->ChainTransformation(&mTransform);

		// Now get the values for all the different stuff.
		// Set the image and reset size since we're using the pivot.
		// Also forces the matrix to be recalculated.
		sprite.SetImage(mParent->GetImageFromIndex(mImageIndex), false);
		sprite.SetSize(1, 1);

		if (sprite.GetImage())
		{
			auto i = sprite.GetImage();
			T->SetSize(i->w, i->h);
		}

		// Okay, a pretty long function follows. Fade first.
		auto fade_evt = GetEvent(Time, EVT_FADE);
		
		if (IsValidEvent(fade_evt, EVT_FADE))
			sprite.Alpha = event_cast<FadeEvent>(fade_evt)->LerpValue(Time);
		else sprite.Alpha = 0;

		// Now position.	
		auto movx_evt = GetEvent(Time, EVT_MOVEX);
		if (IsValidEvent(movx_evt, EVT_MOVEX))
			mTransform.SetPositionX(event_cast<MoveXEvent>(movx_evt)->LerpValue(Time));
		else mTransform.SetPositionX(mStartPos.x);

		auto movy_evt = GetEvent(Time, EVT_MOVEY);
		if (IsValidEvent(movy_evt, EVT_MOVEY))
			mTransform.SetPositionY(event_cast<MoveYEvent>(movy_evt)->LerpValue(Time));
		else mTransform.SetPositionY(mStartPos.y);

		// We already unpacked move events, so no need for this next snip.
		/* auto mov_evt = GetEvent(Time, EVT_MOVE);
		if (IsValidEvent(mov_evt, EVT_MOVE))
			mTransform.SetPosition(event_cast<MoveEvent>(mov_evt)->LerpValue(Time)); */

		// Now scale and rotation.
		auto scale_evt = GetEvent(Time, EVT_SCALE);
		if (IsValidEvent(scale_evt, EVT_SCALE))
			mTransform.SetScale(event_cast<ScaleEvent>(scale_evt)->LerpValue(Time));
		else mTransform.SetScale(1);

		// Since scale is just applied to size straight up, we can use this extra scale
		// defaulting at 1,1 to be our vector scale. That way they'll pile up.
		auto vscale_evt = GetEvent(Time, EVT_SCALEVEC);
		if (IsValidEvent(scale_evt, EVT_SCALEVEC))
			mTransform.SetSize(event_cast<VectorScaleEvent>(vscale_evt)->LerpValue(Time));
		else mTransform.SetSize(1, 1);

		auto rot_evt = GetEvent(Time, EVT_ROTATE);
		if (IsValidEvent(rot_evt, EVT_ROTATE))
			mTransform.SetRotation(glm::degrees(event_cast<RotateEvent>(rot_evt)->LerpValue(Time)));
		else mTransform.SetRotation(0);

		auto additive_evt = GetEvent(Time, EVT_ADDITIVE);
		if (IsValidEvent(additive_evt, EVT_ADDITIVE))
			sprite.SetBlendMode(BLEND_ADD);
		else sprite.SetBlendMode(BLEND_ALPHA);

		// Flip events are skipped for now.
	}

	GString BGASprite::GetImageFilename()
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

	void EventComponent::AddEvent(shared_ptr<Event> event)
	{
		if (event->GetEventType() != EVT_MOVE)
			mEventList[event->GetEventType()].push_back(event);
		else // Unpack move events.
		{
			auto mov = static_pointer_cast<MoveEvent>(event);
			auto mxe = make_shared<MoveXEvent>();
			auto mye = make_shared<MoveYEvent>();
			
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
	}

	float Loop::GetIterationDuration()
	{
		float dur = 0;
		for (auto k = 0; k < EVT_COUNT; k++)
			for (auto evt : mEventList[k])
				dur = std::max(evt->GetEndTime(), dur);

		return dur;
	}

	shared_ptr<osb::EventVector> Loop::Unroll()
	{
		auto ret = make_shared<osb::EventVector>();
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
					(*ret)[k].push_back(make_shared<Event>(new_evt));
				}

		return ret;
	}
}

osb::EOrigin OriginFromString(GString str)
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

shared_ptr<osb::Event> ParseEvent(vector<GString> split)
{
	auto ks = split[0];
	shared_ptr<osb::Event> evt;
	boost::algorithm::to_upper(ks);

	if (ks == "V"){
		auto xvt = make_shared<osb::VectorScaleEvent>();
		// xvt->SetValue()
		evt = xvt;
	}
	if (ks == "S") {
	}
	if (ks == "M") {
	}
	if (ks == "MX") {
	}
	if (ks == "MY") {
	}
	if (ks == "C") {
	}
	if (ks == "P") {
	}
	if (ks == "F") {
	}
	if (ks == "L") {
	}

	evt->SetTime(latof(split[2]));
	evt->SetEndTime(latof(split[3]));

	return evt;
}

shared_ptr<osb::SpriteList> ReadOSBEvents(std::istream& event_str)
{
	auto list = make_shared<osb::SpriteList>();
	int previous_lead = 100;
	shared_ptr<osb::BGASprite> sprite = nullptr;
	shared_ptr<osb::Loop> loop = nullptr;
	bool readingLoop = false;

	GString line;
	while (std::getline(event_str, line))
	{
		int lead_spaces;
		line = line.substr(line.find("//")); // strip comments
		lead_spaces = line.find_first_not_of("\t _");
		line = line.substr(lead_spaces, line.length() - lead_spaces + 1);

		vector<GString> split_result;
		boost::split(split_result, line, boost::is_any_of(","));
		for (auto &&s : split_result) boost::algorithm::to_lower(s);

		if (!line.length() || !split_result.size()) continue;

		if (lead_spaces < previous_lead && !readingLoop)
		{
			if (split_result[0] == "sprite")
			{
				Vec2 new_position(latof(split_result[3]), latof(split_result[4]));
				sprite = make_shared<osb::BGASprite>(split_result[1], OriginFromString(split_result[2]), new_position);
				list->push_back(sprite);
			}
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
					auto loop_events = loop->Unroll();
					for (auto i = 0; i < osb::EVT_COUNT; i++)
						for (auto evt : (*loop_events)[i])
							sprite->AddEvent(evt);
				}
				else
					loop->AddEvent(ParseEvent(split_result));
			}
			
			// It's not a command on the loop, or we weren't reading a loop in the first place.
			// Read a regular command.

			// Not "else" because we do want to execute this if we're no longer reading the loop.
			if (!readingLoop) {
				auto ev = ParseEvent(split_result);

				// A loop began - set that we are reading a loop and set this loop as where to add the following commands.
				if (ev->GetEventType() == osb::EVT_LOOP)
				{
					loop = static_pointer_cast<osb::Loop>(ev);
					readingLoop = true;
				}else // add this event, if not a loop to this sprite. It'll be unrolled once outside.
					sprite->AddEvent(ev);
			}
		}

		previous_lead = lead_spaces;
	}

	return list;
}

void osuBackgroundAnimation::AddImageToList(GString image_filename)
{
	if (mFileIndices.find(image_filename) == mFileIndices.end())
		mFileIndices[image_filename] = mFileIndices.size() + 1;
}

osuBackgroundAnimation::osuBackgroundAnimation(VSRG::Song* song, shared_ptr<osb::SpriteList> existing_sprites)
{
	initializeTransforms();
	for (auto sp : *existing_sprites)
	{
		mSprites.push_back(sp);
		AddImageToList(sp->GetImageFilename());
	}

	// Read the osb file from the song's directory.
	vector<GString> candidates;
	song->SongDirectory.ListDirectory(candidates, Directory::FS_REG, "osb");

	if (candidates.size())
	{
		GString head;
		std::fstream s(candidates.at(0), std::ios::in);

		if (std::getline(s, head) && head == "[Events]")
		{
			auto sprite_list = ReadOSBEvents(s);
			for (auto sp : *sprite_list)
			{
				AddImageToList(sp->GetImageFilename());
				mSprites.push_back(sp);
			}
		}
	}
}

Image* osuBackgroundAnimation::GetImageFromIndex(int m_image_index)
{
	return mImageList.GetFromIndex(m_image_index);
}

int osuBackgroundAnimation::GetIndexFromFilename(GString filename)
{
	return mFileIndices[filename];
}
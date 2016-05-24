#pragma once

#include "BackgroundAnimation.h"
#include "Transformation.h"
#include "ImageList.h"
#include "Easing.h"
#include "SceneEnvironment.h"

class osuBackgroundAnimation;

namespace VSRG
{
    class Song;
}

namespace osb
{
    enum EEventType
    {
        EVT_MOVEX,
        EVT_MOVEY,
        EVT_SCALE,
        EVT_SCALEVEC,
        EVT_ROTATE,
        EVT_COLORIZE,
        EVT_FADE,
        EVT_ADDITIVE,
		EVT_HFLIP,
		EVT_VFLIP,
        EVT_COUNT,
		// Non-events (to be unpacked)
        EVT_LOOP, // Unroll once finished loading into regular events.
        EVT_MOVE
    };

	enum EEase : int
	{
		EASE_NONE,
		EASE_OUT,
		EASE_IN,
		EASE_2IN,
		EASE_2OUT,
		EASE_2INOUT,
		EASE_3IN,
		EASE_3OUT,
		EASE_3INOUT,
		EASE_4IN,
		EASE_4OUT,
		EASE_4INOUT,
		EASE_5IN,
		EASE_5OUT,
		EASE_5INOUT,
		EASE_COUNT
		// And many others, tho nobody uses them lol.
	};


    class Event : public TimeBased<Event, float>
    {
        EEventType mEvtType;
		EEase mEase;
    protected:
        float EndTime;
        explicit Event(EEventType typ);
    public:
        EEventType GetEventType() const;
        float GetTime() const;
        float GetEndTime() const;
        float GetDuration() const;
		int GetEase() const;

        void SetTime(float time);
        void SetEndTime(float EndTime);
	    void SetEase(int val);
    };

	// We need to cast these - and to cast these we need to copy them.
	// Thus, we have to use shared_ptr.
    typedef std::vector<std::shared_ptr<Event> > EventList;
    typedef EventList EventVector[EVT_COUNT];

    class SingleValEvent : public Event
    {
    protected:
        float Value, EndValue;
        explicit SingleValEvent(EEventType typ) : 
			Event(typ), Value(0), EndValue(0)
        {};
    public:
        float GetValue() const;
        void SetValue(float value);

        float GetEndValue() const;
        void SetEndValue(float EndValue);
        float LerpValue(float Time) const;
    };

    class TwoValEvent : public Event
    {
        Vec2 Value, EndValue;
    protected:
        TwoValEvent(EEventType evt) : Event(evt) {};
    public:
        Vec2 GetValue() const;
        Vec2 GetEndValue() const;

        void SetValue(Vec2 val);
        void SetEndValue(Vec2 val);
        Vec2 LerpValue(float Time) const;
    };

    class MoveEvent : public TwoValEvent
    {
    public:
        MoveEvent() : TwoValEvent(EVT_MOVE) {};
    };

    class VectorScaleEvent : public TwoValEvent
    {
    public:
        VectorScaleEvent() : TwoValEvent(EVT_SCALEVEC) {};
    };

    class RotateEvent : public SingleValEvent
    {
    public:
        RotateEvent() : SingleValEvent(EVT_ROTATE) {};
    };

    class ColorizeEvent : public Event
    {
        Vec3 Value, EndValue;
    public:
        ColorizeEvent() : Event(EVT_COLORIZE) {};
        Vec3 GetValue() const;
        Vec3 GetEndValue() const;

        void SetValue(Vec3 val);
        void SetEndValue(Vec3 val);
		Vec3 LerpValue(float At) const;
    };

    class MoveXEvent : public SingleValEvent
    {
    public:
        MoveXEvent() : SingleValEvent(EVT_MOVEX) {};
    };

    class MoveYEvent : public SingleValEvent
    {
    public:
        MoveYEvent() : SingleValEvent(EVT_MOVEY) {};
    };

    class FadeEvent : public SingleValEvent
    {
    public:
        FadeEvent() : SingleValEvent(EVT_FADE) {};
    };

    class ScaleEvent : public SingleValEvent
    {
    public:
        ScaleEvent() : SingleValEvent(EVT_SCALE) {};
    };

    class FlipHorizontalEvent : public Event
    {
    public:
        FlipHorizontalEvent() : Event(EVT_HFLIP) {};
    };

    class FlipVerticalEvent : public Event
    {
    public:
        FlipVerticalEvent() : Event(EVT_VFLIP) {};
    };

    class AdditiveEvent : public Event
    {
    public:
        AdditiveEvent() : Event(EVT_ADDITIVE) {};
    };

	class HFlipEvent : public Event
	{
	public:
		HFlipEvent() : Event(EVT_HFLIP) {};
	};

	class VFlipEvent : public Event
	{
	public:
		VFlipEvent() : Event(EVT_VFLIP) {};
	};

    enum EOrigin
    {
        PP_TOPLEFT,
        PP_TOP,
        PP_TOPRIGHT,
        PP_LEFT,
        PP_CENTER,
        PP_RIGHT,
        PP_BOTTOMLEFT,
        PP_BOTTOM,
        PP_BOTTOMRIGHT
    };

	enum ELayer
	{
		LAYER_SP_BACKGROUND, // special background (0,0 event)
		LAYER_BACKGROUND,
		LAYER_FAIL,
		LAYER_PASS,
		LAYER_FOREGROUND
	};

    class EventComponent : public Event
    {
    protected:
		std::vector<MoveXEvent> evMoveX;
		std::vector<MoveYEvent> evMoveY;
		std::vector<ScaleEvent> evScale;
		std::vector<VectorScaleEvent> evScaleVec;
		std::vector<RotateEvent> evRotate;
		std::vector<ColorizeEvent> evColorize;
		std::vector<FadeEvent> evFade;
		std::vector<FlipHorizontalEvent> evFlipH;
		std::vector<FlipVerticalEvent> evFlipV;
		std::vector<AdditiveEvent> evAdditive;

		float StartPeriod, EndPeriod;
	    EventComponent(EEventType evt);
		friend class Loop;
    public:
        void AddEvent(std::shared_ptr<Event> evt);
		void ClearEvents();
        void SortEvents();
	    bool WithinEvents(float Time) const;
		float GetStartTime() const;
		float GetEndTime() const;
		void CopyEventsFrom(EventComponent &ec);
        float GetDuration();
    };

    class Loop : public EventComponent
    {
        int LoopCount;
        // EndTime is the last loop's time in here.
    public:
		Loop(int loop_count) : EventComponent(EVT_LOOP), LoopCount(loop_count) {}

        void Unroll(EventComponent* evt_list);
    };

    class BGASprite : public EventComponent
    {
        EOrigin mOrigin;
        std::string mFile;
        Vec2 mStartPos;
		Transformation mFlip;
        Transformation mTransform;
		Transformation mPivot;
        osuBackgroundAnimation *mParent;

		Sprite* mSprite;
        int mImageIndex;

		ELayer mLayer;
		bool mUninitialized;
    public:
        BGASprite(std::string file, EOrigin origin, Vec2 start_pos, ELayer laer);

		void SetSprite(Sprite* sprite);
	    void InitializeSprite();
	    void Update(float Time);
        std::string GetImageFilename() const;
		ELayer GetLayer() const;
        void SetParent(osuBackgroundAnimation* parent);
	    void SetImageIndex(int index);
    };

    typedef std::vector<osb::BGASprite> SpriteList;
}

class osuBackgroundAnimation : public BackgroundAnimation
{
    osb::SpriteList mSprites;
    std::vector<Sprite> mAutoBGLayer;
    std::vector<Sprite> mBackgroundLayer;
    std::vector<Sprite> mForegroundLayer;
    std::map<std::string, int> mFileIndices;
    ImageList mImageList;
    int AddImageToList(std::string image_filename);
	VSRG::Song *Song;

	Transformation mScreenTransformation;
	bool CanValidate;
public:
    osuBackgroundAnimation(Interruptible* parent, osb::SpriteList* existing_sprites, VSRG::Song* song);
    Image* GetImageFromIndex(int m_image_index);
    int GetIndexFromFilename(std::string filename);
	Transformation& GetScreenTransformation();

	void Load() override;
	void Validate() override;
	void Update(float Delta) override;
	void SetAnimationTime(double Time) override;
	void Render() override;
};

osb::SpriteList ReadOSBEvents(std::istream& event_str);
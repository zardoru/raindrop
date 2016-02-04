#pragma once

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
        EVT_FLIPH,
        EVT_FLIPV,
        EVT_ADDITIVE,
        EVT_LOOP, // Unroll once finished loading into regular events.
        EVT_MOVE,
        EVT_COUNT
    };

    class Event : public TimeBased<Event, float>
    {
        EEventType mEvtType;
    protected:
        float EndTime;
        Event(EEventType typ);
    public:
        EEventType GetEventType();
        float GetTime();
        float GetEndTime();
        float GetDuration();

        void SetTime(float time);
        void SetEndTime(float EndTime);
    };

    typedef vector<shared_ptr<Event> > EventList;
    typedef EventList EventVector[EVT_COUNT];

    class EventComponent : public Event
    {
    protected:
        EventVector mEventList;
        EventComponent(EEventType evt) : Event(evt) {};
    public:
        void AddEvent(shared_ptr<Event> evt);
        void SortEvents();
    };

    class Loop : public EventComponent
    {
        int LoopCount;
        // EndTime is the last loop's time in here.
    public:
        Loop() : EventComponent(EVT_LOOP) {}

        float GetIterationDuration();
        shared_ptr<osb::EventVector> Unroll();
    };

    class SingleValEvent : public Event
    {
    protected:
        float Value, EndValue;
        SingleValEvent(EEventType typ) : Event(typ) {};
    public:
        float GetValue();
        void SetValue(float value);

        float GetEndValue();
        void SetEndValue(float EndValue);
        float LerpValue(float Time);
    };

    class TwoValEvent : public Event
    {
        Vec2 Value, EndValue;
    protected:
        TwoValEvent(EEventType evt) : Event(evt) {};
    public:
        Vec2 GetValue();
        Vec2 GetEndValue();

        void SetValue(Vec2 val);
        void SetEndValue(Vec2 val);
        Vec2 LerpValue(float Time);
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
        Vec3 GetValue();
        Vec3 GetEndValue();

        void SetValue(Vec3 val);
        void SetEndValue(Vec3 val);
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
        FlipHorizontalEvent() : Event(EVT_FLIPH) {};
    };

    class FlipVerticalEvent : public Event
    {
    public:
        FlipVerticalEvent() : Event(EVT_FLIPV) {};
    };

    class AdditiveEvent : public Event
    {
    public:
        AdditiveEvent() : Event(EVT_ADDITIVE) {};
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

    class BGASprite : public EventComponent
    {
        EOrigin mOrigin;
        std::string mFile;
        Vec2 mStartPos;
        Transformation mTransform;
        osuBackgroundAnimation *mParent;

        int mImageIndex;
        EventList::iterator GetEvent(float& Time, EEventType evt);
        bool IsValidEvent(EventList::iterator& fade_evt, EEventType evt);
    public:
        BGASprite(std::string file, EOrigin origin, Vec2 start_pos);
        void Setup(float Time, Sprite& sprite);
        std::string GetImageFilename();

        void SetParent(osuBackgroundAnimation* parent);
    };

    typedef vector<shared_ptr<osb::BGASprite> >SpriteList;
}

class osuBackgroundAnimation : public BackgroundAnimation
{
    bool mIsWidescreen;
    vector<shared_ptr<osb::BGASprite>> mSprites;
    vector<shared_ptr<Sprite>> mDrawObjects;
    map<std::string, int> mFileIndices;
    ImageList mImageList;
    void AddImageToList(std::string image_filename);
    double AnimationTime;
public:
    osuBackgroundAnimation(VSRG::Song* song, shared_ptr<osb::SpriteList> existing_sprites);
    Image* GetImageFromIndex(int m_image_index);
    int GetIndexFromFilename(std::string filename);
};

shared_ptr<osb::SpriteList> ReadOSBEvents(std::istream& event_str);
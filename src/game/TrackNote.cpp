#include <game/Song.h>

int GetFractionKindBeat(double frac);

namespace rd {

    TrackNote::TrackNote() {
        Reset();
    }

    TrackNote::TrackNote(const NoteData &Data) {
        Reset();
        AssignNotedata(Data);
    }

    TrackNote::~TrackNote() {
    }

    void TrackNote::Reset() {
        EnabledHitFlags = EnabledFlag | HeadEnabledFlag;
    }

    float TrackNote::GetHoldSize() const {
        return std::abs((b_pos_holdend - b_pos));
    }

    float TrackNote::GetHoldEndVertical() {
        return b_pos_holdend;
    }

    void TrackNote::FailHit() {
        EnabledHitFlags |= FailedHitFlag;
    }

    void TrackNote::MakeInvisible() {
        EnabledHitFlags |= InvisibleFlag;
    }

    void TrackNote::RemoveSound() {
        Sound = 0;
    }

    bool TrackNote::FailedHit() const {
        return (EnabledHitFlags & FailedHitFlag) != 0;
    }

    void TrackNote::AssignNotedata(const rd::NoteData &Notedata) {
        Time = Notedata.StartTime;
        EndTime = Notedata.EndTime;
        Sound = Notedata.Sound;
        TailSound = Notedata.TailSound;
        NoteKind = Notedata.NoteKind;
    }


    /* calculate the beat snap for this fraction */
    void TrackNote::AssignFraction(double frac) {
        FractionKind = GetFractionKindBeat(frac);
    }

    void TrackNote::AssignPosition(double Position, double endPosition) {
        b_pos = Position;
        b_pos_holdend = endPosition;
    }

    bool TrackNote::IsHold() const {
        return EndTime != 0;
    }

    float TrackNote::GetVertical() const {
        return b_pos;
    }

    void TrackNote::AddTime(double Time) {
        this->Time += Time;

        if (IsHold())
            EndTime += Time;
    }

    double TrackNote::GetEndTime() const {
        return std::max(Time, EndTime);
    }

    double TrackNote::GetStartTime() const {
        return Time;
    }

    bool TrackNote::IsEnabled() const {
        return EnabledHitFlags & EnabledFlag;
    }

    bool TrackNote::IsHeadEnabled() const {
        return (EnabledHitFlags & HeadEnabledFlag) != 0;
    }

    void TrackNote::Disable() {
        EnabledHitFlags &= ~EnabledFlag;
        DisableHead();
    }

    void TrackNote::DisableHead() {
        EnabledHitFlags &= ~HeadEnabledFlag;
    }

    void TrackNote::Hit() {
        EnabledHitFlags |= WasHitFlag;
    }

    bool TrackNote::WasHit() const {
        return (EnabledHitFlags & WasHitFlag) != 0;
    }

    float TrackNote::GetVerticalHold() const {
        return b_pos + GetHoldSize() / 2;
    }

    uint32_t TrackNote::GetSound() const {
        return Sound;
    }

    uint32_t TrackNote::GetTailSound() const {
        return TailSound;
    }

    int TrackNote::GetFracKind() const {
        return FractionKind;
    }

    double &TrackNote::GetDataStartTime() {
        return Time;
    }

    double &TrackNote::GetDataEndTime() {
        return EndTime;
    }

    uint32_t &TrackNote::GetDataSound() {
        return Sound;
    }

    uint8_t TrackNote::GetDataNoteKind() const {
        return NoteKind;
    }

    uint8_t TrackNote::GetDataFractionKind() {
        return FractionKind;
    }

    bool TrackNote::IsJudgable() const {
        return NoteKind != NK_INVISIBLE && NoteKind != NK_FAKE;
    }

    bool TrackNote::IsVisible() const {
        return NoteKind != NK_INVISIBLE && !(EnabledHitFlags & InvisibleFlag);
    }
}
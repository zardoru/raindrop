#ifndef SG7KMECHS_H_
#define SG7KMECHS_H_

class ScoreKeeper7K;

enum TimingType
{
	TT_TIME,
	TT_BEATS,
	TT_PIXELS
};

class VSRGMechanics
{
public:
	typedef function<void(double, uint32, bool, bool)> HitEvent;
	typedef function<void(double, uint32, bool, bool, bool)> MissEvent;
	typedef function<void(uint32)> KeysoundEvent;

protected:
	VSRG::Song *CurrentSong;
	VSRG::Difficulty *CurrentDifficulty;
	ScoreKeeper7K *score_keeper;

public:

	// These HAVE to be set before anything else is called.
	function <bool(uint32)> IsLaneKeyDown;
	function <void(uint32, bool)> SetLaneHoldingState;
	KeysoundEvent PlayLaneSoundEvent, PlayNoteSoundEvent;
	HitEvent HitNotify;
	MissEvent MissNotify;

	virtual void Setup(VSRG::Song *Song, VSRG::Difficulty *Difficulty, ScoreKeeper7K *scoreKeeper);

	// If returns true, don't judge any more notes.
	virtual bool OnUpdate(double SongTime, VSRG::TrackNote* Note, uint32 Lane) = 0;

	// If returns true, don't judge any more notes.
	virtual bool OnPressLane(double SongTime, VSRG::TrackNote* Note, uint32 Lane) = 0;

	// If returns true, don't judge any more notes either.
	virtual bool OnReleaseLane(double SongTime, VSRG::TrackNote* Note, uint32 Lane) = 0;

	virtual TimingType GetTimingKind() = 0;
};

class RaindropMechanics : public VSRGMechanics
{
	bool forcedRelease;
public:
	RaindropMechanics(bool forcedRelease);
	bool OnUpdate(double SongTime, VSRG::TrackNote* Note, uint32 Lane);
	bool OnPressLane(double SongTime, VSRG::TrackNote* Note, uint32 Lane);
	bool OnReleaseLane(double SongTime, VSRG::TrackNote* Note, uint32 Lane);

	TimingType GetTimingKind();
};

class O2JamMechanics : public VSRGMechanics
{
public:

	bool OnUpdate(double SongBeat, VSRG::TrackNote* Note, uint32 Lane);
	bool OnPressLane(double SongBeat, VSRG::TrackNote* Note, uint32 Lane);
	bool OnReleaseLane(double SongBeat, VSRG::TrackNote* Note, uint32 Lane);

	TimingType GetTimingKind();
};

#endif
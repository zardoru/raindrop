#pragma once

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
	typedef function<void(VSRG::TrackNote*)> NoteSoundEvent;

protected:

	VSRG::Song *CurrentSong;
	VSRG::Difficulty *CurrentDifficulty;
	shared_ptr<ScoreKeeper7K> score_keeper;

public:

	virtual ~VSRGMechanics() = default;

	// These HAVE to be set before anything else is called.
	function <bool(uint32)> IsLaneKeyDown;
	function <void(uint32, bool)> SetLaneHoldingState;
	KeysoundEvent PlayLaneSoundEvent;
	NoteSoundEvent PlayNoteSoundEvent;
	HitEvent HitNotify;
	MissEvent MissNotify;

	virtual void Setup(VSRG::Song *Song, VSRG::Difficulty *Difficulty, shared_ptr<ScoreKeeper7K> scoreKeeper);

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
	bool OnUpdate(double SongTime, VSRG::TrackNote* Note, uint32 Lane) override;
	bool OnPressLane(double SongTime, VSRG::TrackNote* Note, uint32 Lane) override;
	bool OnReleaseLane(double SongTime, VSRG::TrackNote* Note, uint32 Lane) override;

	TimingType GetTimingKind() override;
};

class O2JamMechanics : public VSRGMechanics
{
public:

	bool OnUpdate(double SongBeat, VSRG::TrackNote* Note, uint32 Lane) override;
	bool OnPressLane(double SongBeat, VSRG::TrackNote* Note, uint32 Lane) override;
	bool OnReleaseLane(double SongBeat, VSRG::TrackNote* Note, uint32 Lane) override;

	TimingType GetTimingKind() override;
};
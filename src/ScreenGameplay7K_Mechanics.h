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
	typedef std::function<void(double, uint32_t, bool, bool)> HitEvent;
	typedef std::function<void(double, uint32_t, bool, bool, bool)> MissEvent;
	typedef std::function<void(uint32_t)> KeysoundEvent;
	typedef std::function<void(VSRG::TrackNote*)> NoteSoundEvent;

protected:

	VSRG::Song *CurrentSong;
	VSRG::Difficulty *CurrentDifficulty;
    std::shared_ptr<ScoreKeeper7K> score_keeper;

public:

	virtual ~VSRGMechanics() = default;

	// These HAVE to be set before anything else is called.
	std::function <bool(uint32_t)> IsLaneKeyDown;
	std::function <void(uint32_t, bool)> SetLaneHoldingState;
	KeysoundEvent PlayLaneSoundEvent;
	NoteSoundEvent PlayNoteSoundEvent;
	HitEvent HitNotify;
	MissEvent MissNotify;

	virtual void Setup(VSRG::Song *Song, VSRG::Difficulty *Difficulty, std::shared_ptr<ScoreKeeper7K> scoreKeeper);

	// If returns true, don't judge any more notes.
	virtual bool OnUpdate(double SongTime, VSRG::TrackNote* Note, uint32_t Lane) = 0;

	// If returns true, don't judge any more notes.
	virtual bool OnPressLane(double SongTime, VSRG::TrackNote* Note, uint32_t Lane) = 0;

	// If returns true, don't judge any more notes either.
	virtual bool OnReleaseLane(double SongTime, VSRG::TrackNote* Note, uint32_t Lane) = 0;

	virtual TimingType GetTimingKind() = 0;
};

class RaindropMechanics : public VSRGMechanics
{
	bool forcedRelease;
public:
	RaindropMechanics(bool forcedRelease);
	bool OnUpdate(double SongTime, VSRG::TrackNote* Note, uint32_t Lane) override;
	bool OnPressLane(double SongTime, VSRG::TrackNote* Note, uint32_t Lane) override;
	bool OnReleaseLane(double SongTime, VSRG::TrackNote* Note, uint32_t Lane) override;

	TimingType GetTimingKind() override;
};

class O2JamMechanics : public VSRGMechanics
{
public:

	bool OnUpdate(double SongBeat, VSRG::TrackNote* Note, uint32_t Lane) override;
	bool OnPressLane(double SongBeat, VSRG::TrackNote* Note, uint32_t Lane) override;
	bool OnReleaseLane(double SongBeat, VSRG::TrackNote* Note, uint32_t Lane) override;

	TimingType GetTimingKind() override;
};
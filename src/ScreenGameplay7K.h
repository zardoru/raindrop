#pragma once

#include "Audio.h"
#include "Song7K.h"
#include "BackgroundAnimation.h"
#include "AudioSourceOJM.h"

class AudioStream;
class Texture;
class SceneEnvironment;
class LuaManager;

#include "PlayerContext.h"

namespace Game {
	namespace VSRG {
		class ScreenGameplay : public Screen
		{
		private:


			std::map <int, std::vector<std::shared_ptr<AudioSample> > > Keysounds;
			std::vector<std::unique_ptr<PlayerContext>> Players;
			std::queue<AutoplaySound>   BGMEvents;

			std::shared_ptr<Song>			 MySong;
			std::shared_ptr<Song>			 LoadedSong;

			struct {
				double InterpolatedStream, Stream;
				double OldStream; // Previous frame's stream song time
				double Waiting; // Time before T = 0 
				double Game; // Overall screen time
				double Miss; // Time for showing MISS layer
				double Failure; // Time for showing Failure state
				double Success; // Time for showing Success state
				double AudioStart, AudioOld; // DAC thread start time and previous DAC time
				bool InterpolateStream;
			} Time;

			struct {
				double ToleranceMS;
				double AudioDrift;
			} TimeError;


			int				 StartMeasure;

			std::unique_ptr<AudioStream> Music;
			std::unique_ptr<AudioSourceOJM> OJMAudio;
			AudioSample MissSnd;
			AudioSample FailSnd;

			/* Effects */
			bool StageFailureTriggered;
			bool Active;
			bool ForceActivation;
			bool DoPlay;
			bool PlayReactiveSounds;
			bool SongFinishTriggered;

			std::unique_ptr<BackgroundAnimation> BGA;
			double JudgeOffset;
			void SetupScriptConstants();
			
			// Done in loading thread
			bool LoadChartData();
			bool LoadSongAudio();
			void LoadSamples();
			void LoadBmson();
			bool LoadBGA() const;
			bool ProcessSong();


			void AssignMeasure(uint32_t Measure);
			void RunAutoEvents();
			void CheckShouldEndScreen();
			bool ShouldDelayFailure();
			bool PlayersHaveFailed();
			bool SongHasFinished();

			void UpdateSongTime(float Delta);
			void Render();

			void PlayKeysound(int Keysound);

			void Activate();


			void OnPlayerMiss(double dt, uint32_t lane, bool hold, bool dontbreakcombo, bool earlymiss, int pn);

			friend class Noteskin;
		public:

			void SetupLua(LuaManager* Env);

			// Functions for data.
			bool IsActive() const;
			Game::VSRG::Song* GetSong() const;


			ScreenGameplay();
			void Init(std::shared_ptr<Song> S);
			void LoadResources() override;
			void InitializeResources() override;

			void Cleanup() override;

			PlayerContext* GetPlayerContext(int i);

			bool Run(double Delta) override;
			bool HandleInput(int32_t key, KeyEventType code, bool isMouseInput) override;
		};

	}
}
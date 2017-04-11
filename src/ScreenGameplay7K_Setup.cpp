#include "pch.h"

#include "GameGlobal.h"
#include "GameState.h"
#include "Logging.h"
#include "SongLoader.h"
#include "Screen.h"
#include "Audio.h"
#include "GameWindow.h"
#include "ImageList.h"

#include "SceneEnvironment.h"

#include "ScoreKeeper7K.h"
#include "TrackNote.h"
#include "ScreenGameplay7K.h"

#include "AudioSourceOJM.h"
#include "BackgroundAnimation.h"
#include "Noteskin.h"
#include "Line.h"

#include "NoteTransformations.h"

CfgVar DisableBGA("DisableBGA");

namespace Game {
	namespace VSRG {
		ScreenGameplay::ScreenGameplay() : Screen("ScreenGameplay7K")
		{
			Time = {};
			Time.OldStream = -1;
			Music = nullptr;

			StageFailureTriggered = false;
			SongFinishTriggered = false;

			Time.InterpolateStream = (Configuration::GetConfigf("InterpolateTime") != 0);

			LoadedSong = nullptr;
			Active = false;

			StartMeasure = -1;

			// Don't play unless everything goes right (later checks)
			DoPlay = false;
		}

		void ScreenGameplay::Cleanup()
		{
			if (Music)
				Music->Stop();
		}

		void ScreenGameplay::AssignMeasure(uint32_t Measure)
		{
			double mt = Players[0]->GetMeasureTime(Measure);
			double wt = Players[0]->GetPlayerState().GetWarpedSongTime(mt);
			for (auto& player : Players) {
				player->SetUnwarpedTime(mt);
			}

			// We use P1's BGM events in every case.
			// Remove non-played objects
			while (BGMEvents.size() && BGMEvents.front() <= wt)
				BGMEvents.pop();

			Time.Stream = Time.InterpolatedStream = mt;

			if (Music)
			{
				Log::Printf("ScreenGameplay7K: Setting player to time %f.\n", mt);
				Time.OldStream = -1;
				Music->SeekTime(Time.Stream);
			}

			Active = true;
		}

		void ScreenGameplay::Init(std::shared_ptr<VSRG::Song> S)
		{
			MySong = S;
			ForceActivation = false;

			for (auto i = 0; i < GameState::GetInstance().GetPlayerCount(); i++) {
				Players.push_back(std::make_unique<PlayerContext>(i, *GameState::GetInstance().GetParameters(i)));
				Players[i]->PlayKeysound = std::bind(&ScreenGameplay::PlayKeysound, this, std::placeholders::_1);
				Players[i]->SetSceneEnvironment(Animations);
			}
		}

		bool ScreenGameplay::LoadChartData()
		{
			uint8_t index = 0;
			// The song is the same for _everyone_, so...
			bool preloaded = true;
			for (auto &d : MySong->Difficulties) {
				if (!d->Data) preloaded = false;
			}

			if (!preloaded)
			{
				// The difficulty details are destroyed; which means we should load this from its original file.
				SongLoader Loader(GameState::GetInstance().GetSongDatabase());
				std::filesystem::path FN;

				Log::Printf("Loading Chart...");
				LoadedSong = Loader.LoadFromMeta(MySong.get(), GameState::GetInstance().GetDifficultyShared(0), FN, index);

				if (LoadedSong == nullptr)
				{
					Log::Printf("Failure to load chart. (Filename: %s)\n", Utility::ToU8(FN.wstring()).c_str());
					return false;
				}

				MySong = LoadedSong;

				/*
					At this point, MySong == LoadedSong, which means it's not a metadata-only Song* Instance.
					The old copy is preserved; but this new one (LoadedSong) will be removed by the end of ScreenGameplay7K.
				*/
			}



			BGA = BackgroundAnimation::CreateBGAFromSong(index, *MySong, this);

			return true;
		}

		bool ScreenGameplay::LoadSongAudio()
		{
			CfgVar SkipLoadAudio("SkipAudioLoad", "Debug");
			if (SkipLoadAudio) return true;

			auto Rate = GameState::GetInstance().GetParameters(0)->Rate;
			if (!Music)
			{
				Music = std::make_unique<AudioStream>();
				Music->SetPitch(Rate);
				if (std::filesystem::exists(MySong->SongFilename)
					&& Music->Open(MySong->SongDirectory / MySong->SongFilename))
				{
					Log::Printf("Stream for %s succesfully opened.\n", MySong->SongFilename.c_str());
				}
				else
				{
					if (!Players[0]->GetPlayerState().IsVirtual())
					{
						// Caveat: Try to autodetect an mp3/ogg file.
						auto SngDir = MySong->SongDirectory;

						// Open the first MP3 and OGG file in the directory
						for (auto i : std::filesystem::directory_iterator(SngDir))
						{
							auto extension = i.path().extension();
							if (extension == ".mp3" || extension == ".ogg")
								if (Music->Open(i.path()))
									return true;
						}

						// Quit; couldn't find audio for a chart that requires it.
						Music = nullptr;
						Log::Printf("Unable to load song (Path: %s)\n", MySong->SongFilename.c_str());
						return false;
					}
				}
			}

			auto &ps = Players[0]->GetPlayerState();
			auto SoundList = ps.GetSoundList();
			auto ChartType = ps.GetChartType();

			// Load samples.
			if (MySong->SongFilename.extension() == ".ojm")
			{
				Log::Printf("O2JAM: Loading OJM.\n");
				OJMAudio = std::make_unique<AudioSourceOJM>(this);
				OJMAudio->SetPitch(Rate);
				OJMAudio->Open(MySong->SongDirectory / MySong->SongFilename);

				for (int i = 1; i <= 2000; i++)
				{
					std::shared_ptr<AudioSample> Snd = OJMAudio->GetFromIndex(i);

					if (Snd != nullptr)
						Keysounds[i].push_back(Snd);
				}
			}
			else if (SoundList.size())
			{
				Log::Printf("Chart Audio: Loading samples... ");
				LoadSamples();

			}
			else if (ps.IsBmson()) {
				Log::Printf("BMSON: Loading Slice data...\n");
				LoadBmson();
			}


			return true;
		}

		void ScreenGameplay::LoadSamples() {
			auto Rate = GameState::GetInstance().GetParameters(0)->Rate;
			auto &ps = Players[0]->GetPlayerState();
			auto SoundList = ps.GetSoundList();

			auto start = std::chrono::high_resolution_clock::now();
			for (auto i = SoundList.begin(); i != SoundList.end(); ++i)
			{
				auto ks = std::make_shared<AudioSample>();

				ks->SetPitch(Rate);
#ifdef WIN32
				std::filesystem::path rfd = i->second;
				std::filesystem::path afd = MySong->SongDirectory / rfd;
				ks->Open(afd, true);
#else
				ks->Open((MySong->SongDirectory.string() + "/" + i->second).c_str(), true);
#endif
				Keysounds[i->first].push_back(ks);
				CheckInterruption();
			}

			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count();
			Log::LogPrintf("Keysounds loading in the background. Taken %I64dms to finish.", dur);
		}

		void ScreenGameplay::LoadBmson() {
			auto Rate = GameState::GetInstance().GetParameters(0)->Rate;
			auto &ps = Players[0]->GetPlayerState();
			auto dir = MySong->SongDirectory;
			std::map<int, AudioSample> audio;
			std::mutex audio_data_mutex;
			std::mutex keysound_data_mutex;
			auto &slicedata = ps.GetSliceData();

			// do bmson loading - threaded slicing!
			std::vector<std::future<void>> threads;
			std::atomic<int> obj_cnt = 0;

			auto load_start_time = std::chrono::high_resolution_clock::now();
			for (auto audiofile : slicedata.AudioFiles) {
				auto fn = [&](const std::pair<int, std::string> audiofile) {
					auto path = (dir / audiofile.second);
					AudioSample* p;

					// Audio load (parallelly?)
					audio_data_mutex.lock();
					p = &audio[audiofile.first];
					audio_data_mutex.unlock();

					p->SetPitch(Rate);

					// Verbose, but not as verbose as other languages.

					Log::LogPrintf("BMSON: Load sound %s AUDIO ID: %d\n", Utility::ToU8(path.wstring()).c_str(), audiofile.first);
					auto t = std::chrono::high_resolution_clock::now();

					// Open file
					if (!p->Open(path))
						throw std::runtime_error(Utility::Format("Unable to load %s.", audiofile.second.c_str()));


					// Done. Slicing
					auto d = std::chrono::high_resolution_clock::now() - t;
					auto cd = std::chrono::duration_cast<std::chrono::milliseconds>(d);

					Log::LogPrintf("BMSON: Slicing %d. Read in %I64dms...\n", audiofile.first, cd.count());
					auto t2 = std::chrono::high_resolution_clock::now();
					// Slice file
					// For each wav/sound index on the list
					for (auto wav : slicedata.Slices) {
						// for each slice on this index (mix-note)
						for (auto sound : wav.second) {
							// This is a slice of our available big boy.
							if (sound.first == audiofile.first) {
								p->Slice(sound.second.Start, sound.second.End);
								keysound_data_mutex.lock();
								Keysounds[wav.first].push_back(p->CopySlice());
								keysound_data_mutex.unlock();
							}
							// obj_cnt++;
						}
					}

					auto d2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - t2);
					Log::LogPrintf("BMSON: Sliced %d in %I64dms...\n", audiofile.first, d2.count());
				};

				threads.push_back(std::async(std::launch::async, fn, audiofile));
			}

			bool go_on = true;
			while (go_on) {
				bool one_thread_is_not_finished = false;
				for (auto &thread : threads) {
					if (thread.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
						one_thread_is_not_finished = true;
					}
				}

				go_on = one_thread_is_not_finished;
			}

			// Get rid of that extra space
			for (auto &ks : Keysounds) {
				ks.second.shrink_to_fit();
			}

			auto load_dur = std::chrono::high_resolution_clock::now() - load_start_time;
			auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(load_dur).count();
			Log::LogPrintf("BMSON: Loaded slices in %I64d\n", dur);
			//Log::Printf("BMSON: Generated %d sound objects.\n", wavs);
		}

		bool ScreenGameplay::ProcessSong()
		{
			TimeError.AudioDrift = 0;

			double DesiredDefaultSpeed = Configuration::GetSkinConfigf("DefaultSpeedUnits");

			Game::VSRG::ESpeedType Type = (Game::VSRG::ESpeedType)(int)Configuration::GetSkinConfigf("DefaultSpeedKind");
			double SpeedConstant = 0; // Unless set, assume we're using speed changes

			int ApplyDriftVirtual = Configuration::GetConfigf("UseAudioCompensationKeysounds");
			int ApplyDriftDecoder = Configuration::GetConfigf("UseAudioCompensationNonKeysounded");

			auto diff = GameState::GetInstance().GetDifficulty(0);

			if (Time.InterpolateStream &&  // Apply drift is enabled and:
				((ApplyDriftVirtual && diff->IsVirtual) ||  // We want to apply it to a keysounded file and it's virtual
					(ApplyDriftDecoder && diff->IsVirtual))) // or we want to apply it to a non-keysounded file and it's not virtual
				TimeError.AudioDrift += MixerGetLatency();

			TimeError.AudioDrift += Configuration::GetConfigf("Offset7K");

			if (diff->IsVirtual)
				TimeError.AudioDrift += Configuration::GetConfigf("OffsetKeysounded");
			else
				TimeError.AudioDrift += Configuration::GetConfigf("OffsetNonKeysounded");

			JudgeOffset = Configuration::GetConfigf("JudgeOffsetMS") / 1000;

			Log::Logf("TimeCompensation: %f (Latency: %f / Offset: %f)\n", TimeError.AudioDrift, MixerGetLatency(), diff->Offset);

			Log::Printf("Processing song... ");

			for (auto &&p : Players) {
				for (auto sd : MySong->Difficulties)
					if (sd->ID == GameState::GetInstance().GetDifficulty(p->GetPlayerNumber())->ID) {
						p->SetPlayableData(sd, TimeError.AudioDrift, DesiredDefaultSpeed, Type);
						p->Init();

						// there's a better way but not right now
						GameState::GetInstance().SetCurrentGaugeType(p->GetCurrentGaugeType(), p->GetPlayerNumber());
						GameState::GetInstance().SetCurrentScoreType(p->GetCurrentScoreType(), p->GetPlayerNumber());
						GameState::GetInstance().SetCurrentSystemType(p->GetCurrentSystemType(), p->GetPlayerNumber());

						if (!p->GetPlayerState().HasTimingData())
						{
							Log::Printf("Error loading chart: No timing data for player %d.\n", p->GetPlayerNumber());
							return false;
						}
					}
			}

			auto bgm0 = Players[0]->GetBgmData();

			sort(bgm0.begin(), bgm0.end());
			for (auto &s : bgm0)
				BGMEvents.push(s);

			for (auto &&p : Players) {
				Time.Waiting = std::max(Time.Waiting, p->GetWaitingTime());
			}

			return true;
		}

		bool ScreenGameplay::LoadBGA() const
		{
			if (!DisableBGA)
			{
				try {
					BGA->Load();
				}
				catch (std::exception &e)
				{
					Log::LogPrintf("Failure to load BGA: %s.\n", e.what());
				}
			}

			return true;
		}

		void ScreenGameplay::LoadResources()
		{
			auto MissSndFile = Configuration::GetSkinSound("Miss");
			auto FailSndFile = Configuration::GetSkinSound("Fail");

			MissSnd.Open(MissSndFile);
			FailSnd.Open(FailSndFile);

			if (!LoadChartData() || !ProcessSong() || !LoadSongAudio() || !LoadBGA())
			{
				DoPlay = false;
				return;
			}

			SetupLua(Animations->GetEnv());
			TimeError.ToleranceMS = CfgVar("ErrorTolerance");

			if (TimeError.ToleranceMS <= 0)
				TimeError.ToleranceMS = 16; // ms

			SetupScriptConstants();

			Animations->Preload(GameState::GetInstance().GetSkinFile("screengameplay7k.lua"), "Preload");
			Log::Printf("Done.\n");

			if (StartMeasure > 0)
				AssignMeasure(StartMeasure);

			ForceActivation = ForceActivation || (Configuration::GetSkinConfigf("InmediateActivation") == 1);
			

			// We're done with the data stored in the difficulties that aren't the one we're using. Clear it up.
			for (auto i = MySong->Difficulties.begin(); i != MySong->Difficulties.end(); ++i)
				(*i)->Destroy();

			CfgVar await("AwaitKeysoundLoad");
			if (await) {
				auto st = std::chrono::high_resolution_clock::now();
				Log::LogPrintf("Awaiting for keysounds to finish loading...\n");
				for (auto &vec : Keysounds) {
					for (auto &snd : vec.second) {
						snd->AwaitLoad();
					}
				}
			
				auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - st).count();
				Log::LogPrintf("Done. Taken %I64dms to finish.\n", dur);
			}

			DoPlay = true;
		}

		void ScreenGameplay::InitializeResources()
		{

			if (!DoPlay) // Failure to load something important?
			{
				Running = false;
				return;
			}

			PlayReactiveSounds = (!(Configuration::GetConfigf("DisableHitsounds")));

			Animations->GetImageList()->ForceFetch();
			BGA->Validate();

			for (auto& p : Players) {
				p->Validate();
			}


			Animations->Initialize("", false);
			Running = true;
		}
		}
	}
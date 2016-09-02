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
			double mt = Players[0].GetMeasureTime(Measure);
			double wt = Players[0].GetPlayerState().GetWarpedSongTime(mt);
			for (auto& player : Players) {
				player.SetUnwarpedTime(mt);
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

			for (auto i = 0; i < GameState::GetInstance().GetPlayerCount(); i++)
				Players.push_back(std::move(PlayerContext(i)));
		}

		bool ScreenGameplay::LoadChartData()
		{
			uint8_t index = 0;
			// The song is the same for _everyone_, so...

			if (!Preloaded)
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
					if (!Players[0].GetPlayerState().ConnectedDifficulty->IsVirtual)
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

			auto &ps = Players[0].GetPlayerState();
			auto SoundList = ps.GetSoundList();
			auto ChartType = ps.GetChartType();
			// Load samples.
			if (MySong->SongFilename.extension() == ".ojm")
			{
				Log::Printf("Loading OJM.\n");
				OJMAudio = std::make_unique<AudioSourceOJM>(this);
				OJMAudio->SetPitch(Rate);
				OJMAudio->Open(MySong->SongDirectory / MySong->SongFilename);

				for (int i = 1; i <= 2000; i++)
				{
					std::shared_ptr<SoundSample> Snd = OJMAudio->GetFromIndex(i);

					if (Snd != nullptr)
						Keysounds[i].push_back(Snd);
				}
			}
			else if (SoundList.size())
			{
				Log::Printf("Loading samples... ");

				if (ps.IsBmson())
				{
					auto dir = MySong->SongDirectory;
					int wavs = 0;
					std::map<int, SoundSample> audio;
					auto &slicedata = ps.GetSliceData();
					// do bmson loading
					for (auto wav : slicedata.Slices)
					{
						for (auto sounds : wav.second)
						{
							CheckInterruption();
							// load basic sound
							if (!audio[sounds.first].IsValid())
							{
								auto path = (dir / slicedata.AudioFiles[sounds.first]);

								audio[sounds.first].SetPitch(Rate);

								if (!audio[sounds.first].Open(path))
									throw std::runtime_error(Utility::Format("Unable to load %s.", slicedata.AudioFiles[sounds.first]).c_str());
								Log::Printf("BMSON: Load sound %s\n", Utility::ToU8(path.wstring()).c_str());
							}

							audio[sounds.first].Slice(sounds.second.Start, sounds.second.End);
							Keysounds[wav.first].push_back(audio[sounds.first].CopySlice());
							wavs++;
						}

						Keysounds[wav.first].shrink_to_fit();
					}

					Log::Printf("BMSON: Generated %d sound objects.\n", wavs);
				}

				for (auto i = SoundList.begin(); i != SoundList.end(); ++i)
				{
					auto ks = std::make_shared<SoundSample>();

					ks->SetPitch(Rate);
#ifdef WIN32
					std::filesystem::path rfd = i->second;
					std::filesystem::path afd = MySong->SongDirectory / rfd;
					ks->Open(afd);
#else
					ks->Open((MySong->SongDirectory.string() + "/" + i->second).c_str());
#endif
					Keysounds[i->first].push_back(ks);
					CheckInterruption();
				}
			}

			return true;
		}

		bool ScreenGameplay::ProcessSong()
		{
			TimeError.AudioDrift = 0;

			double DesiredDefaultSpeed = Configuration::GetSkinConfigf("DefaultSpeedUnits");

			Game::VSRG::ESpeedType Type = (Game::VSRG::ESpeedType)(int)Configuration::GetSkinConfigf("DefaultSpeedKind");
			double SpeedConstant = 0; // Unless set, assume we're using speed changes

			int ApplyDriftVirtual = Configuration::GetConfigf("UseAudioCompensationKeysounds");
			int ApplyDriftDecoder = Configuration::GetConfigf("UseAudioCompensationNonKeysounded");

			auto &ps = Players[0].GetPlayerState();

			if (Time.InterpolateStream &&  // Apply drift is enabled and:
				((ApplyDriftVirtual && ps.IsVirtual()) ||  // We want to apply it to a keysounded file and it's virtual
					(ApplyDriftDecoder && !ps.IsVirtual()))) // or we want to apply it to a non-keysounded file and it's not virtual
				TimeError.AudioDrift += MixerGetLatency();

			TimeError.AudioDrift += Configuration::GetConfigf("Offset7K");

			if (ps.IsVirtual())
				TimeError.AudioDrift += Configuration::GetConfigf("OffsetKeysounded");
			else
				TimeError.AudioDrift += Configuration::GetConfigf("OffsetNonKeysounded");

			JudgeOffset = Configuration::GetConfigf("JudgeOffsetMS") / 1000;

			Log::Logf("TimeCompensation: %f (Latency: %f / Offset: %f)\n", TimeError.AudioDrift, MixerGetLatency(), ps.GetOffset());

			// What, you mean we don't have timing data at all?
			if (ps.HasTimingData())
			{
				Log::Printf("Error loading chart: No timing data.\n");
				return false;
			}

			Log::Printf("Processing song... ");

			auto bgm0 = Players[0].GetBgmData();

			sort(bgm0.begin(), bgm0.end());
			for (auto &s : bgm0)
				BGMEvents.push(s);

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

			if (!LoadChartData() || !LoadSongAudio() || !ProcessSong() || !LoadBGA())
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

			AssignMeasure(StartMeasure);

			ForceActivation = ForceActivation || (Configuration::GetSkinConfigf("InmediateActivation") == 1);

			// We're done with the data stored in the difficulties that aren't the one we're using. Clear it up.
			for (auto i = MySong->Difficulties.begin(); i != MySong->Difficulties.end(); ++i)
				(*i)->Destroy();

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

			Animations->Initialize("", false);
			Running = true;
		}
	}
}
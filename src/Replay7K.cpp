#include "pch.h"

#include "Replay7K.h"

namespace Game {
	namespace VSRG {
		Replay::Replay()
		{

		}

		Replay::~Replay()
		{

		
		}
		void Replay::SetSongData(
			PlayscreenParameters params, 
			ESpeedType speedType, 
			std::string sha256hash, 
			uint32_t diffindex)
		{
			UserParameters = params;
			SongHash = sha256hash;
			DiffIndex = diffindex;
			SpeedType = speedType;
		}

		PlayscreenParameters Replay::GetEffectiveParameters()
		{
			return UserParameters;
		}

		std::string Replay::GetSongHash()
		{
			return SongHash;
		}

		uint32_t Replay::GetDifficultyIndex()
		{
			return DiffIndex;
		}

		bool Replay::IsLoaded()
		{
			return EventPlaybackQueue.size() > 0;
		}

		void Replay::AddEvent(Entry entry)
		{
			ReplayData.push_back(entry);
		}

		bool Replay::Load(std::filesystem::path input)
		{
			Json::Value root;
			CreateIfstream(in, input);
			in >> root;

			size_t count = root["replayEvents"].size();
			std::vector <Entry> events;

			// copy potentially unsorted events
			for (auto i = 0; i < count; i++) {
				auto jsonentry = root["replayEvents"][i];
				events.push_back(Entry{
						jsonentry["t"].asDouble(),
						jsonentry["l"].asUInt(),
						jsonentry["d"].asBool()
					});
			}

			// put all events ordered on the queue. now we're sure it's sorted
			std::sort(events.begin(), events.end(), [](const Entry& A, const Entry& B) {
				return A.Time < B.Time;
			});

			for(auto evt: events) {
				EventPlaybackQueue.push(evt);
			}

			SongHash = root["song"]["hash"].asString();
			DiffIndex = root["song"]["index"].asUInt();
			UserParameters.deserialize(root["userParameters"]);

			return true;
		}

		bool Replay::Save(std::filesystem::path outputpath)
		{
			Json::Value root;

			for (auto entry : ReplayData)
			{
				Json::Value jsonentry;
				jsonentry["t"] = entry.Time;
				jsonentry["l"] = entry.Lane;
				jsonentry["d"] = (entry.Down != 0);

				root["replayEvents"].append(jsonentry);
			}

			root["song"]["hash"] = SongHash;
			root["song"]["index"] = DiffIndex;

			root["userParameters"] = UserParameters.serialize();

			CreateOfstream(out, outputpath);
			out << root;

			return true;
		}

		void Replay::Update(double Time)
		{
			while (EventPlaybackQueue.size() && 
				EventPlaybackQueue.front().Time <= Time)
			{
				auto evt = EventPlaybackQueue.front();
				
				// push to all listeners
				for (auto &listener : PlaybackListeners)
					listener(evt);

				EventPlaybackQueue.pop();
			}
		}

		void Replay::AddPlaybackListener(OnReplayEvent fn)
		{
			PlaybackListeners.push_back(fn);
		}
		
	}
}
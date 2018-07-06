#include "gamecore-pch.h"
#include "fs.h"

#include <queue>
#include "Replay7K.h"

#include "json.hpp"

using json = nlohmann:Json;

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

		PlayscreenParameters Replay::GetEffectiveParameters() const
		{
			return UserParameters;
		}

		std::string Replay::GetSongHash() const
		{
			return SongHash;
		}

		uint32_t Replay::GetDifficultyIndex() const
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
			
			CreateBinIfstream(in, input);

			Json root;
			root = Json::from_cbor(in);

			std::vector <Entry> events;

			// copy potentially unsorted events
			for (auto jsonentry: root["replayEvents"]) {
				events.push_back(Entry{
						jsonentry["t"],
						jsonentry["l"],
						jsonentry["d"]
					});
			}

			// put all events ordered on the queue. now we're sure it's sorted
			std::sort(events.begin(), events.end(), [](const Entry& A, const Entry& B) {
				return A.Time < B.Time;
			});

			for(auto evt: events) {
				EventPlaybackQueue.push(evt);
			}

			SongHash = root["song"]["hash"];
			DiffIndex = root["song"]["index"];
			UserParameters.deserialize(root["userParameters"]);

			return true;
		}

		bool Replay::Save(std::filesystem::path outputpath) const
		{
			Json root = {
				{"song", 
					{
						{"hash", SongHash},
						{"index", DiffIndex}
					}
				},
				{
					"userParameters",
					UserParameters.serialize()
				}
			};

			for (auto entry : ReplayData)
			{
				Json jsonentry = {
					{"t", entry.Time},
					{"l", entry.Lane},
					{"d", entry.Down != 0}
				};

				root["replayEvents"].push_back(jsonentry);
			}

			
			CreateBinOfstream(out, outputpath);
			auto buf = Json::to_cbor(root);
			out.write((const char*)buf.data(), buf.size());

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
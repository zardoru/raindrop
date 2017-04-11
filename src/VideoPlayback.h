#pragma once

class VideoPlaybackData;

class VideoPlayback : public Texture
{
	double PlaybackTime;
	uint32_t mFrameQueueItems;

	std::thread *mDecodeThread;

	std::atomic<bool> RunDecodeThread;
	// I don't want to recompile this file too often
	// I'm hiding the implementation in VideoPlayback.cpp
	VideoPlaybackData* Context;

	void QueueFrame(); // It's thread-safe so, go! Put on its own thread, though.

	// yeah don't state the type, let the implementation handle that.
	void UpdateVideoTexture(void* data);

	VideoPlayback(VideoPlayback&&) = delete;
	VideoPlayback(VideoPlayback&) = delete;
public:
	VideoPlayback(uint32_t framequeueitems = 2);
	~VideoPlayback();
	bool Open(std::filesystem::path path);
	void Reset();

	void StartDecodeThread();
	void UpdateClock(double new_clock_time);
};
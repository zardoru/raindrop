#pragma once

class VideoPlaybackData;

class VideoPlayback : public Texture2D
{
	double playback_time_;
	uint32_t m_frame_queue_items_;

	std::thread *m_decode_thread_;

	std::atomic<bool> run_decode_thread_;
	// I don't want to recompile this file too often
	// I'm hiding the implementation in VideoPlayback.cpp
	VideoPlaybackData* context_;

	void decode_next_frame() const; // It's thread-safe so, go! Put on its own thread, though.

	// yeah don't state the type, let the implementation handle that.
	void upload_video_texture(void* data);

	VideoPlayback(VideoPlayback&&) = delete;
	VideoPlayback(VideoPlayback&) = delete;
public:
	VideoPlayback(uint32_t framequeueitems = 2);
	~VideoPlayback();
	bool open(const std::filesystem::path &path);
	void reset();

	void start_decode_thread();
	void update_clock(double new_clock_time);
};
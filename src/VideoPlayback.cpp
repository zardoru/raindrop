#include "pch.h"

#include "Texture.h"
#include "VideoPlayback.h"
#include "Logging.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
}
/*
	Much of this implementation are a simplification of the tutorials found at
	http://dranger.com/ffmpeg

	* Differences: Frame queue is a ring buffer of frames rather than packets
	* Frame buffers are stored in vectors
	* Variable frame queue size
	* OpenGL
	
	* No audio sync (external clock)

	* Updating not using a thread timer, 
	but with an external clock (which may be on another thread. to be tested!)
*/

class VideoFrame {
public:
	AVFrame* frame;
	double pts; // seconds

	VideoFrame() {
		frame = nullptr;
		pts = 0;
	}
};

#define MAX_PACKETS 64

class VideoPlaybackData
{
public:
	AVFormatContext *AV;
	AVCodecContext *CodecCtx;
	AVCodecContext *UsableCodecCtx;
	AVCodec *Codec;

	VideoFrame DisplayFrame;
	
	// contains pending AVFrame* to display
	PaUtilRingBuffer mPendingFrameQueue;

	// contains AVFrame* available to write to
	PaUtilRingBuffer mCleanFrameQueue;

	std::vector<char> PendingQueueData;
	std::vector<char> CleanQueueData;

	std::vector<char> DecodedFrameData;
	std::vector<char> FrameData;

	SwsContext *sws_ctx;

	AVFrame* DecodedFrame;

	int videoStreamIndex;

	std::mutex ringbuffer_mutex;
	std::atomic<bool> CleanFrameAvailable;
	std::condition_variable ringbuffer_has_space;

	VideoPlaybackData() {
		AV = nullptr;
		CodecCtx = nullptr;
		Codec = nullptr;
		sws_ctx = nullptr;
	}

	VideoFrame GetCleanFrame()
	{
		VideoFrame ret;
		if (PaUtil_GetRingBufferReadAvailable(&mCleanFrameQueue)) {
			PaUtil_ReadRingBuffer(&mCleanFrameQueue, &ret, 1);
			return ret;
		}
		else
			return VideoFrame();
	}

	VideoFrame GetPendingFrame()
	{
		VideoFrame ret;
		if (PaUtil_GetRingBufferReadAvailable(&mPendingFrameQueue)) {
			PaUtil_ReadRingBuffer(&mPendingFrameQueue, &ret, 1);
			return ret;
		}
		else
			return VideoFrame();
	}

	void PutPendingFrame(VideoFrame frame)
	{
		if (PaUtil_GetRingBufferWriteAvailable(&mPendingFrameQueue)) {
			PaUtil_WriteRingBuffer(&mPendingFrameQueue, &frame, 1);
		}
	}

	void PutCleanFrame(VideoFrame frame)
	{
		if (PaUtil_GetRingBufferWriteAvailable(&mCleanFrameQueue)) {
			PaUtil_WriteRingBuffer(&mCleanFrameQueue, &frame, 1);
		}
	}

	~VideoPlaybackData() {
		av_frame_free(&DisplayFrame.frame);
		
		AVFrame* f;
		while ((f = GetCleanFrame().frame)) {
			av_frame_free(&f);
		}

		while ((f = GetPendingFrame().frame)) {
			av_frame_free(&f);
		}

		avformat_free_context(AV);
		avcodec_free_context(&UsableCodecCtx);
		//avcodec_free_context(&CodecCtx);
		//avcodec_free_context(&Codec);
		sws_freeContext(sws_ctx);
	}

	void InitializeBuffers(uint32_t framecnt, int w, int h) {
		// framecnt += 1;

		auto mem = sizeof(VideoFrame) * framecnt;
		CleanQueueData.assign(mem, 0);
		PendingQueueData.assign(mem, 0);

		PaUtil_InitializeRingBuffer(&mPendingFrameQueue, sizeof(VideoFrame), framecnt, PendingQueueData.data());
		PaUtil_InitializeRingBuffer(&mCleanFrameQueue, sizeof(VideoFrame), framecnt, CleanQueueData.data());

		auto framesize = avpicture_get_size(AV_PIX_FMT_RGB24, w, h) + AV_INPUT_BUFFER_PADDING_SIZE;

		FrameData.assign(framesize * framecnt, 0);

		for (uint32_t i = 0; i < framecnt; i++)
		{
			auto avframe = av_frame_alloc();
			uint8_t *buf = (uint8_t*)(FrameData.data() + i * framesize);
			avpicture_fill((AVPicture*)avframe, buf, AV_PIX_FMT_RGB24, w, h);

			VideoFrame vf;
			vf.frame = avframe;
			PaUtil_WriteRingBuffer(&mCleanFrameQueue, &vf, 1);
		}
	}
};

void InitializeFFMpeg()
{
	static bool initialized = false;
	if (!initialized) {
		av_register_all();
		initialized = true;
	}
}

void VideoPlayback::QueueFrame()
{
	if (!(PaUtil_GetRingBufferReadAvailable(&Context->mCleanFrameQueue) > 0 &&
		PaUtil_GetRingBufferWriteAvailable(&Context->mPendingFrameQueue) > 0)) {
		return;
	}

	int gotframe;
	AVPacket packet;
	while (av_read_frame(Context->AV, &packet) >= 0) { 
		
		if (packet.stream_index == Context->videoStreamIndex) {
			avcodec_decode_video2(Context->UsableCodecCtx, Context->DecodedFrame, &gotframe, &packet);
			if (gotframe) {
				auto cf = Context->GetCleanFrame();

				sws_scale(Context->sws_ctx, (uint8_t const* const*)Context->DecodedFrame->data,
					Context->DecodedFrame->linesize, 0, Context->UsableCodecCtx->height, cf.frame->data, cf.frame->linesize);


				cf.pts = av_frame_get_best_effort_timestamp(Context->DecodedFrame) * av_q2d(Context->AV->streams[Context->videoStreamIndex]->time_base);
				Context->PutPendingFrame(cf);
			} 
		}

		av_free_packet(&packet);

		if (gotframe) break;
	}
}

VideoPlayback::VideoPlayback(uint32_t framequeueitems)
{
	InitializeFFMpeg();
	mFrameQueueItems = framequeueitems;
	Context = nullptr;
	mDecodeThread = nullptr;
}

VideoPlayback::~VideoPlayback()
{
	if (mDecodeThread) {
		RunDecodeThread = false;
		mDecodeThread->join();
	}

	if (Context)
		delete Context;
}

bool VideoPlayback::Open(std::filesystem::path path)
{
	auto newctx = new VideoPlaybackData();
	if (avformat_open_input(&newctx->AV, path.string().c_str(), nullptr, nullptr) < 0) {
		return false;
	}

	if (avformat_find_stream_info(newctx->AV, nullptr) < 0) {
		return false;
	}

	// Find the first video stream
	for (size_t i = 0; i < newctx->AV->nb_streams; i++)
	{
		if (newctx->AV->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
			newctx->videoStreamIndex = i;
			newctx->CodecCtx = newctx->AV->streams[i]->codec;
			break;
		}
	}

	if (!newctx->CodecCtx)
		return false; // Didn't find a video stream

	// decoder find...
	newctx->Codec = avcodec_find_decoder(newctx->CodecCtx->codec_id);
	if (!newctx->Codec)
		return false;

	// context copy
	newctx->UsableCodecCtx = avcodec_alloc_context3(newctx->Codec);
	if (avcodec_copy_context(newctx->UsableCodecCtx, newctx->CodecCtx) != 0) {
		return false;
	}

	// open context?
	if (avcodec_open2(newctx->UsableCodecCtx, newctx->Codec, nullptr) < 0)
		return false;

	newctx->InitializeBuffers(mFrameQueueItems, newctx->UsableCodecCtx->width, newctx->UsableCodecCtx->height);

	/*Log::Printf("Video delay (frames): %d\n", newctx->UsableCodecCtx->delay);
	av_seek_frame(newctx->AV, newctx->videoStreamIndex, newctx->UsableCodecCtx->delay * 2, 0);*/

	auto f = avpicture_get_size(newctx->UsableCodecCtx->pix_fmt, 
		newctx->UsableCodecCtx->width, 
		newctx->UsableCodecCtx->height) + AV_INPUT_BUFFER_PADDING_SIZE;
	newctx->DecodedFrameData.assign(f, 0);

	auto ucc = newctx->UsableCodecCtx;

	w = ucc->width;
	h = ucc->height;

	auto avframe = av_frame_alloc();
	uint8_t *buf = (uint8_t*)newctx->DecodedFrameData.data();
	avpicture_fill((AVPicture*)avframe, buf, ucc->pix_fmt, w, h);
	newctx->DecodedFrame = avframe;

	newctx->sws_ctx = sws_getContext(w, h, 
		ucc->pix_fmt, 
		w, h, 
		AV_PIX_FMT_RGB24, SWS_BILINEAR, 
		nullptr, nullptr, nullptr);

	// only assign on success
	if (Context) delete Context;
	Context = newctx;;
	return true;
}

void VideoPlayback::Reset()
{
}

void VideoPlayback::StartDecodeThread()
{
	RunDecodeThread = true;
	mDecodeThread = new std::thread([&]() {
		while (RunDecodeThread) {

			QueueFrame();

			while (!Context->CleanFrameAvailable) {
				std::unique_lock<std::mutex> lock(Context->ringbuffer_mutex);
				Context->ringbuffer_has_space.wait(lock);
			}
		}
	});
}

void VideoPlayback::UpdateClock(double clock)
{
	bool update = true;

	while (update) {
		if (Context->DisplayFrame.frame) {
			if (Context->DisplayFrame.pts <= clock) {
				UpdateVideoTexture(Context->DisplayFrame.frame);
				Context->PutCleanFrame(Context->DisplayFrame);
				Context->DisplayFrame.frame = nullptr;
			}
			else break;
		}
		else {
			Context->DisplayFrame = Context->GetPendingFrame();
			Context->CleanFrameAvailable = true;
			Context->ringbuffer_has_space.notify_one();

			if (!Context->DisplayFrame.frame) update = false;
		}
	}
}

void VideoPlayback::UpdateVideoTexture(void * data)
{
	AVFrame* frame = (AVFrame*)data;

	CreateTexture();
	Bind();

	if (IsValid) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		if (!TextureAssigned) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);
			TextureAssigned = true;
		}
		else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);
		}
	}
}

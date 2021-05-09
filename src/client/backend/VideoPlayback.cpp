#include <filesystem>
#include <thread>
#include <mutex>
#include <pa_ringbuffer.h>

#include <GL/glew.h>
#include <fstream>
#include "Texture.h"
#include "VideoPlayback.h"
#include "Logging.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/pixfmt.h>
#include <libavutil/imgutils.h>
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

static int readVideoFunction(void* opaque, uint8_t* buf, int buf_size) {
    auto& me = *reinterpret_cast<std::ifstream*>(opaque);
    me.read(reinterpret_cast<char*>(buf), buf_size);
    return me.gcount();
}


class VideoPlaybackData
{
public:
	AVFormatContext *AV;
	AVCodecParameters *CodecCtx;
	AVCodecContext *UsableCodecCtx;
	AVCodec *Codec;

    std::ifstream buf;
    unsigned char* buffer;
    std::shared_ptr<AVIOContext> avioContext;


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

	/*
	 * answer from https://stackoverflow.com/questions/9604633/reading-a-file-located-in-memory-with-libavformat
	 * to read from iostreams
	 * */
	VideoPlaybackData(std::filesystem::path path) :
	    AV(avformat_alloc_context()),
	    buf(path, std::ios::binary),
	    buffer((unsigned char*)av_malloc(8192)),
	    avioContext(avio_alloc_context(
	            buffer,
	            4096, 0,
	            reinterpret_cast<void*>(static_cast<std::istream*>(&buf)),
	             readVideoFunction,
	            nullptr,
	            nullptr),
                 av_free
         ){
		CodecCtx = nullptr;
		Codec = nullptr;
		sws_ctx = nullptr;

        AV->pb = avioContext.get();
        AV->flags |= AVFMT_FLAG_CUSTOM_IO;
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

        // fuck it, leak it
        // avformat_free_context(AV);
        avformat_close_input(&AV);
        avioContext = nullptr;
        // av_free(buffer);


		av_frame_free(&DisplayFrame.frame);
		
		AVFrame* f;
		while ((f = GetCleanFrame().frame)) {
			av_frame_free(&f);
		}

		while ((f = GetPendingFrame().frame)) {
			av_frame_free(&f);
		}



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

		auto frame_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1) + AV_INPUT_BUFFER_PADDING_SIZE;

		FrameData.assign(frame_size * framecnt, 0);

		for (uint32_t i = 0; i < framecnt; i++)
		{
			auto avframe = av_frame_alloc();
			auto *frame_data = (uint8_t*)(FrameData.data() + i * frame_size);
			av_image_fill_arrays(avframe->data, avframe->linesize, frame_data, AV_PIX_FMT_RGB24, w, h, 1);

			VideoFrame vf;
			vf.frame = avframe;
			PaUtil_WriteRingBuffer(&mCleanFrameQueue, &vf, 1);
		}
	}
};


void VideoPlayback::QueueFrame()
{
	if (!(PaUtil_GetRingBufferReadAvailable(&Context->mCleanFrameQueue) > 0 &&
		PaUtil_GetRingBufferWriteAvailable(&Context->mPendingFrameQueue) > 0)) {
		return;
	}

	AVPacket packet;
	while (av_read_frame(Context->AV, &packet) >= 0) {
		bool got_frame = false;

		if (packet.stream_index == Context->videoStreamIndex) {
			// TODO: won't this break?
			auto res = avcodec_send_packet(Context->UsableCodecCtx, &packet);
			if (res < 0)
			{
				// what CAN we do?
				continue;
			}

			if (res >= 0) {
				res = avcodec_receive_frame(Context->UsableCodecCtx, Context->DecodedFrame);

				auto cf = Context->GetCleanFrame();

				sws_scale(
					Context->sws_ctx,
					(uint8_t const* const*)Context->DecodedFrame->data,
					Context->DecodedFrame->linesize,
					0,
					Context->UsableCodecCtx->height,
					cf.frame->data,
					cf.frame->linesize
				);

				cf.pts = Context->DecodedFrame->best_effort_timestamp *
					av_q2d(Context->AV->streams[Context->videoStreamIndex]->time_base);
				Context->PutPendingFrame(std::move(cf));
				got_frame = true;
			}
		}

		av_packet_unref(&packet);

		if (got_frame) break;
	}
}


VideoPlayback::VideoPlayback(uint32_t framequeueitems)
{
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

	delete Context;
}

bool VideoPlayback::Open(std::filesystem::path path)
{
	auto newctx = new VideoPlaybackData(path);

	if (avformat_open_input(&newctx->AV, "dummy", nullptr, nullptr) < 0) {
		delete newctx;
		return false;
	}

	if (avformat_find_stream_info(newctx->AV, nullptr) < 0) {
		delete newctx;
		return false;
	}

	// Find the first video stream
	for (size_t i = 0; i < newctx->AV->nb_streams; i++)
	{
		if (newctx->AV->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			newctx->videoStreamIndex = i;
			newctx->CodecCtx = newctx->AV->streams[i]->codecpar;
			break;
		}
	}

	if (!newctx->CodecCtx) {
		delete newctx;
		return false; // Didn't find a video stream
	}

	// decoder find...
	newctx->Codec = avcodec_find_decoder(newctx->CodecCtx->codec_id);
	if (!newctx->Codec) {
		delete newctx;
		return false;
	}

	// context copy
	newctx->UsableCodecCtx = avcodec_alloc_context3(newctx->Codec);
	if (avcodec_parameters_to_context(newctx->UsableCodecCtx, newctx->CodecCtx) != 0) {
		delete newctx;
		return false;
	}

	if (newctx->UsableCodecCtx->pix_fmt == AV_PIX_FMT_NONE) {
		delete newctx;
		return false;
	}

	// open context?
	if (avcodec_open2(newctx->UsableCodecCtx, newctx->Codec, nullptr) < 0) {
		delete newctx;
		return false;
	}

	newctx->InitializeBuffers(mFrameQueueItems, newctx->UsableCodecCtx->width, newctx->UsableCodecCtx->height);

	/*Log::Printf("Video delay (frames): %d\n", newctx->UsableCodecCtx->delay);
	av_seek_frame(newctx->AV, newctx->videoStreamIndex, newctx->UsableCodecCtx->delay * 2, 0);*/

	auto f = av_image_get_buffer_size(
		newctx->UsableCodecCtx->pix_fmt,
		newctx->UsableCodecCtx->width,
		newctx->UsableCodecCtx->height,
		1) + AV_INPUT_BUFFER_PADDING_SIZE;
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

			while (!Context->CleanFrameAvailable && RunDecodeThread) {
				std::unique_lock<std::mutex> lock(Context->ringbuffer_mutex);
				Context->ringbuffer_has_space.wait_for(lock, std::chrono::seconds(1));
			}
		}
	});
}

void VideoPlayback::UpdateClock(double clock)
{
	bool update = true;

	if (!Context) return;
	if (clock < 0) return; 

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

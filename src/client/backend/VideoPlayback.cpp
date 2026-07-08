#include <filesystem>
#include <thread>
#include <mutex>
#include <pa_ringbuffer.h>

#include <GL/glew.h>
#include <fstream>
#include "Texture2D.h"
#include "VideoPlayback.h"

#include <condition_variable>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
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

static int readVideoFunction(void* opaque, uint8_t* buf, const int buf_size) {
    auto& me = *reinterpret_cast<std::ifstream*>(opaque);
    me.read(reinterpret_cast<char*>(buf), buf_size);
    return me.gcount();
}


class VideoPlaybackData
{
public:
	AVFormatContext *av;
	AVCodecParameters *codec_ctx;
	AVCodecContext *usable_codec_ctx;
	const AVCodec *codec;

    std::ifstream buf;
    unsigned char* buffer;
    std::shared_ptr<AVIOContext> avio_context;


    VideoFrame display_frame;
	
	// contains pending AVFrame* to display
	PaUtilRingBuffer m_pending_frame_queue;

	// contains AVFrame* available to write to
	PaUtilRingBuffer m_frame_pool;

	std::vector<uint8_t> pending_queue_data;
	std::vector<uint8_t> clean_queue_data;

	std::vector<uint8_t> decoded_frame_data;
	std::vector<uint8_t> frame_data;

	SwsContext *sws_ctx;

	AVFrame* decoded_frame;

	int video_stream_index;

	std::mutex ringbuffer_mutex;
	std::atomic<bool> is_clean_frame_available;
	std::condition_variable ringbuffer_has_space;

	/*
	 * answer from https://stackoverflow.com/questions/9604633/reading-a-file-located-in-memory-with-libavformat
	 * to read from iostreams
	 * */
	VideoPlaybackData(const std::filesystem::path &path) :
	    av(avformat_alloc_context()),
	    buf(path, std::ios::binary),
	    buffer((unsigned char*)av_malloc(8192)),
	    avio_context(avio_alloc_context(
	            buffer,
	            4096, 0,
	            reinterpret_cast<void*>(static_cast<std::istream*>(&buf)),
	             readVideoFunction,
	            nullptr,
	            nullptr),
                 av_free
         ){
		codec_ctx = nullptr;
		codec = nullptr;
		sws_ctx = nullptr;

        av->pb = avio_context.get();
        av->flags |= AVFMT_FLAG_CUSTOM_IO;
	}

	VideoFrame alloc_frame()
	{
		VideoFrame ret;
		if (PaUtil_GetRingBufferReadAvailable(&m_frame_pool)) {
			PaUtil_ReadRingBuffer(&m_frame_pool, &ret, 1);
			return ret;
		}
		else
			return VideoFrame();
	}

	VideoFrame get_decoded_frame()
	{
		VideoFrame ret;
		if (PaUtil_GetRingBufferReadAvailable(&m_pending_frame_queue)) {
			PaUtil_ReadRingBuffer(&m_pending_frame_queue, &ret, 1);
			return ret;
		}
		else
			return VideoFrame();
	}

	void put_decoded_frame(const VideoFrame frame)
	{
		if (PaUtil_GetRingBufferWriteAvailable(&m_pending_frame_queue)) {
			PaUtil_WriteRingBuffer(&m_pending_frame_queue, &frame, 1);
		}
	}

	void put_clean_frame(const VideoFrame frame)
	{
		if (PaUtil_GetRingBufferWriteAvailable(&m_frame_pool)) {
			PaUtil_WriteRingBuffer(&m_frame_pool, &frame, 1);
		}
	}

	~VideoPlaybackData() {

        // fuck it, leak it
        // avformat_free_context(AV);
        avformat_close_input(&av);
        avio_context = nullptr;
        // av_free(buffer);


		av_frame_free(&display_frame.frame);
		
		AVFrame* f;
		while ((f = alloc_frame().frame)) {
			av_frame_free(&f);
		}

		while ((f = get_decoded_frame().frame)) {
			av_frame_free(&f);
		}



		avcodec_free_context(&usable_codec_ctx);
		//avcodec_free_context(&CodecCtx);
		//avcodec_free_context(&Codec);
		sws_freeContext(sws_ctx);
	}

	void initialize_buffers(const uint32_t framecnt, const int w, const int h) {
		// framecnt += 1;

		const auto mem = sizeof(VideoFrame) * framecnt;
		clean_queue_data.assign(mem, 0);
		pending_queue_data.assign(mem, 0);

		PaUtil_InitializeRingBuffer(&m_pending_frame_queue, sizeof(VideoFrame), framecnt, pending_queue_data.data());
		PaUtil_InitializeRingBuffer(&m_frame_pool, sizeof(VideoFrame), framecnt, clean_queue_data.data());

		const auto frame_size = av_image_get_buffer_size(AV_PIX_FMT_RGB24, w, h, 1) + AV_INPUT_BUFFER_PADDING_SIZE;

		frame_data.assign(frame_size * framecnt, 0);

		for (uint32_t i = 0; i < framecnt; i++)
		{
			const auto avframe = av_frame_alloc();
			const auto *frame_datad = (uint8_t*)(frame_data.data() + i * frame_size);
			av_image_fill_arrays(avframe->data, avframe->linesize, frame_datad, AV_PIX_FMT_RGB24, w, h, 1);

			VideoFrame vf;
			vf.frame = avframe;
			PaUtil_WriteRingBuffer(&m_frame_pool, &vf, 1);
		}
	}
};


void VideoPlayback::decode_next_frame() const {
	if (!(PaUtil_GetRingBufferReadAvailable(&context_->m_frame_pool) > 0 &&
		PaUtil_GetRingBufferWriteAvailable(&context_->m_pending_frame_queue) > 0)) {
		return;
	}

	AVPacket packet;
	while (av_read_frame(context_->av, &packet) >= 0) {
		bool got_frame = false;

		if (packet.stream_index == context_->video_stream_index) {
			auto res = avcodec_send_packet(context_->usable_codec_ctx, &packet);
			if (res < 0)
			{
				// what CAN we do?
                av_packet_unref(&packet);
				continue;
			}

			if (res >= 0) {
				res = avcodec_receive_frame(context_->usable_codec_ctx, context_->decoded_frame);

				auto cf = context_->alloc_frame();

				sws_scale(
					context_->sws_ctx,
					(uint8_t const* const*)context_->decoded_frame->data,
					context_->decoded_frame->linesize,
					0,
					context_->usable_codec_ctx->height,
					cf.frame->data,
					cf.frame->linesize
				);

				cf.pts = context_->decoded_frame->best_effort_timestamp *
					av_q2d(context_->av->streams[context_->video_stream_index]->time_base);
				context_->put_decoded_frame(std::move(cf));
				got_frame = true;
			}
		}

		av_packet_unref(&packet);

		if (got_frame) break;
	}
}


VideoPlayback::VideoPlayback(const uint32_t framequeueitems)
{
	m_frame_queue_items_ = framequeueitems;
	context_ = nullptr;
	m_decode_thread_ = nullptr;
}

VideoPlayback::~VideoPlayback()
{
	if (m_decode_thread_) {
		run_decode_thread_ = false;

		m_decode_thread_->join();
	}

	delete context_;
}

bool VideoPlayback::open(const std::filesystem::path &path)
{
	const auto newctx = new VideoPlaybackData(path);

	if (avformat_open_input(&newctx->av, "dummy", nullptr, nullptr) < 0) {
		delete newctx;
		return false;
	}

	if (avformat_find_stream_info(newctx->av, nullptr) < 0) {
		delete newctx;
		return false;
	}

	// Find the first video stream
	for (size_t i = 0; i < newctx->av->nb_streams; i++)
	{
		if (newctx->av->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
			newctx->video_stream_index = i;
			newctx->codec_ctx = newctx->av->streams[i]->codecpar;
			break;
		}
	}

	if (!newctx->codec_ctx) {
		delete newctx;
		return false; // Didn't find a video stream
	}

	// decoder find...
	newctx->codec = avcodec_find_decoder(newctx->codec_ctx->codec_id);
	if (!newctx->codec) {
		delete newctx;
		return false;
	}

	// context copy
	newctx->usable_codec_ctx = avcodec_alloc_context3(newctx->codec);
	if (avcodec_parameters_to_context(newctx->usable_codec_ctx, newctx->codec_ctx) != 0) {
		delete newctx;
		return false;
	}

	if (newctx->usable_codec_ctx->pix_fmt == AV_PIX_FMT_NONE) {
		delete newctx;
		return false;
	}

	// open context?
	if (avcodec_open2(newctx->usable_codec_ctx, newctx->codec, nullptr) < 0) {
		delete newctx;
		return false;
	}

	newctx->initialize_buffers(m_frame_queue_items_, newctx->usable_codec_ctx->width, newctx->usable_codec_ctx->height);

	/*Log::Printf("Video delay (frames): %d\n", newctx->UsableCodecCtx->delay);
	av_seek_frame(newctx->AV, newctx->videoStreamIndex, newctx->UsableCodecCtx->delay * 2, 0);*/

	const auto f = av_image_get_buffer_size(
		newctx->usable_codec_ctx->pix_fmt,
		newctx->usable_codec_ctx->width,
		newctx->usable_codec_ctx->height,
		1) + AV_INPUT_BUFFER_PADDING_SIZE;
	newctx->decoded_frame_data.assign(f, 0);

	const auto ucc = newctx->usable_codec_ctx;

	w = ucc->width;
	h = ucc->height;

	const auto avframe = av_frame_alloc();
	const uint8_t *buf = newctx->decoded_frame_data.data();
	avframe->format = ucc->pix_fmt;
	avframe->width = w;
	avframe->height = h;
	av_image_fill_arrays(avframe->data, avframe->linesize, buf, ucc->pix_fmt, w, h, 1);
	newctx->decoded_frame = avframe;

	newctx->sws_ctx = sws_getContext(w, h, 
		ucc->pix_fmt, 
		w, h, 
		AV_PIX_FMT_RGB24, SWS_BILINEAR, 
		nullptr, nullptr, nullptr);

	// only assign on success
	delete context_;
	context_ = newctx;
	return true;
}

void VideoPlayback::reset()
{
}

void VideoPlayback::start_decode_thread()
{
	run_decode_thread_ = true;
	m_decode_thread_ = new std::thread([&]() {
		while (run_decode_thread_) {

			decode_next_frame();

			while (!context_->is_clean_frame_available && run_decode_thread_) {
				std::unique_lock<std::mutex> lock(context_->ringbuffer_mutex);
				context_->ringbuffer_has_space.wait_for(lock, std::chrono::seconds(1));
			}
		}
	});
}

void VideoPlayback::update_clock(const double clock)
{
	bool update = true;

	if (!context_) return;
	if (clock < 0) return; 

	while (update) {
		if (context_->display_frame.frame) {
			if (context_->display_frame.pts <= clock) {
				upload_video_texture(context_->display_frame.frame);
				context_->put_clean_frame(context_->display_frame);
				context_->display_frame.frame = nullptr;
			}
			else break;
		}
		else {
			context_->display_frame = context_->get_decoded_frame();
			context_->is_clean_frame_available = true;
			context_->ringbuffer_has_space.notify_one();

			if (!context_->display_frame.frame) update = false;
		}
	}
}

void VideoPlayback::upload_video_texture(void * data)
{
	const auto* frame = (AVFrame*)data;

	ensure_current_gpu_texture_2d();
	bind();

	if (is_valid_) {
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		if (!texture_was_assigned_) {
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);
			texture_was_assigned_ = true;
		}
		else {
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, frame->data[0]);
		}
	}
}

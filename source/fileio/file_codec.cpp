#include "fileio/file_codec.h"

FileCodec::FileCodec() :
   FrameWidth( 0 ), FrameHeight( 0 ), FrameIndex( 0 ), Framerate( 0 ), VideoCodecID( AV_CODEC_ID_NONE ),
   PixelFormat( AV_PIX_FMT_RGBA ), SampleFormat( AV_SAMPLE_FMT_FLTP ), Samplerate( 44'100 ), SWSContext( nullptr ),
   VideoCodecContext( nullptr ), Packet( nullptr )
{
}

FileCodec::~FileCodec()
{
   if (VideoCodecContext != nullptr) avcodec_free_context( &VideoCodecContext );
   if (Packet != nullptr) av_packet_free( &Packet );
   if (SWSContext != nullptr) {
      sws_freeContext( SWSContext );
      SWSContext = nullptr;
   }
   FrameWidth = 0;
   FrameHeight = 0;
   FrameIndex = 0;
   Framerate = 0.0;
   VideoCodecID = AV_CODEC_ID_NONE;
}

void FileCodec::setVideoCodecContextFlag(uint flag) const
{
   VideoCodecContext->flags = static_cast<int>(static_cast<uint>(VideoCodecContext->flags) | flag);
}

void FileCodec::setVideoCodecParameters(AVCodecParameters* parameters) const
{
   avcodec_parameters_from_context( parameters, VideoCodecContext );
}

void FileCodec::flip(AVFrame* frame)
{
   for (int c = 0; c < 4; ++c) {
      frame->data[c] += frame->linesize[c] * (frame->height - 1);
      frame->linesize[c] *= -1;
   }
}

void FileCodec::copyFrame(AVFrame** dst, const AVFrame* src)
{
   *dst = av_frame_alloc();
   (*dst)->width = src->width;
   (*dst)->height = src->height;
   (*dst)->channels = src->channels;
   (*dst)->nb_samples = src->nb_samples;
   (*dst)->format = src->format;
   (*dst)->channel_layout = src->channel_layout;
   av_frame_get_buffer( *dst, 0 );
   av_frame_copy( *dst, src );
   av_frame_copy_props( *dst, src );
}
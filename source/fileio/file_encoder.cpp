#include "fileio/file_encoder.h"

FileEncoder::FileEncoder() :
   Bitrate( 5'000'000 ), GOPSize( 15 ), EncodingBuffer( nullptr ), OriginalFrame( nullptr ), EncodedFrame( nullptr )
{
}

void FileEncoder::setVideoCodecContext(const AVCodec* encoder)
{
   if (encoder == nullptr) throw std::runtime_error("Could not find encoder");
   VideoCodecContext = avcodec_alloc_context3( encoder );
   if (VideoCodecContext == nullptr)
      throw std::runtime_error("Could not find video codec context");

   VideoCodecContext->width = FrameWidth;
   VideoCodecContext->height = FrameHeight;
   VideoCodecContext->bit_rate = Bitrate;
   VideoCodecContext->gop_size = GOPSize;
   VideoCodecContext->pix_fmt = AV_PIX_FMT_YUV420P;
   VideoCodecContext->time_base = av_d2q( 1.0 / Framerate, 1 );
   VideoCodecContext->framerate = av_d2q( Framerate, 1000 );
   switch (VideoCodecID) {
      case AV_CODEC_ID_H264:
         // reference: https://trac.ffmpeg.org/wiki/Encode/H.264
         av_opt_set( VideoCodecContext, "preset", "fast", 0 );
         //av_opt_set( VideoCodecContext, "tune", "zerolatency", 0 );
         av_opt_set( VideoCodecContext, "profile:v", "main", 0 );
         av_opt_set( VideoCodecContext, "b", std::to_string( Bitrate ).c_str(), 0 );
         break;
      case AV_CODEC_ID_MJPEG:
         VideoCodecContext->pix_fmt = AV_PIX_FMT_YUVJ420P;
         break;
      default:
         break;
   }

   OriginalFrame = av_frame_alloc();
   EncodedFrame = av_frame_alloc();
   if (OriginalFrame == nullptr || EncodedFrame == nullptr) throw std::runtime_error("Could not allocate AVFrame");
   OriginalFrame->width = EncodedFrame->width = FrameWidth;
   OriginalFrame->height = EncodedFrame->height = FrameHeight;
   OriginalFrame->format = PixelFormat;
   EncodedFrame->format = VideoCodecContext->pix_fmt;
   SWSContext = sws_getContext(
      FrameWidth, FrameHeight, PixelFormat,
      FrameWidth, FrameHeight, VideoCodecContext->pix_fmt,
      SWS_FAST_BILINEAR, nullptr, nullptr, nullptr
   );

   if (avcodec_open2( VideoCodecContext, encoder, nullptr ) < 0) {
      close();
      throw std::runtime_error("Could not open AVCodec");
   }
}

bool FileEncoder::openVideo(
   int frame_width,
   int frame_height,
   float framerate,
   AVCodecID codec_id
)
{
   FrameWidth = frame_width;
   FrameHeight = static_cast<int>(((frame_height + 1u) >> 1u) << 1u);
   Framerate = framerate;
   VideoCodecID = codec_id;
   const AVCodec* encoder;
   switch (VideoCodecID) {
      case AV_CODEC_ID_H264:
         encoder = avcodec_find_encoder_by_name( "libx264" );
         break;
      case AV_CODEC_ID_H263:
      case AV_CODEC_ID_H263I:
         VideoCodecID = AV_CODEC_ID_H263P;
      default:
         encoder = avcodec_find_encoder( VideoCodecID );
         break;
   }
   setVideoCodecContext( encoder );
   Packet = av_packet_alloc();
   return true;
}

void FileEncoder::close()
{
   if (OriginalFrame != nullptr) av_frame_free( &OriginalFrame );
   if (EncodedFrame != nullptr) av_frame_free( &EncodedFrame );
   if (EncodingBuffer != nullptr) {
      free( EncodingBuffer );
      EncodingBuffer = nullptr;
   }
}

void FileEncoder::reallocateEncodingBufferIfNeeded()
{
   const int buffer_size = av_image_get_buffer_size(
      VideoCodecContext->pix_fmt,
      FrameWidth,
      FrameHeight,
      1
   );
   if (sizeof( EncodingBuffer ) < buffer_size) {
      if (EncodingBuffer != nullptr) free( EncodingBuffer );
      EncodingBuffer = (uint8_t*)malloc( buffer_size * sizeof( uint8_t ) );
   }
}

bool FileEncoder::writeVideoFrame(AVFormatContext* format_context, const AVFrame* frame, int track_id) const
{
   if (avcodec_send_frame( VideoCodecContext, frame ) < 0) return false;

   int received = 0;
   AVStream* stream = format_context->streams[track_id];
   while (received >= 0) {
      received = avcodec_receive_packet( VideoCodecContext, Packet );
      if (received == AVERROR( EAGAIN ) || received == ERROR_EOF) return true;
      else if (received < 0) break;

      av_packet_rescale_ts( Packet, VideoCodecContext->time_base, stream->time_base );
      Packet->stream_index = stream->index;
      av_interleaved_write_frame( format_context, Packet );
      av_packet_unref( Packet );
   }
   return false;
}

bool FileEncoder::encode(AVFormatContext* format_context, const uint8_t* image_buffer, int track_id)
{
   av_image_fill_arrays(
      OriginalFrame->data, OriginalFrame->linesize, image_buffer,
      PixelFormat, FrameWidth, FrameHeight, 1
   );

   AVFrame* frame = OriginalFrame;
   flip( frame );

   if (VideoCodecContext->pix_fmt != PixelFormat) {
      reallocateEncodingBufferIfNeeded();
      av_image_fill_arrays(
         EncodedFrame->data, EncodedFrame->linesize, EncodingBuffer,
         VideoCodecContext->pix_fmt, FrameWidth, FrameHeight, 1
      );
      sws_scale(
         SWSContext, frame->data, frame->linesize,
         0, FrameHeight,
         EncodedFrame->data, EncodedFrame->linesize
      );
      frame = EncodedFrame;
   }
   frame->pts = FrameIndex++;
   return writeVideoFrame( format_context, frame, track_id );
}

bool FileEncoder::flushVideo(AVFormatContext* format_context, int track_id)
{
   return writeVideoFrame( format_context, nullptr, track_id );
}
#pragma once

#include "fileio/file_codec.h"

class FileEncoder final : public FileCodec
{
public:
   FileEncoder();
   ~FileEncoder() override = default;

   [[nodiscard]] bool openVideo(
      int frame_width,
      int frame_height,
      float framerate,
      AVCodecID codec_id
   );
   void close();
   bool encode(AVFormatContext* format_context, const uint8_t* image_buffer, int track_id);
   bool flushVideo(AVFormatContext* format_context, int track_id);

protected:
   int Bitrate;
   int GOPSize;
   uint8_t* EncodingBuffer;
   AVFrame* OriginalFrame;
   AVFrame* EncodedFrame;

   void setVideoCodecContext(const AVCodec* encoder);
   void reallocateEncodingBufferIfNeeded();
   bool writeVideoFrame(AVFormatContext* format_context, const AVFrame* frame, int track_id) const;
};
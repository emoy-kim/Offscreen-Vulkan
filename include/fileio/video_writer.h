#pragma once

#include "fileio/file_encoder.h"

class VideoWriter
{
public:
   VideoWriter();
   ~VideoWriter();

   [[nodiscard]] bool open(
      const std::filesystem::path& video_file_path,
      int frame_width,
      int frame_height,
      float framerate,
      AVCodecID codec_id
   );
   void close();

   void writeVideo(const uint8_t* image_buffer) const;
   void operator<<(const uint8_t* image_buffer) const
   {
      VideoEncoder->encode( FormatContext, image_buffer, VideoTrackID );
   }

private:
   bool HeaderWritten;
   int VideoTrackID;
   int FrameWidth;
   int FrameHeight;
   int FrameIndex;
   AVRational InverseFramerate;
   AVFormatContext* FormatContext;
   uint8_t* IOContextBuffer;
   size_t IOContextBufferSize;
   std::unique_ptr<FileEncoder> VideoEncoder;

   [[nodiscard]] bool initialize(const std::filesystem::path& video_file_path);
   void writeHeader();
   void addVideoTrack();
};
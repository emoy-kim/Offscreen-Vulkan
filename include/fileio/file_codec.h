#pragma once

#include "base.h"

extern "C"
{
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/avstring.h>
#include <libavutil/channel_layout.h>
#include <libavutil/samplefmt.h>
#include <libavutil/opt.h>
#include <libavutil/frame.h>
#include <libswresample/swresample.h>
};

constexpr int STREAM_NOT_FOUND = -static_cast<int>(
   0xF8u | (static_cast<uint>('S') << 8u) | (static_cast<uint>('T') << 16u) | (static_cast<uint>('R') << 24u)
);
constexpr int ERROR_EOF = -static_cast<int>(
   static_cast<uint>('E') | (static_cast<uint>('O') << 8u) | (static_cast<uint>('F') << 16u) | (static_cast<uint>(' ') << 24u)
);
constexpr uint FORMAT_OPENED_NO_FILE = 0x0001u;
constexpr uint FORMAT_WANTS_GLOBAL_HEADER = 0x0040u;
constexpr uint CODEC_FLAG_GLOBAL_HEADER = 1u << 22u;

class FileCodec
{
public:
   FileCodec();
   virtual ~FileCodec();

   [[nodiscard]] int getFrameWidth() const { return FrameWidth; }
   [[nodiscard]] int getFrameHeight() const { return FrameHeight; }
   [[nodiscard]] int getFrameIndex() const { return FrameIndex; }
   [[nodiscard]] double getFramerate() const { return Framerate; }
   void setVideoCodecContextFlag(uint flag) const;
   void setVideoCodecParameters(AVCodecParameters* parameters) const;
   static void copyFrame(AVFrame** dst, const AVFrame* src);

protected:
   int FrameWidth;
   int FrameHeight;
   int FrameIndex;
   double Framerate;
   AVCodecID VideoCodecID;
   AVPixelFormat PixelFormat;
   AVSampleFormat SampleFormat;
   int Samplerate;
   SwsContext* SWSContext;
   AVCodecContext* VideoCodecContext;
   AVPacket* Packet;

   static void flip(AVFrame* frame);
};
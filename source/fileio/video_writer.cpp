#include "fileio/video_writer.h"

VideoWriter::VideoWriter() :
   HeaderWritten( false ), VideoTrackID( -1 ), FrameWidth( 0 ), FrameHeight( 0 ), FrameIndex( 0 ), InverseFramerate(),
   FormatContext( nullptr ), IOContextBuffer( nullptr ), IOContextBufferSize( 32 * 1024 ), VideoEncoder( nullptr )
{
}

VideoWriter::~VideoWriter()
{
   close();
}

bool VideoWriter::initialize(const std::filesystem::path& video_file_path)
{
   close();
   FormatContext = avformat_alloc_context();
   if (FormatContext == nullptr) return false;
   FormatContext->oformat =
      av_guess_format( nullptr, video_file_path.string().c_str(), nullptr );
   if (FormatContext->oformat == nullptr || (static_cast<uint>(FormatContext->oformat->flags) & FORMAT_OPENED_NO_FILE)) {
      close();
      return false;
   }
   if (avio_open( &FormatContext->pb, video_file_path.string().c_str(), AVIO_FLAG_WRITE ) < 0) {
      close();
      return false;
   }
   return true;
}

void VideoWriter::writeHeader()
{
   if (FormatContext == nullptr || FormatContext->pb == nullptr) return;

   AVDictionary* options = nullptr;
   av_dict_set( &options, "brand", "mp42", 0 );
   if (avformat_write_header( FormatContext, &options ) < 0) {
      close();
      throw std::runtime_error("Could not write video header");
   }
   HeaderWritten = true;
}

void VideoWriter::addVideoTrack()
{
   AVStream* stream = avformat_new_stream( FormatContext, nullptr );
   if (stream == nullptr) return;
   VideoTrackID = stream->index;
   if (static_cast<uint>(FormatContext->oformat->flags) & FORMAT_WANTS_GLOBAL_HEADER) {
      VideoEncoder->setVideoCodecContextFlag( CODEC_FLAG_GLOBAL_HEADER );
   }
   VideoEncoder->setVideoCodecParameters( stream->codecpar );
}

bool VideoWriter::open(
   const std::filesystem::path& video_file_path,
   int frame_width,
   int frame_height,
   float framerate,
   AVCodecID codec_id
)
{
   FrameWidth = frame_width;
   FrameHeight = frame_height;
   if (!initialize( video_file_path )) {
      close();
      throw std::runtime_error("Could not initialize video writer");
   }

   VideoEncoder = std::make_unique<FileEncoder>();
   if (!VideoEncoder->openVideo( frame_width, frame_height, framerate, codec_id )) {
      close();
      throw std::runtime_error("Could not open video");
   }

   addVideoTrack();
   writeHeader();
   return true;
}

void VideoWriter::close()
{
   if (FormatContext != nullptr) {
      VideoEncoder->flushVideo( FormatContext, VideoTrackID );
      if (HeaderWritten) av_write_trailer( FormatContext );
      if (FormatContext->pb != nullptr) {
         avio_close( FormatContext->pb );
         FormatContext->pb = nullptr;
      }
      avformat_free_context( FormatContext );
      FormatContext = nullptr;
   }
   if (VideoEncoder != nullptr) VideoEncoder->close();
   HeaderWritten = false;
   FrameIndex = 0;
   VideoTrackID = -1;
}

void VideoWriter::writeVideo(const uint8_t* image_buffer) const
{
   if (!HeaderWritten || VideoTrackID >= static_cast<int>(FormatContext->nb_streams)) return;
   if (!VideoEncoder->encode( FormatContext, image_buffer, VideoTrackID )) {
      throw std::runtime_error("Encoding is not properly processed");
   }
}
#pragma once
// ─── formats/VideoFile.h ──────────────────────────────────────────────────────
// FFmpeg-backed decoder for NFSMW movie files (EA Multimedia container with On2
// VP6 video + EA ADPCM/PCM audio, e.g. MOVIES/*.vp6). Used by ui/VideoPanel.
//
// Responsibilities (no GL, no audio device — those live in VideoPanel):
//   • Open a file, expose stream metadata (resolution, fps, duration, codecs).
//   • Pre-decode the whole audio track to interleaved S16 (source rate/channels).
//   • Decode video frames one at a time to RGBA for upload to a GL texture.
//   • Restart() seeks to the start so the panel can loop.
//
// FFmpeg headers are kept out of this header (opaque forward declarations) so
// the rest of the app doesn't need libav include paths.
// ─────────────────────────────────────────────────────────────────────────────
#include "Common.h"
#include <filesystem>
#include <string>
#include <vector>
#include <cstdint>

struct AVFormatContext;
struct AVCodecContext;
struct SwsContext;
struct SwrContext;
struct AVPacket;
struct AVFrame;

namespace nfsmw {

struct VideoInfo {
    int         width          = 0;
    int         height         = 0;
    double      fps            = 0.0;
    double      durationSec    = 0.0;
    std::string containerName;          ///< demuxer name (e.g. "ea")
    std::string videoCodec;             ///< e.g. "vp6f"
    std::string audioCodec;             ///< e.g. "adpcm_ea_xas"
    bool        hasAudio       = false;
    int         audioSampleRate = 0;
    int         audioChannels   = 0;
    uint64_t    fileSize        = 0;
};

class VideoFile {
public:
    VideoFile() = default;
    ~VideoFile();
    VideoFile(const VideoFile&)            = delete;
    VideoFile& operator=(const VideoFile&) = delete;

    /// Open `path` and pre-decode the audio track. Video decoding is lazy.
    Result<void> Open(const std::filesystem::path& path);
    void Close();
    bool IsOpen() const { return fmt_ != nullptr; }

    const VideoInfo& Info() const { return info_; }

    /// Fully decoded interleaved S16 audio at info_.audioSampleRate /
    /// audioChannels. Empty when the file has no audio.
    const std::vector<int16_t>& AudioPCM() const { return audioPCM_; }

    /// Decode the next video frame into `rgba` (width*height*4 bytes, row-major,
    /// top-down). Sets `ptsSec` to the frame's presentation time. Returns false
    /// at end-of-stream.
    bool NextFrame(std::vector<uint8_t>& rgba, double& ptsSec);

    /// Seek back to the first frame and flush decoders (for looping/replay).
    void Restart();

    // ── Transcode helpers (static — no open VideoFile required) ──────────────

    /// Transcode an EA Multimedia / VP6 file to a standard H.264+AAC MP4.
    /// `srcVp6` must be a path FFmpeg can demux (ea / vp6f).
    /// Returns an error string on failure.
    static Result<void> ExportMP4(const std::filesystem::path& srcVp6,
                                   const std::filesystem::path& dstMp4);

    /// Transcode a standard MP4 (any FFmpeg-readable video/audio codec) to an
    /// EA Multimedia container with VP6 video + PCM audio, overwriting `dstVp6`.
    /// Callers must create a .bak before calling this.
    /// targetWidth/Height/Fps = 0 means keep source values.
    static Result<void> ImportMP4(const std::filesystem::path& srcMp4,
                                   const std::filesystem::path& dstVp6,
                                   int    targetWidth  = 0,
                                   int    targetHeight = 0,
                                   double targetFps    = 0.0);

private:
    void DecodeAllAudio();

    AVFormatContext* fmt_   = nullptr;
    AVCodecContext*  vctx_  = nullptr;
    AVCodecContext*  actx_  = nullptr;
    SwsContext*      sws_   = nullptr;
    SwrContext*      swr_   = nullptr;
    AVPacket*        pkt_   = nullptr;
    AVFrame*         frame_ = nullptr;

    int  vstream_ = -1;
    int  astream_ = -1;
    bool sentEof_ = false;   ///< video decoder has been flushed (null packet sent)

    VideoInfo            info_;
    std::vector<int16_t> audioPCM_;
};

} // namespace nfsmw

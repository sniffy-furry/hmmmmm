#include "formats/VideoFile.h"

namespace nfsmw {

VideoFile::~VideoFile() { Close(); }

Result<void> VideoFile::Open(const std::filesystem::path&) {
    return Result<void>::Err("Video support is disabled in the initial Android port");
}

void VideoFile::Close() {
    fmt_ = nullptr;
    vctx_ = nullptr;
    actx_ = nullptr;
    sws_ = nullptr;
    swr_ = nullptr;
    pkt_ = nullptr;
    frame_ = nullptr;
    vstream_ = -1;
    astream_ = -1;
    sentEof_ = false;
    info_ = {};
    audioPCM_.clear();
}

bool VideoFile::NextFrame(std::vector<uint8_t>& rgba, double& ptsSec) {
    rgba.clear();
    ptsSec = 0.0;
    return false;
}

void VideoFile::Restart() {}

Result<void> VideoFile::ExportMP4(const std::filesystem::path&,
                                  const std::filesystem::path&) {
    return Result<void>::Err("Video export is disabled in the initial Android port");
}

Result<void> VideoFile::ImportMP4(const std::filesystem::path&,
                                  const std::filesystem::path&,
                                  int,
                                  int,
                                  double) {
    return Result<void>::Err("Video import is disabled in the initial Android port");
}

void VideoFile::DecodeAllAudio() {}

} // namespace nfsmw


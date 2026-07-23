#include "formats/VideoFile.h"
#include "core/Logger.h"

#include <fstream>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/rational.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}

#include <filesystem>

namespace nfsmw {
namespace {

// Open a decoder context for the given stream.
Result<AVCodecContext*> OpenCodec(AVFormatContext* fmt, int streamIdx) {
    AVStream* st = fmt->streams[streamIdx];
    const AVCodec* dec = avcodec_find_decoder(st->codecpar->codec_id);
    if (!dec)
        return Result<AVCodecContext*>::Err("no decoder for codec id "
                                            + std::to_string((int)st->codecpar->codec_id));
    AVCodecContext* ctx = avcodec_alloc_context3(dec);
    if (!ctx) return Result<AVCodecContext*>::Err("avcodec_alloc_context3 failed");
    if (avcodec_parameters_to_context(ctx, st->codecpar) < 0) {
        avcodec_free_context(&ctx);
        return Result<AVCodecContext*>::Err("avcodec_parameters_to_context failed");
    }
    if (avcodec_open2(ctx, dec, nullptr) < 0) {
        avcodec_free_context(&ctx);
        return Result<AVCodecContext*>::Err("avcodec_open2 failed");
    }
    return Result<AVCodecContext*>::Ok(ctx);
}

} // namespace

VideoFile::~VideoFile() { Close(); }

Result<void> VideoFile::Open(const std::filesystem::path& path) {
    Close();

    const std::string p = path.string();
    if (avformat_open_input(&fmt_, p.c_str(), nullptr, nullptr) < 0)
        return Result<void>::Err("Could not open '" + p + "'");
    if (avformat_find_stream_info(fmt_, nullptr) < 0) {
        Close();
        return Result<void>::Err("Could not read stream info");
    }

    vstream_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    astream_ = av_find_best_stream(fmt_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (vstream_ < 0) { Close(); return Result<void>::Err("No video stream found"); }

    auto v = OpenCodec(fmt_, vstream_);
    if (!v) { Close(); return Result<void>::Err("Video decoder: " + v.error); }
    vctx_ = v.value;

    if (astream_ >= 0) {
        auto a = OpenCodec(fmt_, astream_);
        if (a) actx_ = a.value;
        else   { LOG_WARN("VideoFile: audio decoder failed ({}), continuing silent", a.error);
                 astream_ = -1; }
    }

    // ── Metadata ─────────────────────────────────────────────────────────────
    AVStream* vs = fmt_->streams[vstream_];
    info_ = {};
    info_.width  = vctx_->width;
    info_.height = vctx_->height;
    AVRational fr = vs->avg_frame_rate.num ? vs->avg_frame_rate : vs->r_frame_rate;
    info_.fps    = (fr.num && fr.den) ? av_q2d(fr) : 0.0;
    if (fmt_->duration > 0)
        info_.durationSec = (double)fmt_->duration / AV_TIME_BASE;
    else if (vs->duration > 0)
        info_.durationSec = vs->duration * av_q2d(vs->time_base);
    info_.containerName = fmt_->iformat && fmt_->iformat->name ? fmt_->iformat->name : "";
    info_.videoCodec    = avcodec_get_name(vctx_->codec_id);
    if (actx_) {
        info_.hasAudio        = true;
        info_.audioCodec      = avcodec_get_name(actx_->codec_id);
        info_.audioSampleRate = actx_->sample_rate;
        info_.audioChannels   = actx_->ch_layout.nb_channels > 0
                                  ? actx_->ch_layout.nb_channels : 2;
    }
    std::error_code ec;
    info_.fileSize = (uint64_t)std::filesystem::file_size(path, ec);

    pkt_   = av_packet_alloc();
    frame_ = av_frame_alloc();
    if (!pkt_ || !frame_) { Close(); return Result<void>::Err("av_packet/frame_alloc failed"); }

    if (actx_) DecodeAllAudio();   // also seeks back to start for video

    LOG_INFO("VideoFile: opened {} — {}x{} {:.2f}fps {:.1f}s, video={}, audio={} ({} Hz x{})",
             path.filename().string(), info_.width, info_.height, info_.fps,
             info_.durationSec, info_.videoCodec,
             info_.hasAudio ? info_.audioCodec : std::string("none"),
             info_.audioSampleRate, info_.audioChannels);
    return Result<void>::Ok();
}

void VideoFile::DecodeAllAudio() {
    audioPCM_.clear();
    if (!actx_) return;

    const int outRate = info_.audioSampleRate;
    const int outCh   = info_.audioChannels;

    AVChannelLayout outLayout;
    av_channel_layout_default(&outLayout, outCh);
    AVChannelLayout inLayout = actx_->ch_layout;
    if (inLayout.nb_channels == 0) av_channel_layout_default(&inLayout, outCh);

    if (swr_alloc_set_opts2(&swr_, &outLayout, AV_SAMPLE_FMT_S16, outRate,
                            &inLayout, actx_->sample_fmt, actx_->sample_rate,
                            0, nullptr) < 0 || !swr_ || swr_init(swr_) < 0) {
        LOG_WARN("VideoFile: audio resampler init failed; muting");
        if (swr_) swr_free(&swr_);
        info_.hasAudio = false;
        return;
    }

    AVFrame* af = av_frame_alloc();
    if (!af) {
        LOG_WARN("VideoFile: out of memory allocating audio frame; muting");
        if (swr_) swr_free(&swr_);
        info_.hasAudio = false;
        return;
    }
    auto drain = [&]() {
        while (avcodec_receive_frame(actx_, af) == 0) {
            const int maxOut = (int)av_rescale_rnd(af->nb_samples, outRate,
                                                   actx_->sample_rate, AV_ROUND_UP) + 256;
            const size_t base = audioPCM_.size();
            audioPCM_.resize(base + (size_t)maxOut * outCh);
            uint8_t* outPlanes[1] = { reinterpret_cast<uint8_t*>(audioPCM_.data() + base) };
            const int got = swr_convert(swr_, outPlanes, maxOut,
                                        (const uint8_t**)af->data, af->nb_samples);
            if (got > 0) audioPCM_.resize(base + (size_t)got * outCh);
            else         audioPCM_.resize(base);
            av_frame_unref(af);
        }
    };

    while (av_read_frame(fmt_, pkt_) >= 0) {
        if (pkt_->stream_index == astream_) {
            if (avcodec_send_packet(actx_, pkt_) == 0) drain();
        }
        av_packet_unref(pkt_);
    }
    avcodec_send_packet(actx_, nullptr); // flush
    drain();
    av_frame_free(&af);

    // Rewind to the start so video decoding begins from frame 0.
    av_seek_frame(fmt_, -1, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(vctx_);
    avcodec_flush_buffers(actx_);
}

bool VideoFile::NextFrame(std::vector<uint8_t>& rgba, double& ptsSec) {
    if (!vctx_) return false;

    for (;;) {
        int r = avcodec_receive_frame(vctx_, frame_);
        if (r == 0) {
            // ── Convert decoded frame to RGBA ─────────────────────────────────
            if (!sws_) {
                sws_ = sws_getContext(frame_->width, frame_->height,
                                      (AVPixelFormat)frame_->format,
                                      info_.width, info_.height, AV_PIX_FMT_RGBA,
                                      SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (!sws_) { av_frame_unref(frame_); return false; }
            }
            rgba.assign((size_t)info_.width * info_.height * 4, 0);
            uint8_t* dst[4]      = { rgba.data(), nullptr, nullptr, nullptr };
            int      dstStride[4]= { info_.width * 4, 0, 0, 0 };
            sws_scale(sws_, frame_->data, frame_->linesize, 0, frame_->height,
                      dst, dstStride);

            int64_t pts = frame_->best_effort_timestamp;
            if (pts == AV_NOPTS_VALUE) pts = frame_->pts;
            ptsSec = (pts == AV_NOPTS_VALUE)
                       ? 0.0
                       : pts * av_q2d(fmt_->streams[vstream_]->time_base);
            av_frame_unref(frame_);
            return true;
        }
        if (r == AVERROR_EOF) return false;
        if (r != AVERROR(EAGAIN)) return false;

        // Decoder needs more input.
        if (sentEof_) {
            // Already flushed and still EAGAIN should not happen, but guard.
            return false;
        }
        int rr = av_read_frame(fmt_, pkt_);
        if (rr < 0) {
            avcodec_send_packet(vctx_, nullptr); // enter draining mode
            sentEof_ = true;
            continue;
        }
        if (pkt_->stream_index == vstream_)
            avcodec_send_packet(vctx_, pkt_);
        av_packet_unref(pkt_);
    }
}

void VideoFile::Restart() {
    if (!fmt_ || !vctx_) return;
    av_seek_frame(fmt_, -1, 0, AVSEEK_FLAG_BACKWARD);
    avcodec_flush_buffers(vctx_);
    if (actx_) avcodec_flush_buffers(actx_);
    sentEof_ = false;
}

void VideoFile::Close() {
    if (sws_)   { sws_freeContext(sws_); sws_ = nullptr; }
    if (swr_)   { swr_free(&swr_); swr_ = nullptr; }
    if (frame_) { av_frame_free(&frame_); frame_ = nullptr; }
    if (pkt_)   { av_packet_free(&pkt_); pkt_ = nullptr; }
    if (vctx_)  { avcodec_free_context(&vctx_); vctx_ = nullptr; }
    if (actx_)  { avcodec_free_context(&actx_); actx_ = nullptr; }
    if (fmt_)   { avformat_close_input(&fmt_); fmt_ = nullptr; }
    audioPCM_.clear();
    vstream_ = astream_ = -1;
    sentEof_ = false;
    info_ = {};
}

// ─── ExportMP4 ────────────────────────────────────────────────────────────────
// Transcode an EA Multimedia / VP6 file to a standard H.264 + AAC MP4.
// Strategy: open source with the EA demuxer, decode video→YUV420p frames and
// audio→S16, re-encode video with libx264 and audio with aac, mux into MP4.

Result<void> VideoFile::ExportMP4(const std::filesystem::path& srcVp6,
                                   const std::filesystem::path& dstMp4)
{
    const std::string srcStr = srcVp6.string();
    const std::string dstStr = dstMp4.string();

    // ── Open source ───────────────────────────────────────────────────────────
    AVFormatContext* inFmt = nullptr;
    if (avformat_open_input(&inFmt, srcStr.c_str(), nullptr, nullptr) < 0)
        return Result<void>::Err("ExportMP4: cannot open '" + srcStr + "'");
    if (avformat_find_stream_info(inFmt, nullptr) < 0) {
        avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: cannot read stream info");
    }

    int inVidIdx = av_find_best_stream(inFmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int inAudIdx = av_find_best_stream(inFmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (inVidIdx < 0) {
        avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: no video stream");
    }

    // ── Open decoders ─────────────────────────────────────────────────────────
    auto vdecRes = OpenCodec(inFmt, inVidIdx);
    if (!vdecRes) { avformat_close_input(&inFmt); return Result<void>::Err(vdecRes.error); }
    AVCodecContext* vdec = vdecRes.value;

    AVCodecContext* adec = nullptr;
    if (inAudIdx >= 0) {
        auto adecRes = OpenCodec(inFmt, inAudIdx);
        if (adecRes) adec = adecRes.value;
        else         inAudIdx = -1;
    }

    const AVStream* inVS = inFmt->streams[inVidIdx];
    const int srcW = vdec->width, srcH = vdec->height;
    AVRational srcFps = inVS->avg_frame_rate.num ? inVS->avg_frame_rate : inVS->r_frame_rate;
    if (!srcFps.num || !srcFps.den) srcFps = AVRational{30, 1};

    // ── Open output (MP4 muxer) ───────────────────────────────────────────────
    AVFormatContext* outFmt = nullptr;
    if (avformat_alloc_output_context2(&outFmt, nullptr, "mp4", dstStr.c_str()) < 0 || !outFmt) {
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: cannot alloc MP4 output context");
    }

    // ── Video encode stream (libx264) ─────────────────────────────────────────
    const AVCodec* venc = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!venc) {
        avformat_free_context(outFmt); avcodec_free_context(&vdec);
        if (adec) avcodec_free_context(&adec); avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: libx264 encoder not available");
    }
    AVStream* outVS = avformat_new_stream(outFmt, nullptr);
    AVCodecContext* venc_ctx = avcodec_alloc_context3(venc);
    if (!outVS || !venc_ctx) {
        if (venc_ctx) avcodec_free_context(&venc_ctx);
        avformat_free_context(outFmt); avcodec_free_context(&vdec);
        if (adec) avcodec_free_context(&adec); avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: failed to allocate output video stream/context");
    }
    venc_ctx->width     = srcW;
    venc_ctx->height    = srcH;
    venc_ctx->pix_fmt   = AV_PIX_FMT_YUV420P;
    venc_ctx->time_base = av_inv_q(srcFps);
    venc_ctx->framerate = srcFps;
    venc_ctx->bit_rate  = 2000000;
    venc_ctx->gop_size  = 12;
    if (outFmt->oformat->flags & AVFMT_GLOBALHEADER)
        venc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    AVDictionary* vopt = nullptr;
    av_dict_set(&vopt, "preset", "medium", 0);
    av_dict_set(&vopt, "crf", "23", 0);
    if (avcodec_open2(venc_ctx, venc, &vopt) < 0) {
        av_dict_free(&vopt);
        avformat_free_context(outFmt); avcodec_free_context(&vdec); avcodec_free_context(&venc_ctx);
        if (adec) avcodec_free_context(&adec); avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: could not open x264 encoder");
    }
    av_dict_free(&vopt);
    avcodec_parameters_from_context(outVS->codecpar, venc_ctx);
    outVS->time_base = venc_ctx->time_base;

    // ── Audio encode stream (AAC) ─────────────────────────────────────────────
    AVCodecContext* aenc_ctx = nullptr;
    AVStream* outAS = nullptr;
    SwrContext* exportSwr = nullptr;
    if (adec && inAudIdx >= 0) {
        const AVCodec* aenc = avcodec_find_encoder(AV_CODEC_ID_AAC);
        if (aenc) {
            outAS    = avformat_new_stream(outFmt, nullptr);
            aenc_ctx = avcodec_alloc_context3(aenc);
            if (!outAS || !aenc_ctx) {
                if (aenc_ctx) avcodec_free_context(&aenc_ctx);
                outAS = nullptr;
                goto skip_audio_encoder_setup;
            }
            aenc_ctx->sample_rate = adec->sample_rate > 0 ? adec->sample_rate : 44100;
            aenc_ctx->bit_rate    = 128000;
            aenc_ctx->sample_fmt  = AV_SAMPLE_FMT_FLTP;  // AAC wants planar float
            av_channel_layout_default(&aenc_ctx->ch_layout,
                                       adec->ch_layout.nb_channels > 0
                                       ? adec->ch_layout.nb_channels : 2);
            if (outFmt->oformat->flags & AVFMT_GLOBALHEADER)
                aenc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
            if (avcodec_open2(aenc_ctx, aenc, nullptr) < 0) {
                avcodec_free_context(&aenc_ctx); outAS = nullptr;
            } else {
                avcodec_parameters_from_context(outAS->codecpar, aenc_ctx);
                outAS->time_base = AVRational{1, aenc_ctx->sample_rate};
                // Build a resampler: decoded format → FLTP for AAC
                const int outCh = aenc_ctx->ch_layout.nb_channels;
                AVChannelLayout inLayout = adec->ch_layout;
                if (inLayout.nb_channels == 0) av_channel_layout_default(&inLayout, outCh);
                swr_alloc_set_opts2(&exportSwr, &aenc_ctx->ch_layout,
                                    AV_SAMPLE_FMT_FLTP, aenc_ctx->sample_rate,
                                    &inLayout, adec->sample_fmt, adec->sample_rate,
                                    0, nullptr);
                if (!exportSwr || swr_init(exportSwr) < 0) {
                    if (exportSwr) swr_free(&exportSwr); exportSwr = nullptr;
                    avcodec_free_context(&aenc_ctx); outAS = nullptr;
                }
            }
        }
    }
skip_audio_encoder_setup:;

    // ── Open output file for writing ──────────────────────────────────────────
    if (!(outFmt->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&outFmt->pb, dstStr.c_str(), AVIO_FLAG_WRITE) < 0) {
            avcodec_free_context(&venc_ctx); if (aenc_ctx) avcodec_free_context(&aenc_ctx);
            if (exportSwr) swr_free(&exportSwr);
            avformat_free_context(outFmt); avcodec_free_context(&vdec);
            if (adec) avcodec_free_context(&adec); avformat_close_input(&inFmt);
            return Result<void>::Err("ExportMP4: cannot open output file '" + dstStr + "'");
        }
    }
    if (avformat_write_header(outFmt, nullptr) < 0) {
        avio_closep(&outFmt->pb); avcodec_free_context(&venc_ctx);
        if (aenc_ctx) avcodec_free_context(&aenc_ctx); if (exportSwr) swr_free(&exportSwr);
        avformat_free_context(outFmt); avcodec_free_context(&vdec);
        if (adec) avcodec_free_context(&adec); avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: avformat_write_header failed");
    }

    // ── Conversion contexts ───────────────────────────────────────────────────
    SwsContext* exportSws = sws_getContext(srcW, srcH,
                                           vdec->pix_fmt, srcW, srcH,
                                           AV_PIX_FMT_YUV420P, SWS_BILINEAR,
                                           nullptr, nullptr, nullptr);
    AVPacket* rpkt  = av_packet_alloc();
    AVFrame*  rframe = av_frame_alloc();
    AVFrame*  encFrame = av_frame_alloc();
    AVFrame* aFrame   = av_frame_alloc();   // raw decoded audio
    AVFrame* encAFrame = av_frame_alloc();  // fltp for aac
    if (!rpkt || !rframe || !encFrame || !aFrame || !encAFrame) {
        if (rpkt) av_packet_free(&rpkt);
        if (rframe) av_frame_free(&rframe);
        if (encFrame) av_frame_free(&encFrame);
        if (aFrame) av_frame_free(&aFrame);
        if (encAFrame) av_frame_free(&encAFrame);
        if (exportSws) sws_freeContext(exportSws);
        avio_closep(&outFmt->pb); avcodec_free_context(&venc_ctx);
        if (aenc_ctx) avcodec_free_context(&aenc_ctx); if (exportSwr) swr_free(&exportSwr);
        avformat_free_context(outFmt); avcodec_free_context(&vdec);
        if (adec) avcodec_free_context(&adec); avformat_close_input(&inFmt);
        return Result<void>::Err("ExportMP4: out of memory allocating frame/packet buffers");
    }
    encFrame->format = AV_PIX_FMT_YUV420P;
    encFrame->width  = srcW;
    encFrame->height = srcH;
    av_frame_get_buffer(encFrame, 0);

    int64_t vidPts = 0, audPts = 0;
    bool    vidEof = false;

    auto flushEncoder = [&](AVCodecContext* enc, AVStream* st) {
        avcodec_send_frame(enc, nullptr);
        AVPacket* fp = av_packet_alloc();
        if (!fp) return;
        while (avcodec_receive_packet(enc, fp) == 0) {
            av_packet_rescale_ts(fp, enc->time_base, st->time_base);
            fp->stream_index = st->index;
            av_interleaved_write_frame(outFmt, fp);
            av_packet_unref(fp);
        }
        av_packet_free(&fp);
    };

    auto sendVideoFrame = [&](AVFrame* f) {
        if (exportSws)
            sws_scale(exportSws, f->data, f->linesize, 0, f->height,
                      encFrame->data, encFrame->linesize);
        encFrame->pts = vidPts++;
        avcodec_send_frame(venc_ctx, encFrame);
        AVPacket* ep = av_packet_alloc();
        if (!ep) return;
        while (avcodec_receive_packet(venc_ctx, ep) == 0) {
            av_packet_rescale_ts(ep, venc_ctx->time_base, outVS->time_base);
            ep->stream_index = outVS->index;
            av_interleaved_write_frame(outFmt, ep);
            av_packet_unref(ep);
        }
        av_packet_free(&ep);
    };

    auto sendAudioFrame = [&](AVFrame* f) {
        if (!aenc_ctx || !outAS || !exportSwr) return;
        // Resample decoded audio → FLTP
        const int outSamples = aenc_ctx->frame_size > 0 ? aenc_ctx->frame_size : f->nb_samples;
        encAFrame->format = AV_SAMPLE_FMT_FLTP;
        encAFrame->nb_samples = outSamples;
        av_channel_layout_copy(&encAFrame->ch_layout, &aenc_ctx->ch_layout);
        av_frame_get_buffer(encAFrame, 0);
        int got = swr_convert(exportSwr,
                              encAFrame->data, outSamples,
                              (const uint8_t**)f->data, f->nb_samples);
        if (got > 0) {
            encAFrame->nb_samples = got;
            encAFrame->pts = audPts;
            audPts += got;
            avcodec_send_frame(aenc_ctx, encAFrame);
            AVPacket* ep = av_packet_alloc();
            if (!ep) return;
            while (avcodec_receive_packet(aenc_ctx, ep) == 0) {
                av_packet_rescale_ts(ep, aenc_ctx->time_base, outAS->time_base);
                ep->stream_index = outAS->index;
                av_interleaved_write_frame(outFmt, ep);
                av_packet_unref(ep);
            }
            av_packet_free(&ep);
        }
        av_frame_unref(encAFrame);
    };

    // ── Main transcode loop ───────────────────────────────────────────────────
    while (!vidEof) {
        int r = av_read_frame(inFmt, rpkt);
        if (r < 0) {
            // Flush video decoder
            avcodec_send_packet(vdec, nullptr);
            while (avcodec_receive_frame(vdec, rframe) == 0) {
                sendVideoFrame(rframe); av_frame_unref(rframe);
            }
            vidEof = true;
            break;
        }
        if (rpkt->stream_index == inVidIdx) {
            avcodec_send_packet(vdec, rpkt);
            while (avcodec_receive_frame(vdec, rframe) == 0) {
                sendVideoFrame(rframe); av_frame_unref(rframe);
            }
        } else if (rpkt->stream_index == inAudIdx && adec) {
            avcodec_send_packet(adec, rpkt);
            while (avcodec_receive_frame(adec, aFrame) == 0) {
                sendAudioFrame(aFrame); av_frame_unref(aFrame);
            }
        }
        av_packet_unref(rpkt);
    }

    // ── Flush encoders ────────────────────────────────────────────────────────
    flushEncoder(venc_ctx, outVS);
    if (aenc_ctx && outAS) flushEncoder(aenc_ctx, outAS);

    av_write_trailer(outFmt);

    // ── Cleanup ───────────────────────────────────────────────────────────────
    av_frame_free(&rframe); av_frame_free(&encFrame);
    av_frame_free(&aFrame); av_frame_free(&encAFrame);
    av_packet_free(&rpkt);
    if (exportSws) sws_freeContext(exportSws);
    if (exportSwr) swr_free(&exportSwr);
    avcodec_free_context(&venc_ctx);
    if (aenc_ctx) avcodec_free_context(&aenc_ctx);
    avcodec_free_context(&vdec);
    if (adec) avcodec_free_context(&adec);
    if (!(outFmt->oformat->flags & AVFMT_NOFILE)) avio_closep(&outFmt->pb);
    avformat_free_context(outFmt);
    avformat_close_input(&inFmt);

    LOG_INFO("VideoFile::ExportMP4: wrote {}", dstStr);
    return Result<void>::Ok();
}

// ─── EA MVP container writer ──────────────────────────────────────────────────
// Writes the EA Multimedia / MVP container that NFSMW's movie player expects.
// Format (all multi-byte values little-endian):
//
//   [MVhd][u32 blockSize]  movie header with field-tagged metadata
//   Per frame:
//     [MV0K][u32 sz][VP6F bitstream]   keyframe
//     [MV0F][u32 sz][VP6F bitstream]   delta frame
//     [SCDl][u32 sz][s16le PCM]        audio for this frame (if any)
//   [SCEl][u32=8]                      end sentinel
//
// blockSize includes the 8-byte fourcc+size header.
// ─────────────────────────────────────────────────────────────────────────────
namespace {

static void WriteU32LE(std::ostream& os, uint32_t v) {
    uint8_t b[4] = { uint8_t(v), uint8_t(v>>8), uint8_t(v>>16), uint8_t(v>>24) };
    os.write(reinterpret_cast<const char*>(b), 4);
}
static void WriteChunkHeader(std::ostream& os, const char* fourcc, uint32_t payloadSize) {
    os.write(fourcc, 4);
    WriteU32LE(os, payloadSize + 8); // size includes the 8-byte header itself
}

/// Write MVhd header block.
static void WriteMVhd(std::ostream& os, int w, int h,
                      AVRational fps, uint32_t frameCount,
                      int audioRate, int audioCh) {
    // Build payload into a buffer first so we know its size.
    std::vector<uint8_t> payload;
    auto push8  = [&](uint8_t v)  { payload.push_back(v); };
    auto push32 = [&](uint32_t v) {
        payload.push_back(v & 0xFF); payload.push_back((v>>8)&0xFF);
        payload.push_back((v>>16)&0xFF); payload.push_back((v>>24)&0xFF);
    };
    // Field 0x00: video stream id (1)
    push8(0x00); push32(1);
    // Field 0x04: width
    push8(0x04); push32((uint32_t)w);
    // Field 0x05: height
    push8(0x05); push32((uint32_t)h);
    // Field 0x06: fps as u32 fixed-point (fps * 0x10000, matching FFmpeg ea demuxer)
    uint32_t fpsFixed = (fps.num && fps.den)
        ? (uint32_t)(((double)fps.num / fps.den) * 0x10000 + 0.5)
        : (uint32_t)(30.0 * 0x10000);
    push8(0x06); push32(fpsFixed);
    // Field 0x07: frame count
    push8(0x07); push32(frameCount);
    if (audioRate > 0 && audioCh > 0) {
        // Field 0x08: audio stream id (2)
        push8(0x08); push32(2);
        // Field 0x0A: sample rate
        push8(0x0A); push32((uint32_t)audioRate);
        // Field 0x0C: channels
        push8(0x0C); push32((uint32_t)audioCh);
        // Field 0x0D: bits per sample (16)
        push8(0x0D); push32(16);
    }
    // End marker
    push8(0xFF);
    // Pad to 4-byte alignment
    while (payload.size() % 4) payload.push_back(0);

    WriteChunkHeader(os, "MVhd", (uint32_t)payload.size());
    os.write(reinterpret_cast<const char*>(payload.data()), payload.size());
}

} // namespace (EA container helpers)

// ─── ImportMP4 ────────────────────────────────────────────────────────────────
// Transcode a standard MP4 (any FFmpeg-readable codec) back to an EA Multimedia
// container with VP6F video + PCM S16LE audio, compatible with the NFSMW
// in-game movie player.
//
// Strategy:
//   Pass 1 – Decode the source MP4 (video + audio).
//             Re-encode video to VP6 via FFmpeg's FLV muxer into a temp file.
//             FFmpeg's FLV muxer uses the native "flashsv/flv" VP6 encoder
//             (AV_CODEC_ID_FLV1 is Sorenson, but VP6 is supported via the
//             "flv" muxer's codec mapping).  We actually use the FLV muxer with
//             AV_CODEC_ID_VP6F so that the muxer handles the per-packet FLV
//             framing, then we demux those packets straight back out and strip
//             the 1-byte FLV VP6 alpha header to get a clean VP6 bitstream.
//
//             Concretely: encode MP4 → temp FLV (vp6f codec, flv muxer),
//             then read the temp FLV back packet-by-packet, stripping the
//             1-byte FLV video-tag composition offset, and write each stripped
//             packet as MV0K or MV0F into the EA MVP container.
//
//   Pass 2 – Write EA MVP container (MVhd + per-frame MV0K/MV0F + SCDl chunks).

Result<void> VideoFile::ImportMP4(const std::filesystem::path& srcMp4,
                                   const std::filesystem::path& dstVp6,
                                   int    targetWidth,
                                   int    targetHeight,
                                   double targetFps)
{
    const std::string srcStr = srcMp4.string();
    const std::string dstStr = dstVp6.string();

    // ── Temp FLV path ─────────────────────────────────────────────────────────
    // We encode into a temporary .flv file sitting next to the destination,
    // then parse its VP6 packets out to build the EA MVP container.
    const std::filesystem::path tmpFlv =
        dstVp6.parent_path() / (dstVp6.stem().string() + "_mwtmp.flv");
    const std::string tmpFlvStr = tmpFlv.string();
    // Ensure cleanup on any exit path
    struct TmpGuard {
        const std::string& path;
        ~TmpGuard() { std::error_code ec; std::filesystem::remove(std::filesystem::path(path), ec); }
    } tmpGuard{tmpFlvStr};

    // ── Open source MP4 ───────────────────────────────────────────────────────
    AVFormatContext* inFmt = nullptr;
    if (avformat_open_input(&inFmt, srcStr.c_str(), nullptr, nullptr) < 0)
        return Result<void>::Err("ImportMP4: cannot open '" + srcStr + "'");
    if (avformat_find_stream_info(inFmt, nullptr) < 0) {
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: cannot read stream info");
    }

    int inVidIdx = av_find_best_stream(inFmt, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int inAudIdx = av_find_best_stream(inFmt, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (inVidIdx < 0) {
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: no video stream in source");
    }

    auto vdecRes = OpenCodec(inFmt, inVidIdx);
    if (!vdecRes) { avformat_close_input(&inFmt); return Result<void>::Err(vdecRes.error); }
    AVCodecContext* vdec = vdecRes.value;

    AVCodecContext* adec = nullptr;
    if (inAudIdx >= 0) {
        auto adecRes = OpenCodec(inFmt, inAudIdx);
        if (adecRes) adec = adecRes.value;
        else         inAudIdx = -1;
    }

    // Resolve target resolution / frame rate
    const AVStream* inVS = inFmt->streams[inVidIdx];
    const int outW = (targetWidth  > 0) ? targetWidth  : vdec->width;
    const int outH = (targetHeight > 0) ? targetHeight : vdec->height;
    AVRational srcFps = inVS->avg_frame_rate.num ? inVS->avg_frame_rate : inVS->r_frame_rate;
    if (!srcFps.num || !srcFps.den) srcFps = AVRational{30, 1};
    AVRational outFpsR = (targetFps > 0.0) ? av_d2q(targetFps, 1000) : srcFps;

    // ── Open FLV output context ───────────────────────────────────────────────
    // The FLV muxer supports VP6F natively and handles per-packet framing.
    AVFormatContext* flvFmt = nullptr;
    if (avformat_alloc_output_context2(&flvFmt, nullptr, "flv", tmpFlvStr.c_str()) < 0 || !flvFmt) {
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: cannot alloc FLV output context");
    }

    // VP6F encoder — try both IDs; FLV muxer accepts VP6F
    const AVCodec* vp6enc = avcodec_find_encoder_by_name("vp6f");
    if (!vp6enc) vp6enc = avcodec_find_encoder(AV_CODEC_ID_VP6F);
    if (!vp6enc) vp6enc = avcodec_find_encoder_by_name("vp6");
    if (!vp6enc) vp6enc = avcodec_find_encoder(AV_CODEC_ID_VP6);
    if (!vp6enc) {
        avformat_free_context(flvFmt);
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: no VP6/VP6F encoder found in this FFmpeg build. "
                                 "Install a full FFmpeg (gyan.dev or BtbN/FFmpeg-Builds).");
    }

    // Pick pixel format from the encoder's supported list. FFmpeg 7.1+ replaced
    // the deprecated AVCodec::pix_fmts field with avcodec_get_supported_config().
    AVPixelFormat encPixFmt = AV_PIX_FMT_YUV410P;
    {
        const enum AVPixelFormat* supported = nullptr;
        if (avcodec_get_supported_config(nullptr, vp6enc, AV_CODEC_CONFIG_PIX_FORMAT,
                0, reinterpret_cast<const void**>(&supported), nullptr) >= 0 &&
            supported)
            encPixFmt = supported[0];
    }

    AVStream*       flvVS   = avformat_new_stream(flvFmt, nullptr);
    AVCodecContext* venc     = avcodec_alloc_context3(vp6enc);
    if (!flvVS || !venc) {
        if (venc) avcodec_free_context(&venc);
        avformat_free_context(flvFmt);
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: failed to allocate output video stream/context");
    }
    venc->width        = outW;
    venc->height       = outH;
    venc->pix_fmt      = encPixFmt;
    venc->time_base    = av_inv_q(outFpsR);
    venc->framerate    = outFpsR;
    venc->bit_rate     = 1500000;
    venc->gop_size     = 12;
    venc->max_b_frames = 0;
    if (flvFmt->oformat->flags & AVFMT_GLOBALHEADER)
        venc->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    if (avcodec_open2(venc, vp6enc, nullptr) < 0) {
        avcodec_free_context(&venc); avformat_free_context(flvFmt);
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: could not open VP6 encoder");
    }
    avcodec_parameters_from_context(flvVS->codecpar, venc);
    flvVS->time_base = venc->time_base;

    // Audio output stream in FLV (PCM S16 — FLV supports it)
    const int outAudioRate = adec ? (adec->sample_rate > 0 ? adec->sample_rate : 44100) : 0;
    const int outAudioCh   = adec ? (adec->ch_layout.nb_channels > 0
                                     ? adec->ch_layout.nb_channels : 2) : 0;
    AVStream*       flvAS   = nullptr;
    AVCodecContext* aenc    = nullptr;
    if (adec && inAudIdx >= 0) {
        const AVCodec* pcmenc = avcodec_find_encoder(AV_CODEC_ID_PCM_S16LE);
        if (pcmenc) {
            flvAS = avformat_new_stream(flvFmt, nullptr);
            aenc  = avcodec_alloc_context3(pcmenc);
            if (!flvAS || !aenc) {
                if (aenc) avcodec_free_context(&aenc);
                aenc = nullptr; flvAS = nullptr;
                goto skip_audio_encoder_setup_flv;
            }
            aenc->sample_fmt  = AV_SAMPLE_FMT_S16;
            aenc->sample_rate = outAudioRate;
            av_channel_layout_default(&aenc->ch_layout, outAudioCh);
            aenc->time_base   = AVRational{1, outAudioRate};
            if (avcodec_open2(aenc, pcmenc, nullptr) < 0) {
                avcodec_free_context(&aenc); aenc = nullptr;
                avformat_free_context(flvFmt); // recreate without audio stream — simpler: just skip
                // Realloc without audio
                avformat_alloc_output_context2(&flvFmt, nullptr, "flv", tmpFlvStr.c_str());
                flvVS = avformat_new_stream(flvFmt, nullptr);
                avcodec_parameters_from_context(flvVS->codecpar, venc);
                flvVS->time_base = venc->time_base;
            } else {
                avcodec_parameters_from_context(flvAS->codecpar, aenc);
                flvAS->time_base = aenc->time_base;
            }
        }
    }
skip_audio_encoder_setup_flv:;

    if (avio_open(&flvFmt->pb, tmpFlvStr.c_str(), AVIO_FLAG_WRITE) < 0) {
        avcodec_free_context(&venc); if (aenc) avcodec_free_context(&aenc);
        avformat_free_context(flvFmt);
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: cannot open temp FLV for writing");
    }
    if (avformat_write_header(flvFmt, nullptr) < 0) {
        avio_closep(&flvFmt->pb);
        avcodec_free_context(&venc); if (aenc) avcodec_free_context(&aenc);
        avformat_free_context(flvFmt);
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: cannot write FLV header");
    }

    // ── Audio resampler (src → S16) ───────────────────────────────────────────
    SwrContext* impSwr = nullptr;
    if (adec && inAudIdx >= 0) {
        AVChannelLayout inLayout = adec->ch_layout;
        if (inLayout.nb_channels == 0) av_channel_layout_default(&inLayout, outAudioCh);
        AVChannelLayout outLayout;
        av_channel_layout_default(&outLayout, outAudioCh);
        swr_alloc_set_opts2(&impSwr, &outLayout, AV_SAMPLE_FMT_S16, outAudioRate,
                            &inLayout, adec->sample_fmt, adec->sample_rate, 0, nullptr);
        if (!impSwr || swr_init(impSwr) < 0) {
            if (impSwr) swr_free(&impSwr); impSwr = nullptr;
            inAudIdx = -1;
        }
    }

    // ── Scaler (src pixel fmt → encPixFmt) ───────────────────────────────────
    SwsContext* impSws = sws_getContext(vdec->width, vdec->height, vdec->pix_fmt,
                                        outW, outH, encPixFmt, SWS_BILINEAR,
                                        nullptr, nullptr, nullptr);

    AVPacket* rpkt   = av_packet_alloc();
    AVFrame*  rframe = av_frame_alloc();
    AVFrame*  encVf  = av_frame_alloc();
    AVFrame*  aFrame = av_frame_alloc();
    if (!rpkt || !rframe || !encVf || !aFrame) {
        if (rpkt) av_packet_free(&rpkt);
        if (rframe) av_frame_free(&rframe);
        if (encVf) av_frame_free(&encVf);
        if (aFrame) av_frame_free(&aFrame);
        if (impSwr) swr_free(&impSwr);
        if (impSws) sws_freeContext(impSws);
        avio_closep(&flvFmt->pb);
        avcodec_free_context(&venc); if (aenc) avcodec_free_context(&aenc);
        avformat_free_context(flvFmt);
        avcodec_free_context(&vdec); if (adec) avcodec_free_context(&adec);
        avformat_close_input(&inFmt);
        return Result<void>::Err("ImportMP4: out of memory allocating frame/packet buffers");
    }
    encVf->format = encPixFmt;
    encVf->width  = outW;
    encVf->height = outH;
    av_frame_get_buffer(encVf, 0);

    int64_t videoPts = 0;
    int64_t audioPts = 0;

    auto flushVideoEnc = [&]() {
        AVPacket* ep = av_packet_alloc();
        if (!ep) return;
        while (avcodec_receive_packet(venc, ep) == 0) {
            ep->stream_index = flvVS->index;
            av_packet_rescale_ts(ep, venc->time_base, flvVS->time_base);
            av_interleaved_write_frame(flvFmt, ep);
            av_packet_unref(ep);
        }
        av_packet_free(&ep);
    };

    auto sendVideoFrame = [&](AVFrame* f) {
        if (impSws)
            sws_scale(impSws, f->data, f->linesize, 0, f->height,
                      encVf->data, encVf->linesize);
        encVf->pts = videoPts++;
        avcodec_send_frame(venc, encVf);
        flushVideoEnc();
    };

    auto flushAudioDec = [&]() {
        if (!adec || !impSwr || !aenc || !flvAS) return;
        while (avcodec_receive_frame(adec, aFrame) == 0) {
            // Resample to S16
            const int maxOut = (int)av_rescale_rnd(
                swr_get_delay(impSwr, adec->sample_rate) + aFrame->nb_samples,
                outAudioRate, adec->sample_rate, AV_ROUND_UP) + 16;
            AVFrame* outAf = av_frame_alloc();
            if (!outAf) { av_frame_unref(aFrame); continue; }
            outAf->format         = AV_SAMPLE_FMT_S16;
            outAf->sample_rate    = outAudioRate;
            av_channel_layout_default(&outAf->ch_layout, outAudioCh);
            outAf->nb_samples     = maxOut;
            av_frame_get_buffer(outAf, 0);
            int got = swr_convert(impSwr,
                                  outAf->data, maxOut,
                                  (const uint8_t**)aFrame->data, aFrame->nb_samples);
            if (got > 0) {
                outAf->nb_samples = got;
                outAf->pts        = audioPts;
                audioPts += got;
                avcodec_send_frame(aenc, outAf);
                AVPacket* ap = av_packet_alloc();
                if (ap) {
                    while (avcodec_receive_packet(aenc, ap) == 0) {
                        ap->stream_index = flvAS->index;
                        av_packet_rescale_ts(ap, aenc->time_base, flvAS->time_base);
                        av_interleaved_write_frame(flvFmt, ap);
                        av_packet_unref(ap);
                    }
                    av_packet_free(&ap);
                }
            }
            av_frame_free(&outAf);
            av_frame_unref(aFrame);
        }
    };

    // Main decode → encode loop
    while (true) {
        int r = av_read_frame(inFmt, rpkt);
        if (r < 0) {
            // Flush video
            avcodec_send_packet(vdec, nullptr);
            while (avcodec_receive_frame(vdec, rframe) == 0) {
                sendVideoFrame(rframe);
                av_frame_unref(rframe);
            }
            avcodec_send_frame(venc, nullptr);
            flushVideoEnc();
            // Flush audio
            if (adec) { avcodec_send_packet(adec, nullptr); flushAudioDec(); }
            if (aenc) {
                avcodec_send_frame(aenc, nullptr);
                AVPacket* ap = av_packet_alloc();
                if (ap) {
                    while (avcodec_receive_packet(aenc, ap) == 0) {
                        ap->stream_index = flvAS->index;
                        av_packet_rescale_ts(ap, aenc->time_base, flvAS->time_base);
                        av_interleaved_write_frame(flvFmt, ap);
                        av_packet_unref(ap);
                    }
                    av_packet_free(&ap);
                }
            }
            break;
        }
        if (rpkt->stream_index == inVidIdx) {
            avcodec_send_packet(vdec, rpkt);
            while (avcodec_receive_frame(vdec, rframe) == 0) {
                sendVideoFrame(rframe);
                av_frame_unref(rframe);
            }
        } else if (rpkt->stream_index == inAudIdx && adec) {
            avcodec_send_packet(adec, rpkt);
            flushAudioDec();
        }
        av_packet_unref(rpkt);
    }

    av_write_trailer(flvFmt);
    avio_closep(&flvFmt->pb);

    // Cleanup encode/decode
    av_frame_free(&rframe); av_frame_free(&encVf); av_frame_free(&aFrame);
    av_packet_free(&rpkt);
    if (impSws) sws_freeContext(impSws);
    if (impSwr) swr_free(&impSwr);
    avcodec_free_context(&venc);
    if (aenc) avcodec_free_context(&aenc);
    avcodec_free_context(&vdec);
    if (adec) avcodec_free_context(&adec);
    avformat_close_input(&inFmt);
    avformat_free_context(flvFmt);

    // ── Pass 2: demux temp FLV → extract VP6 packets → EA MVP container ───────
    // FLV VP6 video tags have a 1-byte composition-time offset prepended to the
    // raw VP6 bitstream.  Strip that byte before writing MV0K/MV0F chunks.
    AVFormatContext* flvIn = nullptr;
    if (avformat_open_input(&flvIn, tmpFlvStr.c_str(), nullptr, nullptr) < 0)
        return Result<void>::Err("ImportMP4: cannot re-open temp FLV for demux");
    avformat_find_stream_info(flvIn, nullptr);

    int flvVidIdx = av_find_best_stream(flvIn, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    int flvAudIdx = av_find_best_stream(flvIn, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    if (flvVidIdx < 0) {
        avformat_close_input(&flvIn);
        return Result<void>::Err("ImportMP4: no video stream in temp FLV");
    }

    // Samples per video frame for audio interleaving
    const int samplesPerFrame = (outAudioRate > 0 && outFpsR.num > 0)
        ? (int)av_rescale(outAudioRate, outFpsR.den, outFpsR.num) : 0;

    struct EAFrame {
        std::vector<uint8_t> videoData;
        bool                 isKeyframe = false;
        std::vector<int16_t> audioPCM;
    };
    std::vector<EAFrame> frames;
    std::vector<int16_t> audioBuf;

    AVPacket* fpkt = av_packet_alloc();
    if (!fpkt) {
        avformat_close_input(&flvIn);
        return Result<void>::Err("ImportMP4: out of memory allocating packet");
    }
    while (av_read_frame(flvIn, fpkt) >= 0) {
        if (fpkt->stream_index == flvVidIdx && fpkt->size > 1) {
            EAFrame ef;
            ef.isKeyframe = (fpkt->flags & AV_PKT_FLAG_KEY) != 0;
            // Strip the 1-byte FLV VP6 alpha/composition offset header byte
            ef.videoData.assign(fpkt->data + 1, fpkt->data + fpkt->size);
            frames.push_back(std::move(ef));
        } else if (fpkt->stream_index == flvAudIdx && fpkt->size >= 2 && flvAudIdx >= 0) {
            // FLV PCM_S16LE audio tags have a 1-byte FLV audio header; skip it
            const int16_t* samples = reinterpret_cast<const int16_t*>(fpkt->data + 1);
            const int      count   = (fpkt->size - 1) / sizeof(int16_t);
            audioBuf.insert(audioBuf.end(), samples, samples + count);
        }
        av_packet_unref(fpkt);
    }
    av_packet_free(&fpkt);
    avformat_close_input(&flvIn);

    if (frames.empty())
        return Result<void>::Err("ImportMP4: no video frames in transcoded FLV");

    // Distribute audio samples across frames
    if (samplesPerFrame > 0 && !audioBuf.empty()) {
        size_t audioPos = 0;
        for (auto& ef : frames) {
            const size_t want = std::min((size_t)samplesPerFrame * outAudioCh,
                                         audioBuf.size() - audioPos);
            if (want == 0) break;
            ef.audioPCM.assign(audioBuf.data() + audioPos,
                               audioBuf.data() + audioPos + want);
            audioPos += want;
        }
    }

    // ── Pass 3: write EA MVP container ───────────────────────────────────────
    std::ofstream out(dstStr, std::ios::binary | std::ios::trunc);
    if (!out)
        return Result<void>::Err("ImportMP4: cannot open output file '" + dstStr + "'");

    const uint32_t frameCount = (uint32_t)frames.size();
    WriteMVhd(out, outW, outH, outFpsR, frameCount, outAudioRate, outAudioCh);

    for (const auto& ef : frames) {
        const char* vFourcc = ef.isKeyframe ? "MV0K" : "MV0F";
        WriteChunkHeader(out, vFourcc, (uint32_t)ef.videoData.size());
        out.write(reinterpret_cast<const char*>(ef.videoData.data()),
                  ef.videoData.size());

        if (!ef.audioPCM.empty()) {
            const uint32_t audioBytes = (uint32_t)(ef.audioPCM.size() * sizeof(int16_t));
            WriteChunkHeader(out, "SCDl", audioBytes);
            for (int16_t s : ef.audioPCM) {
                uint8_t b[2] = { uint8_t(s & 0xFF), uint8_t((s >> 8) & 0xFF) };
                out.write(reinterpret_cast<const char*>(b), 2);
            }
        }
    }
    WriteChunkHeader(out, "SCEl", 0);

    if (!out)
        return Result<void>::Err("ImportMP4: write error on '" + dstStr + "'");

    LOG_INFO("VideoFile::ImportMP4: wrote EA MVP container → {} ({} frames, {}x{} @ {:.2f}fps)",
             dstStr, frameCount, outW, outH, outFpsR.num / (double)outFpsR.den);
    return Result<void>::Ok();
}

} // namespace nfsmw
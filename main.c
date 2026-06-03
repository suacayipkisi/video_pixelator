#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

#define LOW_WIDTH 32
#define LOW_HEIGHT 18
#define HIGH_WIDTH 1920
#define HIGH_HEIGHT 1080

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Method of use: %s <input_video> <output_video>\n", argv[0]);
        return -1;
    }

    const char *input_filename = argv[1];
    const char *output_filename = argv[2];

    AVFormatContext *in_format_ctx = NULL;
    if (avformat_open_input(&in_format_ctx, input_filename, NULL, NULL) < 0) {
        fprintf(stderr, "Input video could not be opened.\n");
        return -1;
    }

    if (avformat_find_stream_info(in_format_ctx, NULL) < 0) return -1;

    int video_stream_idx = -1;
    int audio_stream_idx = -1;
    
    for (unsigned int i = 0; i < in_format_ctx->nb_streams; i++) {
        if (in_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && video_stream_idx == -1) {
            video_stream_idx = i;
        } else if (in_format_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && audio_stream_idx == -1) {
            audio_stream_idx = i;
        }
    }

    if (video_stream_idx == -1) {
        fprintf(stderr, "Video stream could not be finded.\n");
        return -1;
    }

    // Input Video Decoder Settings
    AVCodecParameters *in_codec_par = in_format_ctx->streams[video_stream_idx]->codecpar;
    const AVCodec *in_codec = avcodec_find_decoder(in_codec_par->codec_id);
    AVCodecContext *in_codec_ctx = avcodec_alloc_context3(in_codec);
    avcodec_parameters_to_context(in_codec_ctx, in_codec_par);
    avcodec_open2(in_codec_ctx, in_codec, NULL);

    // Output File Setup
    AVFormatContext *out_format_ctx = NULL;
    avformat_alloc_output_context2(&out_format_ctx, NULL, NULL, output_filename);

    // Output Video Stream and Encoder Settings
    const AVCodec *out_codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    AVStream *out_video_stream = avformat_new_stream(out_format_ctx, out_codec);
    AVCodecContext *out_codec_ctx = avcodec_alloc_context3(out_codec);

    out_codec_ctx->width = HIGH_WIDTH;
    out_codec_ctx->height = HIGH_HEIGHT;
    out_codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    
    // Stable time_base configuration
    out_codec_ctx->time_base = (AVRational){1, 90000};
    out_codec_ctx->framerate = av_guess_frame_rate(in_format_ctx, in_format_ctx->streams[video_stream_idx], NULL);

    if (out_format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        out_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    avcodec_open2(out_codec_ctx, out_codec, NULL);
    out_video_stream->time_base = out_codec_ctx->time_base;
    avcodec_parameters_from_context(out_video_stream->codecpar, out_codec_ctx);

    // AUDIO ADDITION: Streaming transcription optimized for Fedora compatibility.
    AVStream *out_audio_stream = NULL;
    if (audio_stream_idx != -1) {
        out_audio_stream = avformat_new_stream(out_format_ctx, NULL);
        avcodec_parameters_copy(out_audio_stream->codecpar, in_format_ctx->streams[audio_stream_idx]->codecpar);
        
        //To prevent Fedora and MP4 container rejection, codec_tag cleanup is necessary.
        uint32_t tags[2] = {0};
        if (av_codec_get_tag2(out_format_ctx->oformat->codec_tag, out_audio_stream->codecpar->codec_id, &tags[0]) > 0) {
            out_audio_stream->codecpar->codec_tag = tags[0];
        } else {
            out_audio_stream->codecpar->codec_tag = 0;
        }
    }

    if (!(out_format_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&out_format_ctx->pb, output_filename, AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Output file could not be opened in write mode.\n");
            return -1;
        }
    }

    if (avformat_write_header(out_format_ctx, NULL) < 0) {
        fprintf(stderr, "Header could not be written. There is a container or codec mismatch.\n");
        return -1;
    }

    // Scaler
    struct SwsContext *sws_to_low = sws_getContext(
        in_codec_ctx->width, in_codec_ctx->height, in_codec_ctx->pix_fmt,
        LOW_WIDTH, LOW_HEIGHT, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL
    );
    struct SwsContext *sws_to_high = sws_getContext(
        LOW_WIDTH, LOW_HEIGHT, AV_PIX_FMT_YUV420P,
        HIGH_WIDTH, HIGH_HEIGHT, AV_PIX_FMT_YUV420P, SWS_POINT, NULL, NULL, NULL
    );

    AVFrame *in_frame = av_frame_alloc();
    AVFrame *low_frame = av_frame_alloc();
    low_frame->format = AV_PIX_FMT_YUV420P;
    low_frame->width = LOW_WIDTH;
    low_frame->height = LOW_HEIGHT;
    av_image_alloc(low_frame->data, low_frame->linesize, LOW_WIDTH, LOW_HEIGHT, AV_PIX_FMT_YUV420P, 32);

    AVFrame *out_frame = av_frame_alloc();
    out_frame->format = out_codec_ctx->pix_fmt;
    out_frame->width = HIGH_WIDTH;
    out_frame->height = HIGH_HEIGHT;
    av_image_alloc(out_frame->data, out_frame->linesize, HIGH_WIDTH, HIGH_HEIGHT, out_codec_ctx->pix_fmt, 32);

    AVPacket *packet = av_packet_alloc();
    AVPacket *out_packet = av_packet_alloc();
    int64_t video_frame_counter = 0;

    while (av_read_frame(in_format_ctx, packet) >= 0) {
        if (packet->stream_index == video_stream_idx) {
            if (avcodec_send_packet(in_codec_ctx, packet) >= 0) {
                while (avcodec_receive_frame(in_codec_ctx, in_frame) >= 0) {
                    sws_scale(sws_to_low, (const uint8_t *const *)in_frame->data, in_frame->linesize, 0,
                              in_codec_ctx->height, low_frame->data, low_frame->linesize);

                    sws_scale(sws_to_high, (const uint8_t *const *)low_frame->data, low_frame->linesize, 0,
                              LOW_HEIGHT, out_frame->data, out_frame->linesize);

                    // Recalculating timestamps consistently.
                    out_frame->pts = av_rescale_q(video_frame_counter++, (AVRational){1, (int)av_q2d(out_codec_ctx->framerate)}, out_codec_ctx->time_base);

                    if (avcodec_send_frame(out_codec_ctx, out_frame) >= 0) {
                        while (avcodec_receive_packet(out_codec_ctx, out_packet) >= 0) {
                            av_packet_rescale_ts(out_packet, out_codec_ctx->time_base, out_video_stream->time_base);
                            out_packet->stream_index = out_video_stream->index;
                            av_interleaved_write_frame(out_format_ctx, out_packet);
                            av_packet_unref(out_packet);
                        }
                    }
                }
            }
        } else if (packet->stream_index == audio_stream_idx && out_audio_stream != NULL) {
            // PTS control to prevent GStreamer synchronization error
            if (packet->pts == AV_NOPTS_VALUE) {
                packet->pts = 0;
                packet->dts = 0;
            }
            av_packet_rescale_ts(packet, in_format_ctx->streams[audio_stream_idx]->time_base, out_audio_stream->time_base);
            packet->stream_index = out_audio_stream->index;
            if (packet->dts > packet->pts) packet->dts = packet->pts;
            av_interleaved_write_frame(out_format_ctx, packet);
        }
        av_packet_unref(packet);
    }

    // Video Encoder Flush
    avcodec_send_frame(out_codec_ctx, NULL);
    while (avcodec_receive_packet(out_codec_ctx, out_packet) >= 0) {
        av_packet_rescale_ts(out_packet, out_codec_ctx->time_base, out_video_stream->time_base);
        out_packet->stream_index = out_video_stream->index;
        av_interleaved_write_frame(out_format_ctx, out_packet);
        av_packet_unref(out_packet);
    }

    av_write_trailer(out_format_ctx);

    // Cleaning
    av_packet_free(&packet);
    av_packet_free(&out_packet);
    av_frame_free(&in_frame);
    av_freep(&low_frame->data[0]);
    av_frame_free(&low_frame);
    av_freep(&out_frame->data[0]);
    av_frame_free(&out_frame);
    sws_freeContext(sws_to_low);
    sws_freeContext(sws_to_high);
    avcodec_free_context(&in_codec_ctx);
    avcodec_free_context(&out_codec_ctx);
    avformat_close_input(&in_format_ctx);
    if (!(out_format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&out_format_ctx->pb);
    }
    avformat_free_context(out_format_ctx);

    return 0;
}
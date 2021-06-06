#pragma once

#define __STDC_CONSTANT_MACROS
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswresample/swresample.h"
#include "libavutil/samplefmt.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"


uint8_t *out_buffer;
int out_buffer_size;
int act_buffer_size;

uint8_t *cur_pos;
int left_len;

void  fill_audio(void* udata, uint8_t* stream, int len) {
    SDL_memset(stream, 0, len);

    if(left_len==0)
            return;
    len= len>left_len ? left_len : len ;
    SDL_MixAudio(stream, cur_pos,len,SDL_MIX_MAXVOLUME);
    cur_pos += len;
    left_len -= len;

    //int n = pSoundBuf->Read((char*)copy_buf, len);
    //SDL_MixAudio(stream, copy_buf, n, SDL_MIX_MAXVOLUME);
}

int start_decode_audio()
{
    char* file_path = "F:/_git_home_/SimplePlayer/input/might_guy.flv";
    //----ffmpeg
    AVFormatContext* pFormatCtx = avformat_alloc_context();

    //open file
    if (avformat_open_input(&pFormatCtx, file_path, NULL, NULL) != 0)
    {
        printf("Fail to open %s\n", file_path);
        return -1;
    }
    //read stream_info
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0)
    {
        printf("Fail to find_stream_info \n");
        return -1;
    }
    printf("---------------File Information------------------\n");
    av_dump_format(pFormatCtx, 0, file_path, 0);
    printf("-------------------------------------------------\n");

    //find audio stream index
    int aud_idx = -1;
    printf("pFormatCtx->nb_streams = %d\n", pFormatCtx->nb_streams);
    for (int i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            aud_idx = i;
            printf("find audio index : %d\n", aud_idx);
            break;
        }
    }
    if (aud_idx == -1)
    {
        printf("Fail to find video stream\n");
        return -1;
    }
    AVCodecParameters* pCodecPar = pFormatCtx->streams[aud_idx]->codecpar;
    const AVCodec* pCodec = avcodec_find_decoder(pCodecPar->codec_id);
    if (!pCodec)
    {
        printf("Codec not found\n");
        return -1;
    }
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    avcodec_parameters_to_context(pCodecCtx, pCodecPar);
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return -1;
    }
    printf("codec_id = %d, codec_name = %s\n", pCodecPar->codec_id, pCodec->name);

    uint64_t outChannelLayout = AV_CH_LAYOUT_STEREO;
    int outChannels = av_get_channel_layout_nb_channels(outChannelLayout);
    enum AVSampleFormat outSampleFmt = AV_SAMPLE_FMT_S16;
    int sizeOfOneSample = av_get_bytes_per_sample(outSampleFmt);
    int outSampleRate = pCodecCtx->sample_rate;
    printf("outChannels=%d,sizeOfOneSample=%d,outSampleRate=%d\n", outChannels, sizeOfOneSample, outSampleRate);
    //audio data size per second
    out_buffer_size = outSampleRate * sizeOfOneSample * outChannels;
    out_buffer = (uint8_t*)av_malloc(out_buffer_size);

    SwrContext* pSwrCtx = swr_alloc();
    swr_alloc_set_opts(pSwrCtx, outChannelLayout, outSampleFmt, outSampleRate, 
        pCodecCtx->channel_layout, pCodecCtx->sample_fmt, pCodecCtx->sample_rate, 0, NULL);
    swr_init(pSwrCtx);
    printf("channel_layout: %d==>%d; sample_fmt: %d==>%d, sample_rate: %d==>%d\n", 
        pCodecCtx->channel_layout, outChannelLayout, 
        pCodecCtx->sample_fmt, outSampleFmt, 
        pCodecCtx->sample_rate, outSampleRate);

    AVPacket* pPacket = (AVPacket*)av_malloc(sizeof(AVPacket));
    AVFrame* pFrame = av_frame_alloc();

    //----SDL
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER))
    {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    SDL_AudioSpec audioSpec;
    audioSpec.freq = outSampleRate;
    audioSpec.format = AUDIO_S16SYS;
    audioSpec.channels = outChannels;
    audioSpec.samples = 1024;
    audioSpec.silence = 0;
    audioSpec.callback = fill_audio;
    if (SDL_OpenAudio(&audioSpec, NULL) < 0) {
        printf("Could not open audio - %s\n", SDL_GetError());
        return -1;
    }
    SDL_PauseAudio(0);
    //FILE* fpPcmFile = fopen("out_put.pcm", "wb");
    int ret = 0;
    while (av_read_frame(pFormatCtx, pPacket) >= 0)
    {
        if (pPacket->stream_index == aud_idx)
        {
            int ret = 0;
            ret = avcodec_send_packet(pCodecCtx, pPacket);
            if (ret < 0) {
                printf("Error sending a packet for decoding\n");
                return -1;
            }
            printf("Packet sent successully-----------\n");
            while (ret >= 0)
            {
                ret = avcodec_receive_frame(pCodecCtx, pFrame);
                if (ret == AVERROR(EAGAIN))
                {
                    printf("Need to call `avcodec_send_packet` to send more data, ret=AVERROR(EAGAIN)\n");
                    break;
                }
                else if (ret == AVERROR_EOF)
                {
                    printf("Finish decoding, ret=AVERROR_EOF\n");
                    break;
                }
                else if (ret < 0)
                {
                    printf("Error during decoding\n");
                    exit(-1);
                }
                swr_convert(pSwrCtx, &out_buffer, out_buffer_size,
                    (const uint8_t**)pFrame->data, pFrame->nb_samples);
                act_buffer_size = av_samples_get_buffer_size(NULL, outChannels, pFrame->nb_samples, outSampleFmt, 1);
                printf("nb_samples=%d, act_buf_size=%d\n", pFrame->nb_samples, act_buffer_size);
                //fwrite(out_buffer, 1, act_buffer_size, fpPcmFile);
                cur_pos = out_buffer;
                left_len = act_buffer_size;
                while (left_len > 0)
                {
                    SDL_Delay(1);
                }
            }
        }
    }

    //fclose(fpPcmFile);

    avformat_close_input(&pFormatCtx);
    avcodec_free_context(&pCodecCtx);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);

    SDL_Quit();

    return 0;
}
#pragma once

#include <stdio.h>

#define __STDC_CONSTANT_MACROS
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"

#define SDL_MAIN_HANDLED
#include "SDL.h"

#define SFM_REFRESH_EVENT  (SDL_USEREVENT + 1)
int exit_flag = 0;
int sfp_refresh_thread(int* opaque)
{
    SDL_Event event;
    while (!exit_flag)
    {
        event.type = SFM_REFRESH_EVENT;
        SDL_PushEvent(&event);
        SDL_Delay(30);
    }
}

int start_decode_video()
{
    char* file_path = "F:/_git_home_/SimplePlayer/input/comic_cut.flv";
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

    //find video stream index
    int vid_idx = -1;
    printf("pFormatCtx->nb_streams = %d\n", pFormatCtx->nb_streams);
    for (int i = 0; i < pFormatCtx->nb_streams; ++i)
    {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            vid_idx = i;
            printf("find video index : %d\n", vid_idx);
            break;
        }
    }
    if (vid_idx == -1)
    {
        printf("Fail to find video stream\n");
        return -1;
    }

    //get decoder parameters
    AVCodecParameters* pCodecPar = pFormatCtx->streams[vid_idx]->codecpar;
    const AVCodec* pCodec = avcodec_find_decoder(pCodecPar->codec_id);;
    if (!pCodec)
    {
        printf("Codec not found\n");
        return -1;
    }
    //alloc decoder context
    AVCodecContext* pCodecCtx = avcodec_alloc_context3(pCodec);
    //set codec parameters to decoder
    avcodec_parameters_to_context(pCodecCtx, pCodecPar);
    //open decoder
    if (avcodec_open2(pCodecCtx, pCodec, NULL) < 0) {
        printf("Could not open codec\n");
        return -1;
    }
    printf("codec_id = %d, codec_name = %s\n", pCodecPar->codec_id, pCodec->name);

    AVPacket* pPacket = av_packet_alloc();
    if (!pPacket)
    {
        printf("av_packet_alloc failed.\n");
    }
    AVFrame* pFrame = av_frame_alloc();
    if (!pFrame) {
        printf("Could not allocate video frame\n");
        return -1;
    }
    printf("pCodecCtx->pix_fmt=%d \nwidth=%d, height=%d\n",
        pCodecCtx->pix_fmt, pCodecCtx->width, pCodecCtx->height);

    //----SDL
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
        printf("Could not initialize SDL - %s\n", SDL_GetError());
        return -1;
    }
    int window_w = pCodecCtx->width, window_h = pCodecCtx->height;
    SDL_Window  *pWindow = SDL_CreateWindow("Simplest ffmpeg player's Window",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
        window_w, window_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (!pWindow) {
        printf("SDL: could not set video mode - exiting:%s\n", SDL_GetError());
        return -1;
    }
    int pic_w = pCodecCtx->width, pic_h = pCodecCtx->height;
    SDL_Renderer *pRender = SDL_CreateRenderer(pWindow, -1, 0);
    SDL_Texture *pTexture = SDL_CreateTexture(pRender, 
        SDL_PIXELFORMAT_IYUV, SDL_TEXTUREACCESS_STREAMING, 
        pic_w, pic_h);
    SDL_Rect sdlRect = {0, 0, pic_w, pic_h };

    //FILE* fpyuv = fopen("test.yuv", "wb");
    
    SDL_CreateThread(sfp_refresh_thread, NULL, NULL);
    SDL_Event event;
    while (!exit_flag)
    {
        SDL_WaitEvent(&event);
        if (event.type == SFM_REFRESH_EVENT)
        {
            while (av_read_frame(pFormatCtx, pPacket) >= 0)
            {
                if (pPacket->stream_index == vid_idx)
                {
                    if (avcodec_send_packet(pCodecCtx, pPacket) < 0) {
                        printf("Error sending a packet for decoding\n");
                        exit_flag = 1;
                    }
                    int ret = avcodec_receive_frame(pCodecCtx, pFrame);
                    if (ret == 0)
                    {
                        //ysize= pCodecCtx->width * pCodecCtx->height;
                        //fwrite(pFrame->data[0], 1, y_size, fpyuv);
                        //fwrite(pFrame->data[1], 1, y_size / 4, fpyuv);
                        //fwrite(pFrame->data[2], 1, y_size / 4, fpyuv);
                        printf("decode success, receive a frame\n");
                        SDL_UpdateYUVTexture(pTexture, &sdlRect,
                            pFrame->data[0], pFrame->linesize[0],
                            pFrame->data[1], pFrame->linesize[1],
                            pFrame->data[2], pFrame->linesize[2]);
                        SDL_RenderClear(pRender);
                        SDL_RenderCopy(pRender, pTexture, NULL, &sdlRect);
                        SDL_RenderPresent(pRender);
                        break;
                    }
                    else if (ret != AVERROR(EAGAIN))
                    {
                        printf("Error when decoding a frame\n");
                        exit_flag = 1;
                    }
                }
            }
        }
        else if (event.type == SDL_WINDOWEVENT) 
        {
            // 1.will crash when stretch the height
            // 2.when shorten the width, there will be a remnant of the previous frame. 
            //SDL_GetWindowSize(pWindow, &sdlRect.w, &sdlRect.h); 
        }
        else if (event.type == SDL_QUIT) {
            exit_flag = 1;
        }
    }

    avformat_close_input(&pFormatCtx);
    avcodec_free_context(&pCodecCtx);
    av_frame_free(&pFrame);
    av_packet_free(&pPacket);
    //fclose(fpyuv);
    //system("pause");
    return 0;
}
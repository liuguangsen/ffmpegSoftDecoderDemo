// ConsoleApplication2.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libswscale\swscale.h"
#include "libavutil\log.h"
#include "libavutil\imgutils.h"
}

int main()
{
	//printf("hello VC++ %s: ",avcodec_configuration());
	printf("3758436 %u : ",avcodec_version());
	AVFormatContext *pFormatCtx;
	int i, videoindex;
	AVCodecContext *pCodecCtx;
	AVCodec *pCodec;
	AVFrame *pFrame, *pFrameYUV;
	uint8_t *out_buffer;
	AVPacket *packet;
	int y_size;
	int ret, got_picture;
	struct SwsContext *img_convert_ctx;

	char filepath[] = "demo.mp4";

	FILE *fp_yuv;
	int frame_cnt;

	//1 调用注册组件方法
	av_register_all();
	avformat_network_init();
	pFormatCtx = avformat_alloc_context();

	//打开视频文件
	if (avformat_open_input(&pFormatCtx, filepath, NULL, NULL) != 0) 
	{
		printf("Couldn't open file.\n");
		return -1;
	}

	if (avformat_find_stream_info(pFormatCtx, NULL)<0)
	{
		printf("Couldn't find stream information.\n");
		return -1;
	}


	//开始初始化一下实体
	videoindex = -1;
	for (int i = 0; i < pFormatCtx->nb_streams; i++)
	{
		if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
		{
			videoindex = i;
			break;
		}
		if (videoindex == -1) 
		{
			printf("Didn't find a video stream.\n");
			return -1;
		}
	}

	
	AVCodecParameters *p = pFormatCtx->streams[videoindex]->codecpar;
	//根据视轨index 查找解码器
	pCodec = avcodec_find_decoder(p->codec_id);
	if (pCodec == NULL)
	{
		printf("Codec not found.\n");
		return -1;
	}

	pCodecCtx = avcodec_alloc_context3(pCodec);
	if (pCodecCtx == NULL)
	{
		printf("AVCodecContext is NULL");
		return -1;
	}
	avcodec_parameters_to_context(pCodecCtx, p);
	//打开解码器
	if (avcodec_open2(pCodecCtx,pCodec,NULL)<0)
	{
		printf("Could not open codec.\n");
		return -1;
	}

	//测试相关信息1 打印下相关字段 值
	printf("duration: %d ",pFormatCtx->duration);
	printf("fengzhuang geshi: %s ",pFormatCtx->iformat->long_name);
	//测试相关信息2 参数值写入文件
	FILE *fp;
	fopen_s(&fp,"testinfo.txt","wb+");
	fprintf(fp,"duration: %d ", pFormatCtx->duration);
	fprintf(fp,"fengzhuang geshi: %s ", pFormatCtx->iformat->long_name);
	fclose(fp);

	//这里初始解码相关
	pFrame = av_frame_alloc();
	pFrameYUV = av_frame_alloc();
	out_buffer = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1));
	av_image_fill_arrays(pFrameYUV->data, pFrameYUV->linesize, out_buffer,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	packet = (AVPacket *)av_malloc(sizeof AVPacket);

	//output info---------------------
	printf("------------- File Information ------------");
	av_dump_format(pFormatCtx,0,filepath,0);
	printf("------------- ---------------- ------------");

	img_convert_ctx = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
	pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, NULL, NULL, NULL);

	//真正开始解码工作 这里while true 对每一帧数据进行处理
	frame_cnt = 0;

	//测试相关信息3
	FILE *fph264;
	fopen_s(&fph264, "testh264.h264", "wb+");

	FILE *fpyuv;
	fopen_s(&fpyuv, "testyuv.yuv", "wb+");
	
	while (av_read_frame(pFormatCtx,packet)>=0)
	{
		if (packet->stream_index == videoindex)
		{
			//测试相关信息3
			fwrite(packet->data,1,packet->size, fph264);

			ret = avcodec_send_packet(pCodecCtx, packet);
			if (ret!=0)
			{
				printf("avcodec_send_packet Error");
				continue;
			}
			
			ret = avcodec_receive_frame(pCodecCtx,pFrame);

			if (ret<0)
			{
				printf("avcodec_receive_frame Error");
				return -1;
			}
			//ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_picture, packet);
			
			printf("decode dts %d : ",packet->dts);

			// 解码的数据 会有问题 这里处理下
			if (ret == 0)
			{
				sws_scale(img_convert_ctx, (const uint8_t* const*)pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
					pFrameYUV->data, pFrameYUV->linesize);
				printf("Decoded frame index: %d\n", frame_cnt);

				//测试相关信息3 
				fwrite(pFrameYUV->data[0], 1, pCodecCtx->width*pCodecCtx->height, fpyuv);
				fwrite(pFrameYUV->data[1], 1, pCodecCtx->width*pCodecCtx->height/4, fpyuv);
				fwrite(pFrameYUV->data[2], 1, pCodecCtx->width*pCodecCtx->height/4, fpyuv);

				frame_cnt++;
			}
		}
		av_packet_unref(packet);
	}

	fclose(fph264);
	fclose(fpyuv);
	sws_freeContext(img_convert_ctx);

	av_frame_free(&pFrameYUV);
	av_frame_free(&pFrame);
	avcodec_free_context(&pCodecCtx);
	avcodec_close(pCodecCtx);
	avformat_close_input(&pFormatCtx);
	
	printf("over");

    return 0;
}


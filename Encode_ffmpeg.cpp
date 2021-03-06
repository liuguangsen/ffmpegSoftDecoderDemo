// ConsoleApplication4.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"

extern "C"
{
#include "libavcodec\avcodec.h"
#include "libavformat\avformat.h"
#include "libavutil\avutil.h"
#include "libswresample\swresample.h"
#include "libswscale\swscale.h"
#include "libavutil\imgutils.h"
}

void flush_encoder(AVFormatContext * fmt_ctx, AVCodecContext * c_ctx, unsigned int stream_index)
{
	int framecnt = 0;
	int ret;
	AVPacket pkt;
	while (1)
	{
		pkt.data = NULL;
		pkt.size = 0;
		av_init_packet(&pkt);

		ret = avcodec_send_frame(c_ctx, NULL);

		ret = avcodec_receive_packet(c_ctx, &pkt);
		if (ret != 0)
		{
			break;
		}
		framecnt++;
		ret = av_write_frame(fmt_ctx, &pkt);
		printf("Succees to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
		if (ret < 0)
			break;
	}

int main()
{
	//printf("ffmpeg encode demo %s: ",avcodec_configuration());

	AVFormatContext * pFormatCtx;
	AVOutputFormat *fmt;// 和AVInputFormat 对应 在编码的时候 使用
	AVStream * video_st;// 编码对应的 码流结构体
	AVCodecContext * pCodecCtx;//编码器对应的 信息结构体
	const AVCodec * pCodec;
	AVPacket pkt;//用来存储解码后的每一帧数据 最后可以对这个数据进行操作 干哈都行
	uint8_t * picture_buf;//图片的真实大小 这里默认从 480*360 转换 （可以查看测试的视频宽高）
	AVFrame *pFrame;//需要从yuv文件中拿到数据 最后构造一个AVFrame 编码前的一帧数据
	int picture_size;//宽高
	int y_size;
	int framecnt = 0;

	
	FILE * inputfp;//引入测试的yuv格式的文件
	fopen_s(&inputfp, "testyuv.yuv", "rb");
	int in_w = 480, in_h = 360;//这里定义下测试的宽高
	int framenum = 4456;//有可以播放yuv格式的播放器 可以看下 总的帧数
	const char * out_file = "testh264.mp4";

	//1 注册编码器 等 相关
	av_register_all();
	//Method1.
	pFormatCtx = avformat_alloc_context();
	//Guess a Format
	fmt = av_guess_format(NULL, out_file, NULL);
	pFormatCtx->oformat = fmt;

	//Method 2.  
	//avformat_alloc_output_context2(&pFormatCtx, NULL, NULL, out_file);  
	//fmt = pFormatCtx->oformat;  

	//2 打开输出文件
	if (avio_open(&pFormatCtx->pb, out_file, AVIO_FLAG_READ_WRITE)<0)
	{
		printf("Failed to open output file! \n");
		return -1;
	}

	//3 构造一个输出码流 AVStream
	video_st = avformat_new_stream(pFormatCtx, 0);
	if (video_st == NULL)
	{
		printf("AVStream create failed.\n");
		return -1;
	}

	//4 构建参数
	//Param that must set 

	pCodecCtx = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(pCodecCtx, video_st->codecpar);

	pCodecCtx->codec_id = AV_CODEC_ID_H264;
	pCodecCtx->codec = avcodec_find_encoder(pCodecCtx->codec_id);
	pCodecCtx->codec_type = AVMEDIA_TYPE_VIDEO;
	pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;
	pCodecCtx->width = in_w;
	pCodecCtx->height = in_h;
	pCodecCtx->bit_rate = 400000;
	pCodecCtx->gop_size = 250;//

	pCodecCtx->time_base.num = 1;
	pCodecCtx->time_base.den = 25;

	//H264
	pCodecCtx->qmin = 10;
	pCodecCtx->qmax = 51;

	//Optional param
	pCodecCtx->max_b_frames = 3;

	AVCodecParameters * p = video_st->codecpar;
	p->codec_id = pCodecCtx->codec_id;
	p->codec_type = pCodecCtx->codec_type;
	p->width = pCodecCtx->width;
	p->height = pCodecCtx->height;
	p->format = pCodecCtx->pix_fmt;
	p->bit_rate = pCodecCtx->bit_rate;
	video_st->time_base = pCodecCtx->time_base;
	
	AVDictionary *param = 0;
	//H.264  
	if (pCodecCtx->codec_id == AV_CODEC_ID_H264) {
		av_dict_set(&param, "preset", "slow", 0);
		av_dict_set(&param, "tune", "zerolatency", 0);
	}
	
	//5 打印我们构建的信息 
	av_dump_format(pFormatCtx, 0, out_file, 1);

	//6 根据code_id 查找编码器 
	pCodec = pCodecCtx->codec;
	if (pCodec == NULL)
	{
		printf("Didn't find EnCoder.\n");
		return -1;
	}
	if (avcodec_open2(pCodecCtx, pCodec, &param) < 0)
	{
		printf("Failed to open encoder ! \n");
		return -1;
	}

	//7 创建一个AVframe来存储yuv数据 
	pFrame = av_frame_alloc();
	picture_size = pCodecCtx->width * pCodecCtx->height * 3 / 2;
	//picture_size = av_image_get_buffer_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);
	picture_buf = (uint8_t *)av_malloc(picture_size);
	av_image_fill_arrays(pFrame->data, pFrame->linesize, picture_buf,
		AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height, 1);

	//8 先写个头
	avformat_write_header(pFormatCtx, NULL);

	//9 创建一个AVPacket 来存储h264数据
	av_new_packet(&pkt, picture_size);

	//10 开始循环编码 yuv数据
	y_size = pCodecCtx->width*pCodecCtx->height;
	int index = 0;
	for (int i = 0; i < framenum; i++)
	{
		//读取yuv data
		if (fread(picture_buf, 1, y_size * 3 / 2, inputfp) <= 0)
		{
			printf("Failed to read raw yuv data ! \n");
			return -1;
		}
		else if (feof(inputfp))
		{
			break;
		}

		//PTS
		pFrame->pts = i * (video_st->time_base.den) / ((video_st->time_base.num) * 25);
		pFrame->width = pCodecCtx->width;
		pFrame->height = pCodecCtx->height;
		pFrame->format = pCodecCtx->pix_fmt;

		//Encdoe
		int ret = avcodec_send_frame(pCodecCtx, pFrame);
		if (ret != 0)
		{
			printf("avcodec_send_frame error! \n");
			return -1;
		}
		ret = avcodec_receive_packet(pCodecCtx, &pkt);
		if (ret == 0)
		{
			printf("Succees to encode frame: %5d\tsize:%5d\n", framecnt, pkt.size);
			framecnt++;
			pkt.stream_index = video_st->index;
			ret = av_write_frame(pFormatCtx, &pkt);
			av_packet_unref(&pkt);

		}

	}

	//11 Flush Encoder for 剩余的packet
	flush_encoder(pFormatCtx,pCodecCtx,video_st->index);

	//12 写尾
	av_write_trailer(pFormatCtx);

	if (video_st) {
		avcodec_close(pCodecCtx);
		av_free(pFrame);
		av_free(picture_buf);
	}
	avio_close(pFormatCtx->pb);
	avformat_free_context(pFormatCtx);

	fclose(inputfp);
    return 0;
}


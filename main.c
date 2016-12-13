#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "display_ss.h"
#include "convert_ss.h"
#include "video_ss.h"

#define DEFAULT_DISPLAY_MODULE "fb"
#define DEFAULT_VIDEO_MODULE "v4l2"

void do_inits(void)
{
	/* 显示子系统和显示模块初始化,设置默认模块  */
	display_init();
	display_modules_init();
	choose_default_display_module(DEFAULT_DISPLAY_MODULE);

	/* 视频子系统和视频模块初始化, 设置默认模块 */
	video_init();
	video_modules_init();
	choose_default_video_module(DEFAULT_VIDEO_MODULE);

	/* 视频转换子系统初始化 */
	VideoConvertInit();
}

int main(int argc, char *argv[])
{
	int i, j;
	int lcd_row, lcd_col;
	int cam_row, cam_col;

	/*
	 * 分配两块内存区域用于临时存放视频数据
	 * 因为一个像素点用16bpp表示
	 * 所以数据类型用short
	 * 摄像头采集到的数据是320x240的,放入cam_mem
	 * LCD显示器的尺寸是240x320
	 * 把cam_mem里的数据放入lcd_mem
	 * 最后把lcd_mem放入到framebuffer
	 */

	/*
	 *
	 * cam_mem---->	-----------320------------>x
	 * 					|         |
	 * 					|         |
	 * 					240       |
	 * 					|---------p(x, y)
	 * 					|
	 * 					V
	 * 					y
	 */

	/*
	 *
	 * lcd_mem---->	------240----->x
	 * 					|    |
	 * 					|    |
	 * 					|----p(y, 320 - x)
	 * 					|
	 * 					320
	 * 					|
	 * 					|
	 * 					|
	 * 					|
	 * 					V
	 * 					y
	 */
	unsigned short **cam_mem;
	unsigned short **lcd_mem;

	/* 用于操作每一个像素点 */
	unsigned short *s = NULL;
	unsigned short *d = NULL;

	int iError;
	int iLcdWidth;
	int iLcdHeight;
	int iLcdBpp;
	int iPixelFormatOfDisp;
	int iPixelFormatOfVideo;

	struct VideoBuf	*ptVideoBufCur;

	struct VideoBuf tVideoBuf;//摄像头采集到的数据
	struct VideoBuf tConvertBuf;//转换后的数据
	struct VideoBuf tZoomBuf;//缩放后的数据
	struct VideoBuf tFrameBuf;//最终刷入framebuf的数据

	printf("Video2Lcd version 2.1\n");

	/* 初始化工作 */
	do_inits();

	/* 获取显示参数 */
	GetDispResolution(&iLcdWidth, &iLcdHeight, &iLcdBpp);
	printf("LCD display format [%d x %d][%dBpp]\n", iLcdWidth, iLcdHeight, iLcdBpp);
	lcd_row = iLcdWidth;
	lcd_col = iLcdHeight;

	/* 动态分配二维数组 */
	lcd_mem = (unsigned short **)malloc(sizeof(unsigned short *) * lcd_row);
	if (NULL == lcd_mem)
	{
		printf("no mem ERROR\n");
		return -1;
	}
	for (i = 0; i < lcd_row; i++)
		lcd_mem[i] = (unsigned short *)malloc(sizeof(unsigned short) * lcd_col);

	/* 设置framebuffer */
	GetVideoBufForDisplay(&tFrameBuf);
	iPixelFormatOfDisp = tFrameBuf.iPixelFormat;

	/*  获取摄像头参数 */
	get_camera_format(&cam_row, &cam_col, &iPixelFormatOfVideo);
	printf("CAMERA data format [%d x %d]\n", cam_row, cam_col);

	/* 动态分配二维数组 */
	cam_mem = (unsigned short **)malloc(sizeof(unsigned short *) * cam_row);
	if (NULL == cam_mem)
	{
		printf("no mem ERROR\n");
		return -1;
	}
	for (i = 0; i < cam_row; i++)
		cam_mem[i] = (unsigned short *)malloc(sizeof(unsigned short) * cam_col);


	/* 根据采集到的视频数据格式选取一个合适的转换函数 */
	iError = find_support_convert_module(iPixelFormatOfVideo, iPixelFormatOfDisp);
	if (!iError)
	{
		printf("can not support this format convert\n");
		return -1;
	}

	/* 启动摄像头 */
	iError = start_camera();
	if (iError)
	{
		printf("StartDevice %s error!!\n", argv[1]);
		return -1;
	}

	/* 分配视频数据缓冲区 */
	/* 1. 给直接通过摄像头采集到的数据使用 */
	memset(&tVideoBuf, 0, sizeof(T_VideoBuf));

	/* 2. 转化后的数据区 */
	memset(&tConvertBuf, 0, sizeof(T_VideoBuf));
	tConvertBuf.iPixelFormat = iPixelFormatOfDisp;
	tConvertBuf.tPixelDatas.iBpp = iLcdBpp;

	/* 3. 缩放后的数据区 */
	memset(&tZoomBuf, 0, sizeof(T_VideoBuf));

	/* 从摄像头读出数据后处理 */
	while (1)
	{
		/* 1. 读摄像头数据 */
		iError = get_frame(&tVideoBuf);
		if (iError)
		{
			printf("####get frame ERROR####\n");
			return -1;
		}

		/* 保存当前视频数据的地址 */
		ptVideoBufCur = &tVideoBuf;

		/* 2. 判断是否需要转换为RGB */
		if (iPixelFormatOfVideo != iPixelFormatOfDisp)
		{
			iError = video_convert2rgb(&tVideoBuf, &tConvertBuf);
			if (iError)
			{
				printf("Convert error\n");
				return -1;
			}

			/* 设置当前数据指针 */
			ptVideoBufCur = &tConvertBuf;
		}

		/* 操作源数据 */
		s = (unsigned short *)ptVideoBufCur->tPixelDatas.aucPixelDatas;

		/* 操作framebuffer */
		d = (unsigned short *)tFrameBuf.tPixelDatas.aucPixelDatas;

		/* 把摄像头采集的数据放入cam_mem */
		for (i = 0; i < cam_col; i++)
			for (j = 0; j < cam_row; j++)
				cam_mem[j][i] = *s++;

		/* 把cam_mem里的数据转存到lcd_mem */
		for (i = 0; i < cam_col; i++)
			for (j = 0; j < cam_row; j++)
				lcd_mem[i][cam_row - j] = cam_mem[j][i];

		for (i = 0; i < lcd_col; i++)
			for (j = 0; j < lcd_row; j++)
				*d++ = lcd_mem[j][i];

		/* 释放该帧数据,重新放入采集视频的队列 */
		iError = put_frame(&tVideoBuf);
		if (iError)
		{
			printf("Put frame error\n");
			return -1;
		}
	}

	/* 释放内存 */
    for (i = 0; i < lcd_row; i++)
        free(lcd_mem[i]);
    free(lcd_mem);

    for (i = 0; i < cam_row; i++)
        free(cam_mem[i]);
    free(cam_mem);

	return 0;
}

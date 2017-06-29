#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "display_ss.h"
#include "convert_ss.h"
#include "video_ss.h"
#include "compile_time.h"

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

int PicZoom(PT_PixelDatas ptOriginPic, PT_PixelDatas ptZoomPic)
{
	unsigned long dwDstWidth = ptZoomPic->iWidth;
	unsigned long* pdwSrcXTable;
	unsigned long x;
	unsigned long y;
	unsigned long dwSrcY;
	unsigned char *pucDest;
	unsigned char *pucSrc;
	unsigned long dwPixelBytes = ptOriginPic->iBpp/8;

#if 0
	printf("src:\n");
	printf("%d x %d, %d bpp, data: 0x%x\n", ptOriginPic->iWidth, ptOriginPic->iHeight, ptOriginPic->iBpp, (unsigned int)ptOriginPic->aucPixelDatas);

	printf("dest:\n");
	printf("%d x %d, %d bpp, data: 0x%x\n", ptZoomPic->iWidth, ptZoomPic->iHeight, ptZoomPic->iBpp, (unsigned int)ptZoomPic->aucPixelDatas);
#endif

	if (ptOriginPic->iBpp != ptZoomPic->iBpp)
	{
		return -1;
	}

	pdwSrcXTable = malloc(sizeof(unsigned long) * dwDstWidth);
	if (NULL == pdwSrcXTable)
	{
		printf("malloc error!\n");
		return -1;
	}

	for (x = 0; x < dwDstWidth; x++)//生成表 pdwSrcXTable
	{
		pdwSrcXTable[x]=(x*ptOriginPic->iWidth/ptZoomPic->iWidth);
	}

	for (y = 0; y < ptZoomPic->iHeight; y++)
	{
		dwSrcY = (y * ptOriginPic->iHeight / ptZoomPic->iHeight);

		pucDest = ptZoomPic->aucPixelDatas + y*ptZoomPic->iLineBytes;
		pucSrc  = ptOriginPic->aucPixelDatas + dwSrcY*ptOriginPic->iLineBytes;

		for (x = 0; x <dwDstWidth; x++)
		{
			/* 原图座标: pdwSrcXTable[x]，srcy
			 * 缩放座标: x, y
			 */
			memcpy(pucDest+x*dwPixelBytes, pucSrc+pdwSrcXTable[x]*dwPixelBytes, dwPixelBytes);
		}
	}

	free(pdwSrcXTable);
	return 0;
}

int PicMerge(int iX, int iY, PT_PixelDatas ptSmallPic, PT_PixelDatas ptBigPic)
{
	int i;
	unsigned char *pucSrc;
	unsigned char *pucDst;

	if ((ptSmallPic->iWidth > ptBigPic->iWidth)  ||
			(ptSmallPic->iHeight > ptBigPic->iHeight) ||
			(ptSmallPic->iBpp != ptBigPic->iBpp))
	{
		return -1;
	}

	pucSrc = ptSmallPic->aucPixelDatas;
	pucDst = ptBigPic->aucPixelDatas + iY * ptBigPic->iLineBytes + iX * ptBigPic->iBpp / 8;
	for (i = 0; i < ptSmallPic->iHeight; i++)
	{
		memcpy(pucDst, pucSrc, ptSmallPic->iLineBytes);
		pucSrc += ptSmallPic->iLineBytes;
		pucDst += ptBigPic->iLineBytes;
	}
	return 0;
}

int main(int argc, char *argv[])
{
#ifdef MINI2440_LCD_X35
	int i, j;
	unsigned short **cam_mem;
	unsigned short **lcd_mem;

	/* 用于操作每一个像素点 */
	unsigned short *s = NULL;
	unsigned short *d = NULL;
#endif

	int lcd_row, lcd_col;
	int cam_row, cam_col;
	int iTopLeftX;
	int iTopLeftY;
	float k;

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
#ifdef MINI2440_LCD_X35
	struct VideoBuf tRotateBuf;//旋转后的数据
#endif

	printf("Video2Lcd version 2.1 %s\n", COMPILE_DATE);

	/* 初始化工作 */
	do_inits();

	/* 获取显示参数 */
	GetDispResolution(&iLcdWidth, &iLcdHeight, &iLcdBpp);
	printf("LCD display format [%d x %d][%dBpp]\n", iLcdWidth, iLcdHeight, iLcdBpp);
	lcd_row = iLcdWidth;
	lcd_col = iLcdHeight;

#ifdef MINI2440_LCD_X35
	/* 动态分配二维数组 */
	lcd_mem = (unsigned short **)malloc(sizeof(unsigned short *) * lcd_row);
	if (NULL == lcd_mem)
	{
		printf("no mem ERROR\n");
		return -1;
	}
	for (i = 0; i < lcd_row; i++)
		lcd_mem[i] = (unsigned short *)malloc(sizeof(unsigned short) * lcd_col);
#endif

	/* 设置framebuffer */
	GetVideoBufForDisplay(&tFrameBuf);
	iPixelFormatOfDisp = tFrameBuf.iPixelFormat;

	/*  获取摄像头参数 */
	get_camera_format(&cam_row, &cam_col, &iPixelFormatOfVideo);
	printf("CAMERA data format [%d x %d]\n", cam_row, cam_col);

#ifdef MINI2440_LCD_X35
	/* 动态分配二维数组 */
	cam_mem = (unsigned short **)malloc(sizeof(unsigned short *) * cam_row);
	if (NULL == cam_mem)
	{
		printf("no mem ERROR\n");
		return -1;
	}
	for (i = 0; i < cam_row; i++)
		cam_mem[i] = (unsigned short *)malloc(sizeof(unsigned short) * cam_col);
#endif

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

#ifdef MINI2440_LCD_X35
	memset(&tRotateBuf, 0, sizeof(T_VideoBuf));
#endif

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

		/* 旋转摄像头 */
#ifdef MINI2440_LCD_X35
		memcpy(&tRotateBuf, &tFrameBuf, sizeof(struct VideoBuf));
		/* 操作源数据 */
		s = (unsigned short *)ptVideoBufCur->tPixelDatas.aucPixelDatas;

		/* 操作rotate buffer */
		d = (unsigned short *)tRotateBuf.tPixelDatas.aucPixelDatas;

		/* 把摄像头采集的数据放入cam_mem */
		for (i = 0; i < cam_col; i++)
			for (j = 0; j < cam_row; j++)
				cam_mem[j][i] = *s++;

		/* 把cam_mem里的数据转存到lcd_mem */
		for (i = 0; i < cam_col; i++)
			for (j = 0; j < cam_row; j++)
				lcd_mem[i][cam_row - j] = cam_mem[j][i];

		/* 把旋转后的数据放入rotate buffer */
		for (i = 0; i < lcd_col; i++)
			for (j = 0; j < lcd_row; j++)
				*d++ = lcd_mem[j][i];

		/* 使当前video buffer指针指向旋转后的buffer */
		ptVideoBufCur = &tRotateBuf;
#endif
		/* 如果图像分辨率大于LCD, 缩放 */
		if (ptVideoBufCur->tPixelDatas.iWidth > iLcdWidth || ptVideoBufCur->tPixelDatas.iHeight > iLcdHeight)
		{
			/* 确定缩放后的分辨率
			 * 把图片按比例缩放到VideoMem上, 居中显示
			 * 先算出缩放后的大小
			 * 摄像头获取到的图像的高宽比(k)
			 */
			k = (float)ptVideoBufCur->tPixelDatas.iHeight / ptVideoBufCur->tPixelDatas.iWidth;

			/* 需要缩放到的尺寸, 先宽不变,高按比例缩放 */
			tZoomBuf.tPixelDatas.iWidth  = iLcdWidth;
			tZoomBuf.tPixelDatas.iHeight = iLcdWidth * k;

			/* 上面缩放不理想的话按, 先高不变,宽按比例缩放 */
			if (tZoomBuf.tPixelDatas.iHeight > iLcdHeight)
			{
				tZoomBuf.tPixelDatas.iWidth  = iLcdHeight / k;
				tZoomBuf.tPixelDatas.iHeight = iLcdHeight;
			}

			tZoomBuf.tPixelDatas.iBpp        = iLcdBpp;
			tZoomBuf.tPixelDatas.iLineBytes  = tZoomBuf.tPixelDatas.iWidth * tZoomBuf.tPixelDatas.iBpp / 8;
			tZoomBuf.tPixelDatas.iTotalBytes = tZoomBuf.tPixelDatas.iLineBytes * tZoomBuf.tPixelDatas.iHeight;

			/* 给缩放buffer分配空间 */
			if (!tZoomBuf.tPixelDatas.aucPixelDatas)
				tZoomBuf.tPixelDatas.aucPixelDatas = malloc(tZoomBuf.tPixelDatas.iTotalBytes);

			/* 将当前视频数据缩放 */
			PicZoom(&ptVideoBufCur->tPixelDatas, &tZoomBuf.tPixelDatas);
			ptVideoBufCur = &tZoomBuf;
		}

		/* 合并进framebuffer */
		/* 接着算出居中显示时左上角坐标 */
		iTopLeftX = (iLcdWidth - ptVideoBufCur->tPixelDatas.iWidth) / 2;
		iTopLeftY = (iLcdHeight - ptVideoBufCur->tPixelDatas.iHeight) / 2;

		PicMerge(iTopLeftX, iTopLeftY, &ptVideoBufCur->tPixelDatas, &tFrameBuf.tPixelDatas);

		FlushPixelDatasToDev(&tFrameBuf.tPixelDatas);
		/* 释放该帧数据,重新放入采集视频的队列 */
		iError = put_frame(&tVideoBuf);
		if (iError)
		{
			printf("Put frame error\n");
			return -1;
		}
	}

#ifdef MINI2440_LCD_X35
	/* 释放内存 */
    for (i = 0; i < lcd_row; i++)
        free(lcd_mem[i]);
    free(lcd_mem);

    for (i = 0; i < cam_row; i++)
        free(cam_mem[i]);
    free(cam_mem);
#endif
	return 0;
}

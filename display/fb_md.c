#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <fcntl.h>

#include "display_ss.h"

static int FbDeviceInit(struct DispOpr *pModule);
static int FbCleanScreen(struct DispOpr *pModule, unsigned int dwBackColor);
static int FBShowPage(struct DispOpr *pModule, PT_PixelDatas ptPixelDatas);

static struct DispOpr disp_module = {
	.name = "fb",
	.use_as_default = 0,
	.DeviceInit = FbDeviceInit,
	.CleanScreen = FbCleanScreen,
	.ShowPage = FBShowPage,
};

static int FBShowPage(struct DispOpr *pModule, PT_PixelDatas ptPixelDatas)
{
	if (pModule->pucFbMem != ptPixelDatas->aucPixelDatas)
		memcpy(pModule->pucFbMem, ptPixelDatas->aucPixelDatas, ptPixelDatas->iTotalBytes);
	return 0;
}

static int FbDeviceInit(struct DispOpr *pModule)
{
	int iError;
	int iFd;

	iFd = open("/dev/graphics/fb0", O_RDWR);
	if (iFd < 0)
	{
		printf("can't open device fb\n");
		return -1;
	}

	/* 获取LCD的可变参数和固定参数 */
	iError = ioctl(iFd, FBIOGET_VSCREENINFO, &pModule->fb_var);
	if (iError < 0)
	{
		printf("can't get fb's var\n");
		return -1;
	}

	iError = ioctl(iFd, FBIOGET_FSCREENINFO, &pModule->fb_fix);
	if (iError < 0)
	{
		printf("can't get fb's fix\n");
		return -1;
	}

	pModule->dwScreenSize = pModule->fb_var.xres * pModule->fb_var.yres * pModule->fb_var.bits_per_pixel / 8;
	pModule->pucFbMem = (unsigned char *)mmap(NULL, pModule->dwScreenSize, PROT_READ | PROT_WRITE, MAP_SHARED, iFd, 0);
	if (pModule->pucFbMem < 0)
	{
		printf("can't mmap\n");
		return -1;
	}

	pModule->iXres = pModule->fb_var.xres;
	pModule->iYres = pModule->fb_var.yres;
	pModule->iBpp = pModule->fb_var.bits_per_pixel;
	pModule->iLineWidth = pModule->fb_var.xres * pModule->iBpp / 8;

	return 0;
}

static int FbCleanScreen(struct DispOpr *pModule, unsigned int dwBackColor)
{
	unsigned char *pucFb;
	unsigned short *pwFb16bpp;
	unsigned int *pdwFb32bpp;
	unsigned short wColor16bpp;

	int iRed;
	int iGreen;
	int iBlue;
	int i = 0;

	pucFb = pModule->pucFbMem;
	pwFb16bpp = (unsigned short *)pucFb;
	pdwFb32bpp = (unsigned int *)pucFb;

	switch (pModule->fb_var.bits_per_pixel)
	{
		case 8:
			memset(pModule->pucFbMem, dwBackColor, pModule->dwScreenSize);
			break;
		case 16:
			/*
			 * 从dwBackColor中取出红绿蓝三原色
			 * 构造为16bpp的颜色
			 */
			iRed = (dwBackColor >> (16 + 3)) & 0x1f;
			iGreen = (dwBackColor >> (8 + 2)) & 0x3f;
			iBlue = (dwBackColor >> 3) & 0x1f;

			wColor16bpp = (iRed << 11) | (iGreen << 5) | iBlue;

			while (i < pModule->dwScreenSize)
			{
				*pwFb16bpp = wColor16bpp;
				pwFb16bpp++;
				i += 2;
			}
			break;
		case 32:
			while (i < pModule->dwScreenSize)
			{
				*pdwFb32bpp = dwBackColor;
				pdwFb32bpp++;
				i+= 4;
			}
			break;
		default:
			printf("can't support %d bpp\n", pModule->fb_var.bits_per_pixel);
			return -1;
	}

	return 0;
}

int fb_init(void)
{
	printf("Display module fb init\n");
	/* 调用子系统提供的注册接口向子系统注册模块 */
	return display_register(&disp_module.list);
}

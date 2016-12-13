#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <string.h>
#include <vga.h>
#include <vgagl.h>
#include <stdlib.h>
#include <stdio.h>

#include "display_ss.h"

static GraphicsContext *physicalscreen;
static GraphicsContext *virtualscreen;

static int CRTDeviceInit(struct DispOpr *pModule);
static int CRTShowPixel(struct DispOpr *pModule, int iX, int iY, unsigned int dwColor);
static int CRTCleanScreen(struct DispOpr *pModule, unsigned int dwBackColor);
//static int CRTDeviceExit(void);
static int CRTShowPage(struct DispOpr *pModule, PT_PixelDatas ptPixelDatas);

static struct DispOpr disp_module = {
	.name        = "crt",
	.use_as_default = 0,
	.DeviceInit  = CRTDeviceInit,
	.ShowPixel   = CRTShowPixel,
	.CleanScreen = CRTCleanScreen,
	.ShowPage    = CRTShowPage,
};

static int CRTDeviceInit(struct DispOpr *pModule)
{
    vga_init();
    vga_setmode(G640x480x64K);
    gl_setcontextvga(G640x480x64K);

    /* 資誼"麗尖徳鳥" */
    physicalscreen = gl_allocatecontext();
    gl_getcontext(physicalscreen);

    /* 資誼"倡亭徳鳥" */
    gl_setcontextvgavirtual(G640x480x64K);
    virtualscreen = gl_allocatecontext();
    gl_getcontext(virtualscreen);

    /* 譜崔"倡亭徳鳥"葎輝念侭聞喘議"徳鳥" */
    gl_setcontext(virtualscreen);

	pModule->iXres = 640;
	pModule->iYres = 480;
	pModule->iBpp  = 32;

    pModule->iLineWidth = pModule->iXres * pModule->iBpp / 8;

    pModule->pucFbMem = malloc(pModule->iLineWidth * pModule->iYres);

	return 0;
}

#if 0
static int CRTDeviceExit(void)
{
    free(pModule->pucDispMem);
    gl_clearscreen(0);
	vga_setmode(TEXT);
	return 0;
}
#endif


static int CRTShowPixel(struct DispOpr *pModule, int iX, int iY, unsigned int dwColor)
{
	int iRed, iGreen, iBlue;

	iRed   = (dwColor >> 16) & 0xff;
	iGreen = (dwColor >> 8) & 0xff;
	iBlue  = (dwColor >> 0) & 0xff;

//	gl_setpalettecolor(5, iRed>>2, iGreen>>2, iBlue>>2);   /* 0xE7DBB5	*/ /* 刑仔議崕 */
//	vga_setcolor(5);

//	vga_drawpixel(iX, iY);

    gl_setpixelrgb(iX, iY, iRed, iGreen, iBlue);

    gl_copyscreen(physicalscreen);

	return 0;
}


static int CRTCleanScreen(struct DispOpr *pModule, unsigned int dwBackColor)
{
	int iX;
	int iY;
	int iRed, iGreen, iBlue;

	iRed   = (dwBackColor >> 16) & 0xff;
	iGreen = (dwBackColor >> 8) & 0xff;
	iBlue  = (dwBackColor >> 0) & 0xff;

//	gl_setpalettecolor(4, iRed>>2, iGreen>>2, iBlue>>2);   /* 0xE7DBB5  */ /* 刑仔議崕 */
//	vga_setcolor(4);

	for (iX = 0; iX < 320; iX++)
		for (iY = 0; iY < 200; iY++)
            gl_setpixelrgb(iX, iY, iRed, iGreen, iBlue);

    gl_copyscreen(physicalscreen);

	return 0;
}

static int CRTShowPage(struct DispOpr *pModule, PT_PixelDatas ptPixelDatas)
{
    int x, y;
    unsigned int *pdwColor = (unsigned int *)ptPixelDatas->aucPixelDatas;
    unsigned int dwColor;
    unsigned int dwRed, dwGreen, dwBlue;

    if (ptPixelDatas->iBpp != 32)
    {
        return -1;
    }

    for (y = 0; y < pModule->iYres; y++)
    {
        for (x = 0; x < pModule->iXres; x++)
        {
            /* 0x00RRGGBB */
            dwColor = *pdwColor++;
            dwRed   = (dwColor >> 16) & 0xff;
            dwGreen = (dwColor >> 8) & 0xff;
            dwBlue  = (dwColor >> 0) & 0xff;

            // CRTShowPixel(x, y, dwColor);
            gl_setpixelrgb(x, y, dwRed, dwGreen, dwBlue);
        }
    }

    gl_copyscreen(physicalscreen);

    return 0;
}


int CRTInit(void)
{
	printf("Display module crt init\n");
	/* 调用子系统提供的注册接口向子系统注册模块 */
	return display_register(&disp_module.list);
}


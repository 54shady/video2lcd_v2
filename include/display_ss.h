#ifndef _display_H_
#define _display_H_

#include "klist.h"
#include <linux/fb.h>
#include "video_ss.h"

struct DispOpr {
	/* variables */
	char *name;
	int iXres;
	int iYres;
	int iBpp;
	int iLineWidth;
	int use_as_default;

	/* 显示屏大小*/
	unsigned int dwScreenSize;

	/* 显存地址 */
	unsigned char *pucFbMem;

	/* functions */
	int (*DeviceInit)(struct DispOpr *pModule);
	int (*CleanScreen)(struct DispOpr *pModule, unsigned int dwBackColor);
	int (*ShowPixel)(struct DispOpr *pModule, int iX, int iY, unsigned int dwColor);

	int (*ShowPage)(struct DispOpr *pModule, PT_PixelDatas ptPixelDatas);
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;

	struct list_head list;
};

int display_register(struct list_head *list);
int fb_init(void);
int CRTInit(void);


/* call for app */
int display_init(void);
int GetDispResolution(int *piXres, int *piYres, int *piBpp);
int GetVideoBufForDisplay(struct VideoBuf *ptFrameBuf);
void display_modules_init(void);
void choose_default_display_module(const char *name);

#endif

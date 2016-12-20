#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "display_ss.h"
#include "video_ss.h"

/* 将该子系统里所有模块都装入链表 */
LIST_HEAD(display_list);

/* 开放给底层具体模块的注册接口 */
int display_register(struct list_head *list)
{
	list_add(list, &display_list);
	return 0;
}

/* 开放给应用层调用 */
int display_init(void)
{
	int iError;
	printf("Display subsystem init\n");
	/* 调用个模块初始化函数 */
	iError = fb_init();
	//iError |= CRTInit();

	return iError;
}

struct DispOpr *display_get_module(const char *name)
{
	struct DispOpr *pModule;

	list_for_each_entry(pModule, &display_list, list)
	{
		if (!strcmp(name, pModule->name))
			return pModule;
	}

	printf("no sub module ERROR\n");
	return NULL;
}

static struct DispOpr *get_default_module(void)
{
	struct DispOpr *pModule;

	list_for_each_entry(pModule, &display_list, list)
	{
		if (pModule->use_as_default)
			return pModule;
	}

	printf("no sub module ERROR\n");
	return NULL;

}

int GetDispResolution(int *piXres, int *piYres, int *piBpp)
{
	struct DispOpr *pModule;

	pModule = get_default_module();
	*piXres = pModule->iXres;
	*piYres = pModule->iYres;
	*piBpp = pModule->iBpp;
	return 0;
}

/* 设置framebuffer */
int GetVideoBufForDisplay(struct VideoBuf *ptFrameBuf)
{
	struct DispOpr *pModule;

	pModule = get_default_module();

	ptFrameBuf->iPixelFormat = (pModule->iBpp == 16) ? V4L2_PIX_FMT_RGB565 : \
							   (pModule->iBpp == 32) ? V4L2_PIX_FMT_RGB32 : \
							   0;

	ptFrameBuf->tPixelDatas.iWidth = pModule->iXres;
	ptFrameBuf->tPixelDatas.iHeight = pModule->iYres;
	ptFrameBuf->tPixelDatas.iBpp = pModule->iBpp;
	ptFrameBuf->tPixelDatas.iLineBytes = pModule->iLineWidth;
	ptFrameBuf->tPixelDatas.iTotalBytes = ptFrameBuf->tPixelDatas.iLineBytes * ptFrameBuf->tPixelDatas.iHeight;

	/* 显存地址 */
	ptFrameBuf->tPixelDatas.aucPixelDatas = pModule->pucFbMem;

	return 0;
}

/* 调用各个子模块的初始化函数 */
void display_modules_init(void)
{
	struct DispOpr *pModule;

	list_for_each_entry(pModule, &display_list, list)
	{
		if (pModule->DeviceInit)
			pModule->DeviceInit(pModule);

		if (pModule->CleanScreen)
			pModule->CleanScreen(pModule, 0);
	}
}

void choose_default_display_module(const char *name)
{
	struct DispOpr *pModule;

	list_for_each_entry(pModule, &display_list, list)
		if (!strcmp(name, pModule->name))
			pModule->use_as_default = 1;
}

void FlushPixelDatasToDev(PT_PixelDatas ptPixelDatas)
{
	struct DispOpr *pModule;

	pModule = get_default_module();

    pModule->ShowPage(pModule, ptPixelDatas);
}

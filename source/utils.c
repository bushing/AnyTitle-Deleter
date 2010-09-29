/*-------------------------------------------------------------
 
utils.c -- utility functions
 
Copyright (C) 2008 tona
Copyright (C) 2010 bushing
 
This software is provided 'as-is', without any express or implied
warranty.  In no event will the authors be held liable for any
damages arising from the use of this software.
 
Permission is granted to anyone to use this software for any
purpose, including commercial applications, and to alter it and
redistribute it freely, subject to the following restrictions:
 
1.The origin of this software must not be misrepresented; you
must not claim that you wrote the original software. If you use
this software in a product, an acknowledgment in the product
documentation would be appreciated but is not required.
 
2.Altered source versions must be plainly marked as such, and
must not be misrepresented as being the original software.
 
3.This notice may not be removed or altered from any source
distribution.
 
-------------------------------------------------------------*/

#include <stdio.h>
#include <gccore.h>
#include <wiiuse/wpad.h>
#include <stdarg.h>

#include "utils.h"
#include "name.h"

#define MAX_WIIMOTES 4

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

#define HAVE_AHBPROT ((*(vu32*)0xcd800064 == 0xFFFFFFFF) ? 1 : 0)
#define MEM_REG_BASE 0xd8b4000
#define MEM_PROT (MEM_REG_BASE + 0x20a)

const u16 isfs_old[] = {0x428B, 0xD001, 0x2566};

u32 patch_isfs(void) {
    u32 count = 0;
    if (!HAVE_AHBPROT) {
	printf("Sorry, AHBPROT not enabled!\n");
	return 0;
    }

    write16(MEM_PROT, 0);
    u16 *ptr;
    for (ptr= (u16 *)0x93400000; ptr < (u16 *)0x94000000; ptr ++)
	if (ptr[0] == isfs_old[0] && ptr[1] == isfs_old[1] && ptr[2] == isfs_old[2]) {
		ptr[1] = 0xE001;
		DCFlushRange(ptr, 64);
		count++;
		break;
	}
    write16(MEM_PROT, 1);

    return count;
}

/* Basic init taken pretty directly from the libOGC examples */
void basicInit(void)
{
	// Initialise the video system
	VIDEO_Init();
	
	// This function initialises the attached controllers
	WPAD_Init();
	
	// Obtain the preferred video mode from the system
	// This will correspond to the settings in the Wii menu
	rmode = VIDEO_GetPreferredMode(NULL);

	// Allocate memory for the display in the uncached region
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	
	// Initialise the console, required for printf
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
	
	// Set up the video registers with the chosen mode
	VIDEO_Configure(rmode);
	
	// Tell the video hardware where our display memory is
	VIDEO_SetNextFramebuffer(xfb);
	
	// Make the display visible
	VIDEO_SetBlack(FALSE);

	// Flush the video register changes to the hardware
	VIDEO_Flush();

	// Wait for Video setup to complete
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();

	// The console understands VT terminal escape codes
	// This positions the cursor on row 2, column 0
	// we can use variables for this with format codes too
	// e.g. printf ("\x1b[%d;%dH", row, column );
	printf("\x1b[2;0H");
}

void miscDeInit(void)
{
	fflush(stdout);
	ISFS_Deinitialize();
	freeDatabase();
}

u32 getButtons(void)
{
	WPAD_ScanPads();
	//int i;
	//for (i=0;i<MAX_WIIMOTES;i++)
	return WPAD_ButtonsDown(0);
}

u32 wait_anyKey(void) {
	u32 pressed;
	while(!(pressed = getButtons())) {
		VIDEO_WaitVSync();
	}
	return pressed;
}

u32 wait_key(u32 button) {
	u32 pressed;
	do {
		VIDEO_WaitVSync();
		pressed = getButtons();
	} while(!(pressed & button));
	return pressed;
}

s32 Uninstall(u32 title_u, u32 title_l)
{
	s32 ret;
	char filepath[256];
	sprintf(filepath, "/title/%08x/%08x",  title_u, title_l);
	
	/* Remove title */
	printf("\t\t- Deleting title dir %s...", filepath);
	fflush(stdout);

	ret = ISFS_Delete(filepath);
	if (ret < 0)
		printf("\n\tError! ISFS_Delete(ret = %d)\n", ret);
	else
		printf(" OK!\n");

	sprintf(filepath, "/ticket/%08x/%08x.tik", title_u, title_l);
	
	/* Delete ticket */
	printf("\t\t- Deleting ticket file %s...", filepath);
	fflush(stdout);

	ret = ISFS_Delete(filepath);
	if (ret < 0)
		printf("\n\tTicket delete failed (No ticket?) %d\n", ret);
	else
		printf(" OK!\n");

	return ret;
}

#define MAX_TITLES 256
u32 __titles_init = 0;
u32 __num_titles = 0;

typedef struct {
	u64 id;
	u32 version;
	u16 group;
	u8 ios_used;
	u32 is_fakesigned;
	u32 num_dependencies;
	u64 dependencies[4];
} title_info_struct;

title_info_struct title_info[MAX_TITLES];

static u64 __title_list[MAX_TITLES] ATTRIBUTE_ALIGN(32);

static s32 __getTitles(void) {
	s32 ret;
	ret = ES_GetNumTitles(&__num_titles);
	if (ret <0)
		return ret;
	if (__num_titles > MAX_TITLES)
		return -1;
	ret = ES_GetTitles(__title_list, __num_titles);
	if (ret<0)
		return ret;
		
	int i,j;
	for (i=0; i<__num_titles; i++) {
		signed_blob *s_tmd = getTMD(__title_list[i]);
		if (s_tmd == NULL) {
			printf("couldn't read TMD for %llx\n", __title_list[i]);
//			wait_anyKey();
			continue;
		}
		tmd *tmd = SIGNATURE_PAYLOAD(s_tmd);
		title_info[i].id = __title_list[i];
		title_info[i].version = tmd->title_version;
		title_info[i].group = tmd->group_id;
		title_info[i].ios_used = tmd->sys_version & 0xFF;
		title_info[i].num_dependencies = 0;
		memset(title_info[i].dependencies, 0, sizeof title_info[i].dependencies);

		// crude fakesigning check -- only detects if a signature is zeroed out.
		// Does not check signature validity or contents against TMD.
		u32 *tmd_sig = (u32 *)s_tmd;
		title_info[i].is_fakesigned = (tmd_sig[4] == 0) ? 1:0;
		free(s_tmd);
	}
	
	// go back and assign dependencies
	for (i=0; i<__num_titles; i++) {
		if (title_info[i].ios_used != 0) {
			for (j=0; j<__num_titles; j++) {
				if (title_info[i].ios_used == (__title_list[j] & 0xFF)) break;
			}
			if (j < __num_titles) {
				if (title_info[j].num_dependencies < (sizeof title_info[0].dependencies/sizeof title_info[0].dependencies[0]))
					title_info[j].dependencies[title_info[j].num_dependencies] = __title_list[i];
				title_info[j].num_dependencies++;
			}
		}
	}

	__titles_init = 1;
	return 0;
}

u16 get_title_group(u64 titleid) {
	int i;
	for(i=0; i<__num_titles; i++)
		if (__title_list[i] == titleid) return title_info[i].group;
	return 0;
}

u32 get_title_num_dependencies(u64 titleid) {
	int i;
	for(i=0; i<__num_titles; i++)
		if (__title_list[i] == titleid) return title_info[i].num_dependencies;
	return 0;
}

void get_title_dependencies(u64 titleid, u64 *buf, u32 max_deps) {
	int i;
	for(i=0; i<__num_titles; i++)
		if (__title_list[i] == titleid)
			memcpy(buf, title_info[i].dependencies, max_deps * sizeof(title_info[0].dependencies[0]));
}

// return first tid for which group = 'HB'
u64 get_hbc_tid(void) {
	int i;
	for(i=0; i<__num_titles; i++)
		if (title_info[i].group == 0x4842) return __title_list[i];
	return 0;
}

u32 is_title_fakesigned(u64 titleid) {
	int i;
	for(i=0; i<__num_titles; i++)
		if (__title_list[i] == titleid) return title_info[i].is_fakesigned;
	return 0;
}

s32 getTitles_TypeCount(u32 type, u32 *count) {
	s32 ret = 0;
	u32 type_count;
	if (!__titles_init)
		ret = __getTitles();
	if (ret <0)
			return ret;
	int i;
	type_count = 0;
	for (i=0; i < __num_titles; i++) {
		u32 upper;
		upper = __title_list[i] >> 32;
		if(upper == type)
			type_count++;
	}
	*count = type_count;
	return ret;
}
	
s32 getTitles_Type(u32 type, u32 *titles, u32 count) {
	s32 ret = 0;
	u32 type_count;
	if (!__titles_init)
		ret = __getTitles();
	if (ret <0)
			return ret;
	int i;
	type_count = 0;
	for (i=0; type_count < count && i < __num_titles; i++) {
		u32 upper, lower;
		upper = __title_list[i] >> 32;
		lower = __title_list[i] & 0xFFFFFFFF;
		if(upper == type) {
			titles[type_count]=lower;
			type_count++;
		}
	}
	if (type_count < count)
		return -2;
	__titles_init = 0;
	return 0;
}

s32 getapp(u64 req, u32 *outBuf)
{
	//gprintf("\n\tgetBootIndex(%016llx)",req);
	u32 tmdsize;
	u64 tid = 0;
	u64 *list;
	u32 titlecount;
	s32 ret;
	u32 i;

	ret = ES_GetNumTitles(&titlecount);
	if (ret < 0) return WII_EINTERNAL;

	list = memalign(32, titlecount * sizeof(u64) + 32);
	if (list == NULL) return WII_EINTERNAL;
	
	ret = ES_GetTitles(list, titlecount);
	if (ret < 0) {
		free(list);
		return WII_EINTERNAL;
	}

	for(i=0; i<titlecount; i++)
		if (list[i]==req) {
			tid = list[i];
			break;
		}

	free(list);

	if(!tid) return WII_EINSTALL;

	if(ES_GetStoredTMDSize(tid, &tmdsize) < 0) return WII_EINSTALL;

	signed_blob *s_tmd = getTMD(tid);
	
	if (s_tmd == NULL) return WII_EINSTALL;

	tmd *tmd = SIGNATURE_PAYLOAD(s_tmd);

	tmd_content *content = &tmd->contents[0];
	*outBuf = content->cid;
	free(s_tmd);
	return 1;
}

#define TITLEID_SYSMENU 0x0000000100000002ULL
#define TMDVIEW_OFFSET_IOS 5
#define TMDVIEW_OFFSET_VER 0x2c

#define error(format, args...)  \
	do { printf (format , ## args); \
	wait_anyKey(); \
	return 0; } while (0)

static u8 do_tmdview_stuff(u64 title_id, u16 *title_version, u16 *title_ios)
{
	s32 ret;	
	static u16 tmdview[0x100] ATTRIBUTE_ALIGN(32);
	u32 tmdview_size = 0;

	ret = ES_GetTMDViewSize(title_id, &tmdview_size);
	if (ret < 0) error("Error! ES_GetTMDViewSize(%llx) (ret = %d)\n", title_id, ret);

	if (tmdview_size > sizeof tmdview)
		error("Error! TMDview too big (%d > %d)\n", tmdview_size, sizeof tmdview);

	ret = ES_GetTMDView(title_id, (u8 *)tmdview, tmdview_size);
	if (ret < 0) error("Error! ES_GetTMDView(%llx) (ret = %d)\n", title_id, ret);

	if (title_version) *title_version = tmdview[TMDVIEW_OFFSET_VER];
	if (title_ios) *title_ios = tmdview[TMDVIEW_OFFSET_IOS];
	return 1;
}

u64 get_title_ios(u64 title) {
	u16 ios_version = 0;
	do_tmdview_stuff(title, NULL, &ios_version);
	if (ios_version != 0) return 1ULL << 32 | ios_version;
	return 0;
}

/* Get Sysmenu Region identifies the region of the system menu (not your Wii)
  by looking into it's resource content file for region information. */
u8 get_sysmenu_region(void)
{
	u16 title_version = 0;
	do_tmdview_stuff(TITLEID_SYSMENU, &title_version, NULL);
	printf("title_version = %hu\n", title_version);

	switch (title_version & 0xF) {
	case 0:	return 'J';
	case 1:	return 'E';
	case 2:	return 'P';
	case 6:	return 'K';
	default:return 0;
	}
}

u16 get_title_version(u64 tid) {
	u16 title_version = 0;
	do_tmdview_stuff(tid, &title_version, NULL);
	return title_version;
}

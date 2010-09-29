/*-------------------------------------------------------------
 
utils.h -- basic Wii initialization and functions
 
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

#ifndef _UTILS_H_
#define _UTILS_H_

// Turn upper and lower into a full title ID
#define TITLE_ID(x,y)		(((u64)(x) << 32) | (y))
// Get upper or lower half of a title ID
#define TITLE_UPPER(x)		((u32)((x) >> 32))
// Turn upper and lower into a full title ID
#define TITLE_LOWER(x)		((u32)(x))

// Do basic Wii init: Video, console, WPAD
void basicInit(void);

// Do our custom init: Identify and initialized ISFS driver
void miscInit(void);

// Clean up after ourselves (Deinit ISFS)
void miscDeInit(void);

// Scan the pads and return buttons
u32 getButtons(void);

u32 wait_anyKey(void);

u32 wait_key(u32 button);

int Uninstall(u32 tid_h, u32 tid_l);

//Get the IOS version of a given title
u64 get_title_ios(u64 title);

//Get the region that the System menu is currently using
u8 get_sysmenu_region(void);

//Compare system region and sysmenu region, display warning.
void check_region(void);

u16 get_title_version(u64 tid);

// Get the number of titles on the Wii of a given type
s32 getTitles_TypeCount(u32 type, u32 *count);

// Get the list of titles of this type
s32 getTitles_Type(u32 type, u32 *titles, u32 count);

//get the first .app file for a title.  this probable has the name and shit in it
s32 getapp(u64 req, u32 *outBuf);

u16 get_title_group(u64 titleid);
u32 get_title_num_dependencies(u64 titleid);
void get_title_dependencies(u64 titleid, u64 *buf, u32 max_deps);
u64 get_hbc_tid(void);

u32 is_title_fakesigned(u64 titleid);
u32 patch_isfs(void);

#endif

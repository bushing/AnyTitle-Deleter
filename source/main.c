/*-------------------------------------------------------------
 
main.c -- main and menu code.
 
Copyright (C) 2009 tona, MrClick
Copyright (C) 2010 bushing
Unless other credit specified
 
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

// Uncomment the next line to do a hard reset on exit
//#define REBOOT
// Comment out the next line to disable brick protection !!!RISKY!!!
#define BRICK_PROTECTION	

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

#include "name.h"
#include "utils.h"

#define TITLE_ID(x,y)           (((u64)(x) << 32) | (y))
#define TITLE_UPPER(x)          ((u32)((x) >> 32))
#define TITLE_LOWER(x)          ((u32)(x))

/* Constants */
// ITEMS_PER_PAGE must be evenly divisible by COLUMNS
#define ITEMS_PER_PAGE 		23
#define COLUMNS 			1
#define ROWS				(ITEMS_PER_PAGE / COLUMNS)
#define MAX_TITLES 			(ITEMS_PER_PAGE * 12)
#define NUM_TYPES 			7

static char *help_text[] = {
	"\t+-- Credits ------------------------------------------------------------+\n",
	"\t AnyTitle Deleter v1.1  by tona, MrClick, Red Squirrel, giantpune, etc.\n",
	"\t+-----------------------------------------------------------------------+\n",
	"\n",
	"\t+-- Controls -----------------------------------------------------------+\n",
	"\t (DPAD) Browse | (+/-) Change Page | (A) Select | (B) Back | (HOME) Exit \n",
	"\t (1) Change naming mode | (2) Change display mode                        \n",
	"\t+-----------------------------------------------------------------------+\n",
//	printf("\t Database: Loaded %d entries    |    IOS: %d\n", getDatabaseCount(), IOS_GetVersion);
	"\t                                                                         \n", // status text goes here
	"\t+-----------------------------------------------------------------------+\n"
};

u8 system_region;
u32 page = 0, selected = 0, menu_index = 0, num_titles, max_selected;
static u32 titles[MAX_TITLES] ATTRIBUTE_ALIGN(32);
static char page_header[2][80];
static char page_contents[ITEMS_PER_PAGE][80];
u32 types[] = {1, 0x10000, 0x10001, 0x10002, 0x10004, 0x10005, 0x10008};
bool exit_now = false;

void uninstallChecked(u32 kind, u32 title) {
	u64 tid = TITLE_ID(kind, title);
	u64 sysmenu_ios = get_title_ios(TITLE_ID(1, 2));
	u16 title_version = get_title_version(tid);
	u8 title_ios = get_title_ios(tid) & 0xFF;;
	char bufdb[256] __attribute__ ((aligned (32)));
	char bufbn[256] __attribute__ ((aligned (32)));
	char buf00[256] __attribute__ ((aligned (32)));
	bool has_db = false, has_bn = false, has_00 = false;
	
	if(getNameDB(bufdb, titleText(kind, title)) >= 0)
		has_db = true;
	if(getNameBN(bufbn, tid) >= 0)
		has_bn = true;
	if(getName00(buf00, tid) >= 0)
		has_00 = true;
	
	printf("\x1b[2J\n\n");
	#ifndef BRICK_PROTECTION
	printf("WARNING!!! BRICK PROTECTION DISABLED!! WARNING!!! BRICK PROTECTION DISABLED!!\n");
	#endif
	printf("+- Title -------------------------------------------------------------------+\n");
	if (has_db)
	printf(" Name DB  : %s\n", bufdb);
	if (has_bn)
	printf(" Name BAN : %s\n", bufbn);
	if (has_00)
	printf(" Name APP : %s\n", buf00);
	printf(" Title ID : %s (%08x/%08x)\n", titleText(kind, title), kind, title);
	if (title_ios)
	printf(" IOS req. : %hhu\n", title_ios);
	printf(" Version  : %hu\n", title_version);
	printf(" Group    : %04hx\n", get_title_group(tid));
	int num_deps = get_title_num_dependencies(tid);
	if (num_deps > 0) {
		u64 deps[4];
		get_title_dependencies(tid, deps, 4);
		printf(" Used by  : ");
		int i;
		for (i=0; i < num_deps && i < 4; i++) printf("%llx, ", deps[i]);
		if (num_deps > 4) printf("etc.");
		printf("\n");
	}
	printf(" # Deps   : %d\n", get_title_num_dependencies(tid));
	printf("Fakesigned: %s\n", is_title_fakesigned(tid)?"yes":"no");
	printf("+---------------------------------------------------------------------------+\n");
	printf("+- Contents ----------------------------------------------------------------+\n");
	printContent(tid);
	printf("+---------------------------------------------------------------------------+\n\n");
	
	
	#ifdef BRICK_PROTECTION
	// Don't uninstall System titles if we can't find sysmenu IOS
	if (kind == 1){
		if (sysmenu_ios == 0) {
			printf("\tSafety Check! Can't detect Sysmenu IOS, system title deletes disabled\n");
			printf("\tPlease report this to the author\n");
			printf("\tPress any key...\n\n");
			wait_anyKey();
			return;
		} 
	}
	
	// Don't uninstall hidden channel titles if we can't find sysmenu region
	if (kind == 0x10008){	
		if (system_region == 0) {
			printf("\tSafety check! Can't detect Sysmenu Region, hidden channel deletes disabled\n");
			printf("\tPlease report this to the author\n");
			printf("\tPress any key...\n\n");
			wait_anyKey();
			return;
		}
	}
	
	// Fail for uninstalls of various titles.
	
	if (tid ==TITLE_ID(1, 1))
		printf("\tBrick protection! Can't delete boot2!\n");
	else if (tid == TITLE_ID(1, 2))
		printf("\tBrick protection! Can't delete System Menu!\n");
	else if(tid == sysmenu_ios) 
		printf("\tBrick protection! Can't delete Sysmenu IOS!\n");
	else if(tid  == TITLE_ID(0x10008, 0x48414B00 | system_region))
		printf("\tBrick protection! Can't delete your region's EULA!\n");
	else if(tid  == TITLE_ID(0x10008, 0x48414C00 | system_region))
		printf("\tBrick protection! Can't delete your region's rgnsel!\n");
	else {
	#endif	
		
		// Display a warning if you're deleting the current IOS, although it won't break operation to delete it
		if (title == IOS_GetVersion())
			printf("\tWARNING: This is the currently loaded IOS!\n\t- However, deletion should not affect current usage.\n");
		
		// Display a warning if you're deleting the Homebrew Channe's IOS
		if (tid == get_title_ios(get_hbc_tid()))
			printf("\tWARNING: This is the IOS used by the Homebrew Channel!\n\t- Deleting it will cause the Homebrew Channel to stop working!\n");
		
		printf("\t               (A) Uninstall this title\n");
		if(has_bn || has_00)
		printf("\t               (+) Add this title to the database\n");
		printf("\t               (B) Abort\n\n");
		u32 key = wait_key(WPAD_BUTTON_A | WPAD_BUTTON_B | WPAD_BUTTON_PLUS);
		if (key & WPAD_BUTTON_B)
			return;
		else if (key & WPAD_BUTTON_PLUS && (has_bn || has_00)) {
			if (has_db) {
				printf("This title is already present in the database!\n");
				printf("To add it anyway press (A) or any other button to abort.\n");
				if (!(wait_anyKey() & WPAD_BUTTON_A))
					return;
			}
			if (has_bn)
				addTitleToDatabase(titleText(kind, title), bufbn);
			else
				addTitleToDatabase(titleText(kind, title), buf00);
		}
		else {
			Uninstall(tid >> 32, tid & 0xFFFFFFFF);	
		}
	#ifdef BRICK_PROTECTION
	}
	#endif
	printf("\tPress any key...\n\n");
	wait_anyKey();
}


/* Double-caching menu code was made in an attempt to improve performance 
  In the end, my input lag was caused by something entirely different, 
  but I didn't really want to undo the whole menu */


void printMenuList(void){
	//Shove the headers out
	printf("%s\n", page_header[0]);
	printf("%s\n", page_header[1]);
	printf("\t+------------------------------------------------- (%-5s) (%-9s) -+\n",
			getDispModeName(),
			getNamingMode());
	
	if (menu_index == 0){
		// If we're on the main index, just shove out each row
		int i;
		for (i = 0; i < NUM_TYPES + sizeof help_text / sizeof help_text[0]; i++)
			printf(page_contents[i]);
		return;
		
	}
	
	if (num_titles) {
		// If we're on a page with titles, print all columns and rows
		int i;		
		for (i = 0; i < ROWS; i++){
			int j;
			for (j = 0; j < COLUMNS; j++)
				printf(page_contents[i+(j*ROWS)]);
			printf("\n");
		}
		
	} else {
		// Or a blank page
		printf("\tNo titles to display\n");
	}
	
	printf("\t+-----------------------------------------------------------------------+\n");
}

void updateSelected(int delta) {
	if (delta == 0) {
		// Set cursor location to 0
		selected = 0;
	} else {
		// Remove the cursor from the last selected item
		page_contents[selected][1] = ' ';
		page_contents[selected][2] = ' ';
		// Set new cursor location
		selected += delta;
	}
	
	// Add the cursor to the now-selected item
	page_contents[selected][1] = '-';
	page_contents[selected][2] = '>';
}

void updatePage(void){
	if (menu_index == 0){
		// On the main index...
		// Set our max-tracking variables to the number of items on this menu
		max_selected = num_titles = NUM_TYPES;
		
		// Fill headers and page contents
		strcpy(page_header[0], "\tTitles:");
		strcpy(page_header[1], "");
		strcpy(page_contents[0], "    00000001 - System Titles\n");
		strcpy(page_contents[1], "    00010000 - Disc Game Titles (and saves)\n");
		strcpy(page_contents[2], "    00010001 - Installed Channel Titles\n");
		strcpy(page_contents[3], "    00010002 - System Channel Titles\n");
		strcpy(page_contents[4], "    00010004 - Games that use Channels (Channel+Save)\n");
		strcpy(page_contents[5], "    00010005 - Downloadable Game Content\n");
		strcpy(page_contents[6], "    00010008 - Hidden Channels\n");
		int i;
		for(i=0; i < sizeof(help_text) / sizeof(help_text[0]); i++)
			strcpy(page_contents[7+i],help_text[i]);
	} else {
		// For every other page....
		
		// Fill the headers
		sprintf(page_header[0], "\tTitles in %08x:", types[menu_index-1]);
		strcpy(page_header[1], "\tPage: ");
		if (num_titles) {
			// If there's any contents...
			int i;
			
			// Figure out where our page is starting
			int page_offset = page * ITEMS_PER_PAGE;
			
			// And the highest we can select
			if ((page+1) * ITEMS_PER_PAGE <= num_titles)
				max_selected = ITEMS_PER_PAGE;
			else
				max_selected = num_titles % ITEMS_PER_PAGE;
			
			// Fill the "Page" header with our number of pages
			for (i = 0; i <= ((num_titles - 1) / ITEMS_PER_PAGE); i++)
				if(i == page)
					sprintf(page_header[1], "%s(%d) ", page_header[1], i+1);
				else
					sprintf(page_header[1], "%s %d  ", page_header[1], i+1);
			
			// Determine maximal length of titleText on current page
			i = 0;
			u32 max_titlelen = 0;
			while (i < max_selected) {
				u32 titlelen = strlen(titleText(types[menu_index-1], titles[page_offset+i]));
				if (titlelen > max_titlelen)
					max_titlelen = titlelen;
				i++;
			}
			
			// Fill the cached page contents with each title entry		
			i = 0;
			while (i < max_selected){
				u32 s = sizeof(page_contents[i]);
				u8 mode = getDispMode();
	
				char name[256];
				char text[15];
				
				sprintf(text, "%s", titleText(types[menu_index-1], titles[page_offset+i]));
				getTitle_Name(name, TITLE_ID(types[menu_index-1], titles[(page*ITEMS_PER_PAGE)+i]), text);
				snprintf(page_contents[i], s,  "   %c",
					is_title_fakesigned(TITLE_ID(types[menu_index-1], titles[(page*ITEMS_PER_PAGE)+i])) ? '!' : ' ');
				
				if (mode > 0) {
					snprintf(page_contents[i], s,  "%s%s ", page_contents[i], text);
					u8 sp = max_titlelen - strlen(text);
					for (; sp > 0; sp--)
						strncat(page_contents[i], " ", s);
				}
				
				if (mode > 1)
					snprintf(page_contents[i], s, "%s(%08x) ", page_contents[i], titles[page_offset+i]);
				
				if (mode != 0)
					snprintf(page_contents[i], s,  "%s: ", page_contents[i]);
				
				strncat(page_contents[i], name, s - strlen(page_contents[i]) - 1); 
				// Cut off line to prevent spilling of characters to the next line
				page_contents[i][76] = 0;
				int space = 76 - strlen(page_contents[i]);
				if (space > 0) memset(&page_contents[i][strlen(page_contents[i])], ' ', space);
				if (is_title_fakesigned(TITLE_ID(types[menu_index-1], titles[(page*ITEMS_PER_PAGE)+i])))
					memcpy(&page_contents[i][60], "(fakesigned)", 12);
				i++;
			}
			// And fill the rest of the slots with whitespace
			while (i < ITEMS_PER_PAGE){
				strcpy(page_contents[i], "                                            ");
				i++;
			}
			
		} else {
			max_selected = 0;
		}
	}
	updateSelected(0);
}


void updateTitleList(void){
	// Sanity check to make sure we're on a title page
	if (menu_index > 0){
		s32 ret;
		
		// Get count of titles of our requested type
		ret = getTitles_TypeCount(types[menu_index-1], &num_titles);
		if (ret < 0){
			printf("\tError! Can't get count of titles! (ret = %d)\n", ret);
			exit(1);
		}
		
		// Die if we can't handle this many
		if (num_titles > MAX_TITLES){
			printf("\tError! Too many titles! (%u)\n", num_titles);
			exit(1);
		}
		
		// Get titles of our requested type
		ret = getTitles_Type(types[menu_index-1], titles, num_titles);
		if (ret < 0){
			printf("\tError! Can't get list of titles! (ret = %d)\n", ret);
			exit(1);
		}
	}
	// Now generate the page
	updatePage();
}


void parseCommand(u32 pressed){
	
	// B: Load Index menu
	if (pressed & WPAD_BUTTON_B){
		menu_index = 0;
		page = 0;
		updatePage();
	}
	
	// Left/right: Next page, last page.
	if ((pressed & WPAD_BUTTON_PLUS) || (pressed & WPAD_BUTTON_RIGHT))
		if ((page+1) * ITEMS_PER_PAGE < num_titles) {
			page++;
			updatePage();
		}
	if ((pressed & WPAD_BUTTON_MINUS) || (pressed & WPAD_BUTTON_LEFT)) 
		if (page > 0) {
			page--;
			updatePage();
		}
	
	// Directional controls
	if (pressed & WPAD_BUTTON_UP){
		if (selected == 0)
			updateSelected(max_selected - 1);
		else if (selected > 0) 
			updateSelected(-1);
	} else if (pressed & WPAD_BUTTON_DOWN){
		if (selected + 1 == max_selected) 
			updateSelected(-max_selected + 1);
		else if (selected + 1 < max_selected) 
			updateSelected(1);
	}
			
	// Uninstall selected or enter menu
	if (pressed & WPAD_BUTTON_A){
		if (menu_index == 0) {
			menu_index = selected+1;
		}
		else if (selected < max_selected) {
			uninstallChecked(types[menu_index-1], titles[(page*ITEMS_PER_PAGE)+selected]);
		}
		updateTitleList();
	}
	
	// Change the naming mode
	if (pressed & WPAD_BUTTON_1){
		changeNamingMode();
		updatePage();
	}
	
	// Change the display mode
	if (pressed & WPAD_BUTTON_2){
		changeDispMode();
		updatePage();
	}
	
	// Add name to database.txt
	if ((pressed & WPAD_BUTTON_PLUS)){
		//addToDatabase(TITLE_ID(types[menu_index-1], titles[(page*ITEMS_PER_PAGE)+i]), titleText(titles[page_offset+i]));
	}

}


// Make sure we're getting the correct system menu region
void checkRegion(void){
	s32 sysmenu_region, conf_region;
	
	conf_region = CONF_GetRegion();
	
	printf("\n\tChecking system region...\n");
	
	system_region = get_sysmenu_region();
	switch(system_region){
		case 'E':
			sysmenu_region = CONF_REGION_US;
			break;
		case 'J':
			sysmenu_region = CONF_REGION_JP;
			break;
		case 'P':
			sysmenu_region = CONF_REGION_EU;
			break;
		case 'K':
			sysmenu_region = CONF_REGION_KR;
			break;
		default:
			printf("\tRegion detection failure! %d\n", system_region);
			printf("\tConf region: %d", conf_region);
			wait_anyKey();
			return;
			break;
	}
	printf("\tRegion properly detected as %c\n", system_region);

	if (sysmenu_region != conf_region){
		printf("\tYour System Menu and SYSCONF regions differ. (semi brick?)\n");
		printf("\tSysmenu: %d  Sysconf: %d\n", sysmenu_region, conf_region);
		printf("\tProceed with caution! (Using Sysmenu region)\n");
		wait_anyKey();
	}
}

int main(void) {
	ISFS_Initialize();
	basicInit();	
	checkRegion();
	updatePage();
	//geckoinit =InitGecko();
	
	/* Main loop - Menu basics very loosely adapted from Waninkoko's WAD Manager 1.0 source */
	
	u32 pressed = 0;
	
	// Wait to see init messages
	//wait_anyKey();

	snprintf(help_text[8], strlen(help_text[8]), "\t Database: Loaded %d entries    |   Running under IOS%dv%d\n",
		getDatabaseCount(), IOS_GetVersion(), IOS_GetRevision());
	/* Print welcome message */
	printf("\x1b[2J\n\n");
	int i;
	for(i=0; i < sizeof help_text / sizeof help_text[0]; i++) printf("%s", help_text[i]);
	// Wait to see welcome messages
	
	printf("\n\tPatching IOS in memory...");
	fflush(stdout);

	printf("Applied %d patches\n", patch_isfs());
	printf("\tLoading database... ");
	fflush(stdout);
	
	int ret = loadDatabase();
	if (ret < 0)
		printf("Failed!!!\n");
	else
		printf("OK!\n");
		
	wait_anyKey();
	
	
	#ifndef BRICK_PROTECTION
	printf("\x1b[2J\n\n");
	printf("WARNING!!! BRICK PROTECTION DISABLED!! WARNING!!! BRICK PROTECTION DISABLED!!\n\n");
	printf("+---------------------------------------------------------------------------+\n");
	printf(" This copy of AnyTitle Deleter was compiled without protection against    \n");
	printf(" deleting important parts of your Wii system software. You might damage your \n");
	printf(" Wii beyond repair if you delete certain titles resulting in a semi or even  \n");
	printf(" FULL BRICK. If you do not fully understand this warning or are uncertain    \n");
	printf(" about the next steps you are about to take                                  \n\n");
	printf("                   ABORT BY PRESSING THE (A) BUTTON                          \n\n");
	printf(" You can get a version of this program with brick protection enabled at      \n");
	printf("            http://wiibrew.org/wiki/AnyTitle_Deleter                      \n");
	printf(" If you really want to continue, you know what you are doing and you are     \n");
	printf(" aware that the authors of this software cannot be held responsible for any  \n");
	printf(" damage you might inflict on your Wii's software by the use of AnyTitle      \n");
	printf(" Deleter, fell free to continue by pressing the Minus button.                \n");
	printf("+---------------------------------------------------------------------------+\n\n");	
	printf("WARNING!!! BRICK PROTECTION DISABLED!! WARNING!!! BRICK PROTECTION DISABLED!!\n");
	if (wait_key(WPAD_BUTTON_A | WPAD_BUTTON_MINUS ) & WPAD_BUTTON_A)
		exit_now = true;
	#endif
	
	
	do{
		if (exit_now)
			break;
		
		if (pressed)
			parseCommand(pressed);

		/* Wait for vertical sync */
		VIDEO_WaitVSync();
	
		/* Clear screen */
		printf("\x1b[2J\n\n");
	
		/* Print item list */
		printMenuList();
		
		/* Controls */
		pressed = wait_anyKey();
		
	} while(!(pressed & WPAD_BUTTON_HOME));
	
	/* Outro */
	printf("\tThanks for playing!\n");
	
	miscDeInit();
	
	#ifdef REBOOT
	SYS_ResetSystem(SYS_RESTART,0,0);
	#endif
	
	exit(0);
}

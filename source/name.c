/*-------------------------------------------------------------
 
name.c -- functions for determining the name of a title
 
Copyright (C) 2009 MrClick
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

#include "name.h"
#include "utils.h"

// Max number of entries in the database
#define MAX_DB 		1024

// Max name length
#define MAX_LINE	80

// Contains all title ids (e.g.: "HAC")
char **__db_i;
// Contains all title names (e.g.: "Mii Channel")
char **__db;
// Contains the number of entries in the database
u32 __db_cnt = 0;
// The name of the database file
char dbfile[] = "/database.txt";

// Contains the selected mode for getTitle_Name()
u8 __selected_mode = 0;
// The names of the different modes for getTitle_Name()
const char __modeStrings[][10] = {
 "SD > NAND",
 "NAND > SD",
 " SD only ",
 "NAND only"
};

// Contains the selected display mode
u8 __selected_disp = 2;
// The names of the different display modes
const char __dispStrings[][10] = {
 "TITLE",
 "SHORT",
 "FULL "
};

int isdir(char *path)
{
	s32 res;
	u32 num = 0;

	res = ISFS_ReadDir(path, NULL, &num);
	if(res < 0)
	{
	    return -1;
	}

	return num;
}


s32 loadDatabase(void) {
	FILE *fdb;
	
	// Init SD card access, open database file and check if it worked
	fatInitDefault();
	fdb = fopen(dbfile, "r");
	if (fdb == NULL)
		return -1;
	
	// Allocate memory for the database
	__db_i = calloc(MAX_DB, sizeof(char*));
	__db = calloc(MAX_DB, sizeof(char*));
	
	// Define the line buffer. Each line in the db file will be stored here first
	char line[MAX_LINE];
	line[sizeof(line)-1] = 0;
	
	// Generic char buffer and counter variable
	char byte;
	u32 i = 0;
	
	// Read each character from the file
	do {
		byte = fgetc(fdb);
		// In case a line was longer than MAX_LINE
		if (i == -1){
			// Read bytes till a new line is hit
			if (byte == 0x0A)
				i = 0;
		// In case were still good with the line length
		} else {
			// Add the new byte to the line buffer
			line[i] = byte;
			i++;
			// When a new line is hit or MAX_LINE is reached
			if (byte == 0x0A || i == sizeof(line) - 1) {
				// Terminate finished line to create a string
				line[i] = 0;
				// When the line is not a comment or not to short
				if (line[0] != '#' && i > 5){
					
					// Allocate and copy title id to database
					__db_i[__db_cnt] = calloc(4, sizeof(char*));
					memcpy(__db_i[__db_cnt], line, 3);
					__db_i[__db_cnt][3] = 0;
					// Allocate and copy title name to database
					__db[__db_cnt] = calloc(i - 4, sizeof(char*));
					memcpy(__db[__db_cnt], line + 4, i - 4);
					__db[__db_cnt][i - 5] = 0;
					
					// Check that the next line does not overflow the database
					__db_cnt++;
					if (__db_cnt == MAX_DB)
						break;
				}
				// Set routine to ignore all bytes in the line when MAX_LINE is reached
				if (byte == 0x0A) i = 0; else i = -1;
			}
		}	
	} while (!feof(fdb));	
	
	// Close database file; we are done with it
	fclose(fdb);
	
	return 0;
}


void freeDatabase(void) {
	u32 i = 0;
	for(; i < __db_cnt; i++){
		free(__db_i[i]);
		free(__db[i]);
	}
	free(__db_i);
	free(__db);
}


s32 getDatabaseCount(void) {
	return __db_cnt;
}


s32 addTitleToDatabase(char *tid, char *name){
	char entry[128];
	snprintf(entry, sizeof(entry), "%3.3s-%s", tid, name);
	
	FILE *fdb;
	fatInitDefault();
	fdb = fopen(dbfile, "a");
	if (fdb == NULL)
		return -1;
	
	if (fseek(fdb, 0, SEEK_END) < 0)
		return -2;
	
	if (fwrite(entry, 1, strnlen(entry, 128), fdb)  < 0)
		return -3;
	
	if (fwrite("\n", 1, 1, fdb)  < 0)
		return -4;
	
	fclose(fdb);
	printf("Wrote \"%s\" to database\n", entry);
	return 0;
}


s32 getTitle_Name(char* name, u64 id, char *tid){
	char buf[256] __attribute__ ((aligned (32)));
				
	s32 r = -1;
	// Determine the title's name accoring to the selected naming mode
	switch (__selected_mode){
		case 0:	r = getNameDB(buf, tid);
				if (r < 0)
					r = getNameBN(buf, id);
					if (r < 0)
						r = getName00(buf, id);
				break;
		case 1:	r = getName00(buf, id);
				if (r < 0)
					r = getNameBN(buf, id);
					if (r < 0)
						r = getNameDB(buf, tid);
				break;
		case 2:	r = getNameDB(buf, tid);
				break;
		case 3:	r = getName00(buf, id);
				if (r < 0)
					r = getNameBN(buf, id);
				break;
	}

	//check to see if this is a disc TMD with no data associated with it
	//this sets noData to 1 if there is not actually anything in the data directory
	//and the file count in the folder is 1 (because its probably just the tmd).
	int noData=0;
	if (TITLE_UPPER(id)==0x10000)
	{
	    //gprintf("this is a disc based game");
	    char file[256] __attribute__ ((aligned (32)));
	    sprintf(file, "/title/%08x/%08x/data", (u32)(id >> 32), (u32)id);
	    int dir = isdir(file);
	    if (dir==0)
	    {
		sprintf(file, "/title/%08x/%08x/content", (u32)(id >> 32), (u32)id);
		dir = isdir(file);
		if (dir==1)
		{
		    sprintf(file, "/title/%08x/%08x/content/title.tmd", (u32)(id >> 32), (u32)id);
		    s32 fh = ISFS_Open(file, ISFS_OPEN_READ);
		    if(fh>0)
		    {
			ISFS_Close(fh);
			noData =1;
		    }

		}
	    }

	}

	switch (r){
		// In case a name was found in the database
		case 0:		sprintf(name, "%s%s",(noData?"tmd! ":""), buf);
					break;
		// In case a name was found in the banner.bin
		case 1:		sprintf(name, "*%s%s*",(noData?"tmd! ":""), buf);
					break;
		// In case a name was found in the 00000000.app
		case 2:		sprintf(name, "+%s%s+",(noData?"tmd! ":""), buf);
					break;
		// In case no proper name was found return a ?	
		default: 	sprintf(name, "%s(No name)",(noData?"tmd! ":""));
					break;
	}

	return 0;
}


s32 getNameDB(char* name, char* id){
	// Return fixed values for special entries
	if (strncmp(id, "IOS", 3) == 0){
		sprintf(name, "Operating System %s", id);
		return 0;
	}
	if (strncmp(id, "MIOS", 3) == 0){
		sprintf(name, "Gamecube Compatibility Layer");
		return 0;
	}
	if (strncmp(id, "SYSMENU", 3) == 0){
		sprintf(name, "System Menu");
		return 0;
	}
	if (strncmp(id, "BC", 2) == 0){
		sprintf(name, "BC");
		return 0;
	}
	
	// Create an ? just in case the function aborts prematurely
	sprintf(name, "?");
	
	u32 i;
	u8 db_found = 0;
	// Compare each id in the database to the title id
	for (i = 0; i < __db_cnt; i++)
		if (strncmp(id, __db_i[i], 3) == 0){
			db_found = 1;
			break;
		}
	
	if (db_found == 0)
		// Return -1 if no mathcing entry was found
		return -1;
	else {
		// Get name from database once a matching id was found	
		sprintf(name, __db[i]);
		return 0;
	}
}


s32 getNameBN(char* name, u64 id){
	// Terminate the name string just in case the function exits prematurely
	name[0] = 0;

	// Create a string containing the absolute filename
	char file[256] __attribute__ ((aligned (32)));
	sprintf(file, "/title/%08x/%08x/data/banner.bin", (u32)(id >> 32), (u32)id);
	
	// Bring the Wii into the title's userspace
//	if (ES_SetUID(id) < 0){
		// Should that fail repeat after setting permissions to system menu mode
//		Identify_SysMenu();
//		if (ES_SetUID(id) < 0)
//			return -1;
//	}
	
	// Try to open file
	s32 fh = ISFS_Open(file, ISFS_OPEN_READ);
	
	// If a title does not have a banner.bin bail out
	if (fh == -106)
		return -2;
	
	// If it fails try to open again after identifying as SU
	if (fh == -102){
//		Identify_SU();
		fh = ISFS_Open(file, ISFS_OPEN_READ);
	}
	// If the file won't open 
	else if (fh < 0)
		return fh;

	// Seek to 0x20 where the name is stored
	ISFS_Seek(fh, 0x20, 0);

	// Read a chunk of 256 bytes from the banner.bin
	u8 *data = memalign(32, 0x100);
	if (ISFS_Read(fh, data, 0x100) < 0){
		ISFS_Close(fh);
		free(data);
		return -3;
	}
	
	
	// Prepare the strings that will contain the name of the title
	char name1[0x41] __attribute__ ((aligned (32)));
	char name2[0x41] __attribute__ ((aligned (32)));
	name1[0x40] = 0;
	name2[0x40] = 0;

	__convertWiiString(name1, data + 0x00, 0x40);
	__convertWiiString(name2, data + 0x40, 0x40);
	free(data);
	
	// Assemble name
	sprintf(name, "%s", name1);
	if (strlen(name2) > 1)
		sprintf(name, "%s (%s)", name, name2);

	// Close the banner.bin
	ISFS_Close(fh);

	// Job well done
	return 1;
}

/*
s32 getName00(char* name, u64 id){
	// Create a string containing the absolute filename
	char file[256] __attribute__ ((aligned (32)));
	sprintf(file, "/title/%08x/%08x/content/00000000.app", (u32)(id >> 32), (u32)id);
	
	s32 fh = ISFS_Open(file, ISFS_OPEN_READ);
	
	
	
	// If the title does not have 00000000.app bail out
	if (fh == -106)
		return fh;
	
	// In case there is some problem with the permission
	if (fh == -102){
		// Identify as super user
		Identify_SU();
		fh = ISFS_Open(file, ISFS_OPEN_READ);
	}
	else if (fh < 0)
		return fh;
	
	// Jump to start of the name entries
	ISFS_Seek(fh, 0x9C, 0);

	// Read a chunk of 0x22 * 0x2B bytes from 00000000.app
	u8 *data = memalign(32, 2048);
	s32 r = ISFS_Read(fh, data, 0x22 * 0x2B);
	//printf("%s %d\n", file, r);wait_anyKey();
	if (r < 0){
		ISFS_Close(fh);
		free(data);
		return -4;
	}

	// Take the entries apart
	char str[0x22][0x2B];
	u8 i = 0;
	// Convert the entries to ASCII strings
	for(; i < 0x22; i++)
		__convertWiiString(str[i], data + (i * 0x2A), 0x2A);
	
	// Clean up
	ISFS_Close(fh);
	free(data);
	
	// Assemble name
	// Only the English name is returned
	// There are 6 other language names in the str array
	sprintf(name, "%s", str[2]);
	if (strlen(str[3]) > 1)
		sprintf(name, "%s (%s)", name, str[3]);
	
	// Job well done
	return 2;
}*/
s32 getName00(char* name, u64 id) {
    int lang = CONF_GetLanguage();
    /*
    languages
    0jap
    2eng
    4german
    6french
    8spanish
    10italian
    12dutch
    */
    //find the .app file we need
    u32 app=0;
    s32 ret = getapp(id, &app);
    if (ret<1)return ret;
    // Create a string containing the absolute filename
    char file[256] __attribute__ ((aligned (32)));
    sprintf(file, "/title/%08x/%08x/content/%08x.app", (u32)(id >> 32), (u32)id, app);

    s32 fh = ISFS_Open(file, ISFS_OPEN_READ);

    // If the title does not have 00000000.app bail out
    if (fh == -106)
	return fh;

    // In case there is some problem with the permission
    if (fh == -102) {
	// Identify as super user
//	Identify_SU();
	fh = ISFS_Open(file, ISFS_OPEN_READ);
    }

    if (fh < 0)
	return fh;

    // Jump to start of the name entries
    ISFS_Seek(fh, 0x9C, 0);

    // Read a chunk of 0x22 * 0x2B bytes from 00000000.app
    u8 *data = memalign(32, 2048);
    s32 r = ISFS_Read(fh, data, 0x22 * 0x2B);
    //printf("%s %d\n", file, r);wait_anyKey();
    if (r < 0) {
	ISFS_Close(fh);
	free(data);
	return -4;
    }

    // Take the entries apart
    char str[0x22][0x2B];
    u8 i = 0;
    // Convert the entries to ASCII strings
    for (; i < 0x22; i++)
	__convertWiiString(str[i], data + (i * 0x2A), 0x2A);

    // Clean up
    ISFS_Close(fh);
    free(data);

    // Assemble name

    //first try the language we were trying to get
    if (strlen(str[lang]) > 1) {
	sprintf(name, "%s", str[lang]);

	//if there is a part of the name in () add it
	if (strlen(str[lang+1]) > 1)
	    sprintf(name, "%s (%s)", name, str[lang+1]);
    } else {
	//default to english
	sprintf(name, "%s", str[2]);
	//if there is a part of the name in () add it
	if (strlen(str[3]) > 1)
	    sprintf(name, "%s (%s)", name, str[3]);
    }
    // Job well done
    return 2;
}

s32 printContent(u64 tid){
	char dir[256] __attribute__ ((aligned (32)));
	sprintf(dir, "/title/%08x/%08x/content", (u32)(tid >> 32), (u32)tid);

	u32 num = 64;
	
	static char list[8000] __attribute__((aligned(32)));

	ISFS_ReadDir(dir, list, &num);
	
	char *ptr = list;
	u8 br = 0;
	for (; strlen(ptr) > 0; ptr += strlen(ptr) + 1){
		printf("     %-12.12s", ptr);
		 br++; if (br == 4) { br = 0; printf("\n"); }
	}
	if (br != 0)
		printf("\n");
	
	return num;
}


signed_blob * getTMD(u64 tid) {
	static char filepath[256] ATTRIBUTE_ALIGN(32);	
	signed_blob *s_tmd = NULL;
	u32 tmd_size;
	s32 ret;
	ret = ES_GetDataDir(tid, filepath);
	if (ret < 0) {
		printf("ES_GetDataDir(%llx, %p) returned %d\n", tid, filepath, ret);
		return NULL;
	}
	
	ret = ES_GetStoredTMDSize(tid, &tmd_size);
	if (ret < 0) {
		printf("ES_GetStoredTMDSize(%llx, %p) returned %d\n", tid, &tmd_size, ret);
		return NULL;
		
	}
	
	s_tmd = memalign(32, tmd_size);
	if (s_tmd == NULL) {
		printf("memalign(32, %d) failed\n", tmd_size);
		return NULL;
	}
	
	ret = ES_GetStoredTMD(tid, s_tmd, tmd_size);
	if (ret < 0) {
		printf("ES_GetStoredTMD(%llx, %p, %d) returned %d\n", tid, s_tmd, tmd_size, ret);
		return NULL;
	}
	return s_tmd;
}



char *getNamingMode(void){
	return (char *)__modeStrings[__selected_mode];
}


void changeNamingMode(void){
	// Switch through the 4 modes
	__selected_mode++;
	if (__selected_mode >= 4)
		__selected_mode = 0;
}


u8 getDispMode(void){
	return __selected_disp;
}


char *getDispModeName(void){
	return (char *)__dispStrings[__selected_disp];
}


void changeDispMode(void){
	// Switch through the 3 modes
	__selected_disp++;
	if (__selected_disp >= 3)
		__selected_disp = 0;
}


char *titleText(u32 kind, u32 title){
	static char text[10];
	
	if (kind == 1){
		// If we're dealing with System Titles, use custom names
		switch (title){
			case 1:
				strcpy(text, "BOOT2");
			break;
			case 2:
				strcpy(text, "SYSMENU");
			break;
			case 0x100:
				strcpy(text, "BC");
			break;
			case 0x101:
				strcpy(text, "MIOS");
			break;
			default:
				sprintf(text, "IOS%u", title);
			break;
		}
	} else {
		// Otherwise, just convert the title to ASCII
		int i =32, j = 0;
		do {
			u8 temp;
			i -= 8;
			temp = (title >> i) & 0x000000FF;
			if (temp < 32 || temp > 126)
				text[j] = '.';
			else
				text[j] = temp;
			j++;
		} while (i > 0);
		text[4] = 0;
	}
	return text;
}


s32 __convertWiiString(char *str, u8 *data, u32 cnt){
	u32 i = 0;
	for(; i < cnt; data += 2){
		u16 *chr = (u16*)data;
		if (*chr == 0)
			break;
		// ignores all but ASCII characters
		else if (*chr >= 0x20 && *chr <= 0x7E)
			str[i] = *chr;
		else
			str[i] = '.';
		i++;
	}
	str[i] = 0;
		
	return 0;
}


void printh(void* p, u32 cnt){
	u32 i = 0;
	for(; i < cnt; i++){
		if (i % 16 == 0)
			printf("\n %04X: ", (u8)i);
		u8 *b = p + i;
		printf("%02X ", *b);
	}
	printf("\n");
}


void printa(void* p, u32 cnt){
	u32 i = 0;
	for(; i < cnt; i++){
		if (i % 32 == 0)
			printf("\n %04X: ", (u8)i);
		u8 *c = p + i;
		if (*c > 0x20 && *c < 0xF0)
			printf("%c", *c);
		else
			printf(".");
	}
	printf("\n");
}

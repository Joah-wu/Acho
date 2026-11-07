#pragma once
#include <stdio.h>
#include <stdint.h>
typedef unsigned int UINT;
typedef enum { FR_OK, FR_DISK_ERR, FR_NO_FILE, FR_EXIST } FRESULT;
typedef struct { FILE *file; } FIL;
typedef struct { int unused; } FILINFO;
#define FA_CREATE_ALWAYS 1
#define FA_WRITE 2
FRESULT f_stat(const char *path,FILINFO *info);
FRESULT f_unlink(const char *path);
FRESULT f_rename(const char *from,const char *to);
FRESULT f_mkdir(const char *path);
FRESULT f_open(FIL *file,const char *path,int mode);
FRESULT f_write(FIL *file,const void *data,UINT size,UINT *written);
FRESULT f_close(FIL *file);

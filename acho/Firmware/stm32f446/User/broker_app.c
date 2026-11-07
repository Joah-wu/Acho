#include "broker_app.h"
#include "transfer_link.h"
#include "lv_conf.h"
#include <stdlib.h>
#include <ctype.h>


static void submit(const char *json, const char *path,
                   UartHandle_FinishCallBackFuncTypedef callback, void *data) {
    if(!Transfer_Submit(json,path,callback,data) && callback) callback(HAL_ERROR,data);
}
void downLoadFileList(File_TypeTypeDef type, UartHandle_FinishCallBackFuncTypedef callback, void *data) {
    if(type!=IMG && type!=AUDIO) { if(callback) callback(HAL_ERROR,data); return; }
    char json[80];
    snprintf(json,sizeof(json),"{\"op\":\"get_file_list\",\"file_type\":%d}",type);
    submit(json,type==IMG?"img_list.txt":"audio_list.txt",callback,data);
}
void downLoadFile(File_TypeTypeDef type, char *id, char *name,
                  UartHandle_FinishCallBackFuncTypedef callback, void *data) {
    char path[180], json[256], stem[140];
    if(!name || !*name || strlen(name)>=sizeof(stem) || strstr(name,"..")) goto invalid;
    for(const char *p=name;*p;p++)
        if((unsigned char)*p<32 || strchr("/\\:*?\"<>|",*p)) goto invalid;
    strcpy(stem,name);
    if(type==IMG || type==QR) {
        char *dot=strrchr(stem,'.');
        if(dot) *dot=0;
        if(!*stem) goto invalid;
    }
    if(type==QR) {
        snprintf(path,sizeof(path),"Setting/%s.bin",stem);
        snprintf(json,sizeof(json),"{\"op\":\"get_bind_QR\",\"size\":[%d,%d]}",MY_DISP_VER_RES,MY_DISP_HOR_RES);
    } else if(type==IMG || type==AUDIO) {
        if(!id || !*id) goto invalid;
        for(const char *p=id;*p;p++) if(!isdigit((unsigned char)*p)) goto invalid;
        char *end;
        unsigned long number=strtoul(id,&end,10);
        if(*end || number==0 || number>0x7fffffffUL) goto invalid;
        if(type==IMG) {
            snprintf(path,sizeof(path),"Images/%s.bin",stem);
            snprintf(json,sizeof(json),"{\"op\":\"get_dither_bin\",\"file_id\":%lu,\"size\":[%d,%d]}",
                     number,MY_DISP_VER_RES,MY_DISP_HOR_RES);
        } else {
            snprintf(path,sizeof(path),"Audios/%s",stem);
            snprintf(json,sizeof(json),"{\"op\":\"get_audio_file\",\"file_id\":%lu}",number);
        }
    } else goto invalid;
    submit(json,path,callback,data);
    return;
invalid:
    if(callback) callback(HAL_ERROR,data);
}

FILE_Info_StateTypedef loadNextFileData(FIL *file, File_InfoTypedef *info) {
    if(!file || !info || f_error(file)!=FR_OK) return FILE_INFO_ERROR;
    memset(info,0,sizeof(*info));
    char *fields[]={info->id,info->name,info->description};
    const size_t limits[]={sizeof(info->id),sizeof(info->name),sizeof(info->description)};
    size_t index=0;
    unsigned field=0;
    for(;;) {
        char byte;
        UINT count=0;
        if(f_read(file,&byte,1,&count)!=FR_OK) return FILE_INFO_ERROR;
        if(count==0) return field==0 && index==0 ? FILE_INFO_END : FILE_INFO_ERROR;
        if(byte=='\r') continue;
        if(byte=='\t' && field<2) { field++; index=0; continue; }
        if(byte=='\n') return field==2 ? FILE_INFO_OK : FILE_INFO_ERROR;
        if(byte=='\t' || index+1>=limits[field]) return FILE_INFO_ERROR;
        fields[field][index++]=byte;
    }
}

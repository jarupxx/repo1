#include "stdafx.h"
#include "FileFLAC.h"

#include "flac/all.h"
#include "share/windows_unicode_filenames.h"
#include "assert.h"
#include <sys/types.h>
#include <sys/stat.h>

/*
enum{
    FLA_TITLE,      //トラック名
    FLA_ARTIST,     //アルバム名
    FLA_ALBUMARTIST,//Albm.アーティスト
    FLA_ALBUM,      //アルバム名
    FLA_YEAR,       //寝号
    FLA_TRACKNUMBER,//TrackNo
    FLA_TRACKTOTAL, //Track数
    FLA_DISCNUMBER, //DiscNo
    FLA_DISCTOTAL,  //Disc数
    FLA_GENRE,      //ジャンル
    FLA_COMMENT,    //コメント
    FLA_COPYRIGHT,  //著作権
    FLA_LYRICIST,   //作詞者
    FLA_COMPOSER,   //作曲者
    FLA_PERFORMER,  //演奏者
    FLA_ENCODEDBY,  //ソフトウェア
    FLA_LAST
};
*/
static const char *g_field_names[] =
{//FLA_XXX との対応関係(順序)に注意
    "title",      //FLA_TITLE
    "artist",     //FLA_ARTIST
    "albumartist",//FLA_ALBUMARTIST
    "album",      //FLA_ALBUM
    "date",       //FLA_YEAR
    "tracknumber",//FLA_TRACKNUMBER
    "tracktotal", //FLA_TRACKTOTAL
    "discnumber", //FLA_DISCNUMBER
    "disctotal",  //FLA_DISCTOTAL
    "genre",      //FLA_GENRE
    "comment",    //FLA_COMMENT
    "copyright",  //FLA_COPYRIGHT
    "lyricist",   //FLA_LYRICIST
    "composer",   //FLA_COMPOSER
    "performer",  //FLA_PERFORMER
    "encoded by", //FLA_ENCODEDBY
    NULL          //FLA_LAST
};
static FLAC__byte reservoir_[FLAC__MAX_BLOCK_SIZE * 2 * 2 * 2]; /* *2 for max bytes-per-sample, *2 for max channels, another *2 for overflow */
static unsigned reservoir_samples_ = 0;

static FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data);
static void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data);
static void error_callback_   (const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data);

FLAC__StreamDecoderWriteStatus write_callback_(const FLAC__StreamDecoder *decoder, const FLAC__Frame *frame, const FLAC__int32 * const buffer[], void *client_data)
{
    file_info_struct *file_info = (file_info_struct *)client_data;
    const unsigned bps = file_info->bits_per_sample, channels = file_info->channels, wide_samples = frame->header.blocksize;
    unsigned wide_sample, sample, channel;
    FLAC__int8 *scbuffer = (FLAC__int8*)reservoir_;
    FLAC__int16 *ssbuffer = (FLAC__int16*)reservoir_;

    (void)decoder;

    if(file_info->abort_flag)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    if(bps == 8) {
        for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
            for(channel = 0; channel < channels; channel++, sample++)
                scbuffer[sample] = (FLAC__int8)buffer[channel][wide_sample];
    }
    else if(bps == 16) {
        for(sample = reservoir_samples_*channels, wide_sample = 0; wide_sample < wide_samples; wide_sample++)
            for(channel = 0; channel < channels; channel++, sample++)
                ssbuffer[sample] = (FLAC__int16)buffer[channel][wide_sample];
    }
    else {
        file_info->abort_flag = true;
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }

    reservoir_samples_ += wide_samples;

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void metadata_callback_(const FLAC__StreamDecoder *decoder, const FLAC__StreamMetadata *metadata, void *client_data)
{
    file_info_struct *file_info = (file_info_struct *)client_data;
    (void)decoder;
    if(metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        //FLAC__ASSERT(metadata->data.stream_info.total_samples < 0x100000000); /* this plugin can only handle < 4 gigasamples */
        file_info->total_samples = (unsigned)(metadata->data.stream_info.total_samples&0xffffffff);
        file_info->bits_per_sample = metadata->data.stream_info.bits_per_sample;
        file_info->channels = metadata->data.stream_info.channels;
        file_info->sample_rate = metadata->data.stream_info.sample_rate;

        if(file_info->bits_per_sample == 8) {
            file_info->sample_format = FMT_S8;
        }
        else if(file_info->bits_per_sample == 16) {
            file_info->sample_format = FMT_S16_NE;
        }
        else if(file_info->bits_per_sample == 24) {
            file_info->sample_format = FMT_S24_NE;
        }
        else {
            file_info->abort_flag = true;
            return;
        }
        file_info->length_in_msec = (UINT64)file_info->total_samples * 10 / (file_info->sample_rate / 100);
    }
}

void error_callback_(const FLAC__StreamDecoder *decoder, FLAC__StreamDecoderErrorStatus status, void *client_data)
{
    file_info_struct *file_info = (file_info_struct *)client_data;
    (void)decoder;
    if(status != FLAC__STREAM_DECODER_ERROR_STATUS_LOST_SYNC)
        file_info->abort_flag = true;
}

char * strndup(char* string, int size) {
    char* buff = (char*)malloc(static_cast<size_t>(size) + 1);
    memset(buff, 0, static_cast<size_t>(size) + 1);
    memcpy(buff, string, size);
    //buff[size] = '0';
    return buff;
}

TCHAR* convert_from_utf8(const char* utf8_str)
{//utf8 => tchar*
    int utf16_len = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, 0, 0);
    WCHAR* utf16_str = (WCHAR*)malloc((static_cast<unsigned long long>(utf16_len) + 1) * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, utf16_str, utf16_len);
    utf16_str[utf16_len] = 0;
#ifdef _UNICODE
    return utf16_str;
#else
    int ansi_len = WideCharToMultiByte(CP_ACP, 0, utf16_str, -1,0,0,NULL,NULL);
    char *ansi_str = (char*)malloc(ansi_len + 1);
    WideCharToMultiByte(CP_ACP, 0, utf16_str, -1, ansi_str, ansi_len, NULL, NULL);
    free(utf16_str);
    return ansi_str;
#endif
}

char* convert_to_utf8(const TCHAR* t_str)
{//tchar* =>utf8
#ifndef _UNICODE
    int utf16_len = MultiByteToWideChar(CP_ACP, 0, t_str, -1, 0, 0);
    WCHAR *utf16_str = (WCHAR*)malloc(utf16_len*sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP, 0, t_str, -1, utf16_str, utf16_len);
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, 0, 0, NULL, NULL);
    char *utf8_str = (char*)malloc(utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, utf8_str, utf8_len, NULL, NULL);
    free(utf16_str);
    return utf8_str;
#else
    const WCHAR *utf16_str = t_str;
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, 0, 0, NULL, NULL);
    char *utf8_str = (char*)malloc(utf8_len);
    memset(utf8_str, 0xFF, utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, utf16_str, -1, utf8_str, utf8_len, NULL, NULL);
    return utf8_str;
#endif
}


/***************
 * Declaration *
 ***************/

#define MULTIFIELD_SEPARATOR _T(";;")

/* FLAC uses Ogg Vorbis comments
 * Ogg Vorbis fields names :
 *  - TITLE        : Track name
 *  - VERSION      : The version field may be used to differentiate multiple version of the same track title in a single collection. (e.g. remix info)
 *  - ALBUM        : The collection name to which this track belongs
 *  - TRACKNUMBER  : The track number of this piece if part of a specific larger collection or album
 *  - ARTIST       : Track performer
 *  - ORGANIZATION : Name of the organization producing the track (i.e. the 'record label')
 *  - DESCRIPTION  : A short text description of the contents
 *  - GENRE        : A short text indication of music genre
 *  - DATE         : Date the track was recorded
 *  - LOCATION     : Location where track was recorded
 *  - COPYRIGHT    : Copyright information
 *  - ISRC         : ISRC number for the track; see the ISRC intro page for more information on ISRC numbers.
 *
 * Field names should not be 'internationalized'; this is a concession to simplicity
 * not an attempt to exclude the majority of the world that doesn't speak English.
 * Field *contents*, however, are represented in UTF-8 to allow easy representation
 * of any language.
 */


/*************
 * Functions *
 *************/

/*
 * Read tag data from a FLAC file.
 * Note:
 *  - if field is found but contains no info (strlen(str)==0), we don't read it
 */
boolean Flac_Tag_Read_File_Tag (const TCHAR *filename, File_Tag *FileTag)
{
    FLAC__Metadata_SimpleIterator            *iter;
    FLAC__StreamMetadata                     *vc_block;
    FLAC__StreamMetadata_VorbisComment       *vc;
    FLAC__StreamMetadata_VorbisComment_Entry *field;
    TCHAR                                    *string = NULL;
    char                                     *string1 = NULL;
    unsigned int                             i;
#if 0	// FLAC Ver 1.4.2から削除されている
    flac_internal_set_utf8_filenames(true);
#endif
    flac_error_msg = NULL;
    char *filename_utf8 = convert_to_utf8(filename);
    iter = FLAC__metadata_simple_iterator_new();
    if ( iter == NULL || !FLAC__metadata_simple_iterator_init(iter, filename_utf8, true, false) )
    {
        if ( iter == NULL )
        {
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
        }else
        {
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iter)];
            FLAC__metadata_simple_iterator_delete(iter);
        }
        ////g_print(_("ERROR while opening file: '%s' as FLAC (%s).\n\a"),filename,flac_error_msg);
        free(filename_utf8);
        return FALSE;
    }
    free(filename_utf8);

    /* libFLAC is able to detect (and skip) ID3v2 tags by itself */

    /* Find the VORBIS_COMMENT block */
    while ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        if ( !FLAC__metadata_simple_iterator_next(iter) )
        {
            /* End of metadata: comment block not found, nothing to read */
            FLAC__metadata_simple_iterator_delete(iter);
            return TRUE;
        }
    }

    /* Get comments from block */
    vc_block = FLAC__metadata_simple_iterator_get_block(iter);
    vc = &vc_block->data.vorbis_comment;
    //by Kobarin
    //コピーアンドペーストが多いのでテーブル化
    //Flac_Tag_Write_File_Tag の方でも同じように処理
/*    struct _table{
        const char *fieldname;
        TCHAR **ppValue;
    }table[] = {
        {"title",  &FileTag->title},
        {"artist", &FileTag->artist},
        {"album",  &FileTag->album},
        {"date",   &FileTag->year},
        {"tracknumber",&FileTag->track},      //オリジナルと取得法が異なる
        {"tracktotal", &FileTag->track_total},//オリジナルと取得法が異なる
        {"genre",    &FileTag->genre},
        {"comment",  &FileTag->comment},
        {"composer", &FileTag->composer},
        {"performer",&FileTag->performer},
        {NULL, NULL}
    };
*/
    for(i = 0; i < vc->num_comments; i++){
        field = &vc->comments[i];
        int j;
        for(j = 0; g_field_names[j]; j++){
            int field_len = strlen(g_field_names[j]);
            if(field->length < field_len){//長さが異なる
                continue;
            }
            if(_strnicmp((char*)field->entry, g_field_names[j], field_len) == 0 &&
               field->entry[field_len] == '='){
            //対応しているフィールド名
                break;
            }
        }
        if(!g_field_names[j]){//テーブル内で見つからなければ未対応
            //g_print("custom %*s\n", field->length, field->entry);
            FileTag->other->Add(strndup((char*)field->entry, field->length));
        }
        else{//対応していれば FileTag->values[] に登録
            const char *value_utf8 = strchr((const char*)field->entry, '=');
            if(!value_utf8 || !value_utf8[1]){//中身がない
                continue;
            }
            value_utf8++;
            TCHAR *value_t = convert_from_utf8(value_utf8);
            if(!FileTag->values[j]){
                FileTag->values[j] = value_t;
            }
            else{//2個目以降は MULTIFIELD_SEPARATOR で連結
                int new_value_len = _tcslen(FileTag->values[j]) + 1 +
                                    _tcslen(MULTIFIELD_SEPARATOR) + 1 +
                                    _tcslen(value_t) + 1;
                TCHAR *new_value = (TCHAR*)realloc(FileTag->values[j], new_value_len * sizeof(TCHAR));
                _tcscat_s(new_value, new_value_len, MULTIFIELD_SEPARATOR);
                _tcscat_s(new_value, new_value_len, value_t);
                free(value_t);
                FileTag->values[j] = new_value;
            }
        }
    }
    /*************************
     * Track and Total Track *
     *************************/
    if(FileTag->values[FLA_TRACKNUMBER]){// "number/total" のように区切られているか確認
        TCHAR *separator = _tcschr(FileTag->values[FLA_TRACKNUMBER], _T('/'));
        if(separator){
            *separator++ = 0;// '/' 以前をトラック番号として扱う
            if(!FileTag->values[FLA_TRACKTOTAL]){// '/' 以降をトラック数として扱う
                FileTag->values[FLA_TRACKTOTAL] = _tcsdup(separator);
            }
        }
    }
    FLAC__metadata_object_delete(vc_block);
    FLAC__metadata_simple_iterator_delete(iter);

    return TRUE;
}

unsigned long Get_File_Size (const TCHAR *filename)
{
    struct _stat statbuf;
    FLAC__Metadata_SimpleIterator               *iter;
    FLAC__StreamMetadata                        *vc_block;

    if (filename)
    {
        _tstat(filename,&statbuf);
        char *filename_utf8 = convert_to_utf8(filename);
        iter = FLAC__metadata_simple_iterator_new();
        if ( iter == NULL || !FLAC__metadata_simple_iterator_init(iter, filename_utf8, true, false) ){
            free(filename_utf8);
            return 0;
        }
        unsigned int tagSize = 0;
        while ((vc_block = FLAC__metadata_simple_iterator_get_block(iter)) != NULL)
        {
            tagSize += vc_block->length;
            FLAC__metadata_object_delete(vc_block);
            if ( !FLAC__metadata_simple_iterator_next(iter) )
            {
                /* End of metadata */
                break;;
            }
        }
        FLAC__metadata_simple_iterator_delete(iter);
        free(filename_utf8);
        return statbuf.st_size - tagSize - 20;
    }else
    {
        return 0;
    }
}

boolean Flac_Header_Read_File_Info (const TCHAR *filename, File_Tag *FileTag)
{
    FILE *file = NULL;
    double duration = 0;
    unsigned long filesize;

    FLAC__StreamDecoder *tmp_decoder;
    file_info_struct tmp_file_info = {0};

    _tfopen_s(&file, filename, _T("rb"));
    if (file==NULL )
    {
        //g_print(_("ERROR while opening file: '%s' (%s)\n\a"),filename,g_strerror(errno));
        return FALSE;
    }
    fclose(file);

    /* Decoding FLAC file */
    tmp_decoder = FLAC__stream_decoder_new();
    if(tmp_decoder == 0){
        return FALSE;
    }
    tmp_file_info.abort_flag = false;
    FLAC__stream_decoder_set_md5_checking     (tmp_decoder, false);

    char *filename_utf8 = convert_to_utf8(filename);
    if(FLAC__stream_decoder_init_file(tmp_decoder,
                                      filename_utf8,
                                      write_callback_,
                                      metadata_callback_,
                                      error_callback_, &tmp_file_info) != FLAC__STREAM_DECODER_INIT_STATUS_OK){
        FLAC__stream_decoder_finish(tmp_decoder);
        FLAC__stream_decoder_delete(tmp_decoder);
        free(filename_utf8);
        return FALSE;
    }
    free(filename_utf8);

    if(!FLAC__stream_decoder_process_until_end_of_metadata(tmp_decoder)){ // FLAC 1.0.4 (Bastian Kleineidam)
        FLAC__stream_decoder_finish(tmp_decoder);
        FLAC__stream_decoder_delete(tmp_decoder);
        fclose(file);
        return FALSE;
    }
    FLAC__stream_decoder_finish(tmp_decoder);
    FLAC__stream_decoder_delete(tmp_decoder);
    fclose(file);
    /* End of decoding FLAC file */


    filesize = Get_File_Size(filename);
    struct _stat64 stat_;
    _tstat64(filename, &stat_);
    //duration = (int)tmp_file_info.length_in_msec/1000;

    //if (tmp_file_info.length_in_msec/1000.0 > 0) {
        //FileTag->bitrate = filesize*8.0/tmp_file_info.length_in_msec;
    if (tmp_file_info.length_in_msec > 0) {
        FileTag->bitrate =
                    8.0 * (float)(filesize) / (1000.0 * (float)tmp_file_info.total_samples / (float)tmp_file_info.sample_rate);
                    // bitrateがあわないときは、_stat.st_size で計算していると思われる
                    // _statでは単にファイルのサイズであり、filesize はタグ分を抜いたサイズ
    }
    FileTag->samplerate  = tmp_file_info.sample_rate;
    FileTag->bps = tmp_file_info.bits_per_sample;
    FileTag->channels        = tmp_file_info.channels;
    FileTag->size        = filesize;
    FileTag->duration    = tmp_file_info.length_in_msec/1000;

    return TRUE;
}

boolean Flac_Tag_Write_File_Tag (const TCHAR *filename, File_Tag *FileTag)
{
    FLAC__Metadata_SimpleIterator               *iter;
    FLAC__StreamMetadata                        *vc_block;
    FLAC__StreamMetadata_VorbisComment_Entry    field;
    FLAC__bool                                  write_ok;
    char                                       *string;

    flac_error_msg = NULL;

    /* libFLAC is able to detect (and skip) ID3v2 tags by itself */
    char *filename_utf8 = convert_to_utf8(filename);
    iter = FLAC__metadata_simple_iterator_new();
    if ( iter == NULL || !FLAC__metadata_simple_iterator_init(iter,filename_utf8,false,false) )
    {
        if ( iter == NULL )
        {
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__METADATA_SIMPLE_ITERATOR_STATUS_MEMORY_ALLOCATION_ERROR];
        }else
        {
            flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iter)];
            FLAC__metadata_simple_iterator_delete(iter);
        }

        //g_print(_("ERROR while opening file: '%s' as FLAC (%s).\n\a"),filename_in,flac_error_msg);
        free(filename_utf8);
        return FALSE;
    }
    free(filename_utf8);
    filename_utf8 = NULL;//念のため

    /* Find the VORBIS_COMMENT block */
    while ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        if ( !FLAC__metadata_simple_iterator_next(iter) )
            break;
    }

    /* Write tag */
    if ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
        vc_block = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
    else
        vc_block = FLAC__metadata_simple_iterator_get_block(iter);

    int i,j;
    for(i = 0; i < FLA_LAST; i++){
        //FIXME: 同一タグの数を調べて必要な回数削除する
        for(j = 0; j < 8; j++){
            FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, g_field_names[i]);
        }
        TCHAR *value = FileTag->values[i];
        if(!value || !*value){
            continue;
        }
        char *value_utf8 = convert_to_utf8(value);
        char *token = strtok(value_utf8, /*MULTIFIELD_SEPARATOR*/";;");
        while (token != NULL) {
            int field_size = strlen(g_field_names[i]) +
                             strlen(token) + 1 + 1;//"=" の分だけ1バイト余計に確保
            string = (char*)malloc(field_size);
            strcpy_s(string, field_size, g_field_names[i]);
            strcat_s(string, field_size, "=");
            strcat_s(string, field_size, token);
            field.entry = (unsigned char*)string;
            field.length = strlen(string);
            FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
            free(string);

            token = strtok(NULL, /*MULTIFIELD_SEPARATOR*/";;");
        }
        free(value_utf8);
        //free(string1);

    }
#if 0
    /*********
     * Title *
     *********/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "title");
    if ( FileTag->title && strlen(FileTag->title) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "title=");
        strcat(string, FileTag->title);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /**********
     * Artist *
     **********/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "artist");
    if ( FileTag->artist && strlen(FileTag->artist) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "artist=");
        strcat(string, FileTag->artist);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /*********
     * Album *
     *********/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "album");
    if ( FileTag->album && strlen(FileTag->album) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "album=");
        strcat(string, FileTag->album);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /********
     * Year *
     ********/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "date");
    if ( FileTag->year && strlen(FileTag->year) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "date=");
        strcat(string, FileTag->year);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /*************************
     * Track and Total Track *
     *************************/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "tracknumber");
    if ( FileTag->str_track /*FileTag->track*/ && strlen(FileTag->str_track) > 0)
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        /*
        if ( FileTag->track_total && strlen(FileTag->track_total)>0 ) {
            strcpy(string, "tracknumber=");
            strcat(string, FileTag->track);
            strcat(string, "/");
            strcat(string, FileTag->track_total);
        } else {
            strcpy(string, "tracknumber=");
            strcat(string, FileTag->track);
        }
        */
        strcpy(string, "tracknumber=");
        strcat(string, FileTag->str_track);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /*********
     * Genre *
     *********/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "genre");
    if ( FileTag->genre && strlen(FileTag->genre) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "genre=");
        strcat(string, FileTag->genre);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /***********
     * Comment *
     ***********/
    // We write the comment using the "both" format
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "comment");
    if ( FileTag->comment && strlen(FileTag->comment) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "comment=");
        strcat(string, FileTag->comment);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /***********
     * Composer*
     ***********/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "composer");
    if ( FileTag->composer && strlen(FileTag->composer) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "composer=");
        strcat(string, FileTag->composer);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }

    /***********
     * Performer*
     ***********/
    FLAC__metadata_object_vorbiscomment_remove_entry_matching(vc_block, "performer");
    if ( FileTag->performer && strlen(FileTag->performer) > 0 )
    {
        string = (char*)malloc(1024+1);//すべてのタグで入力できる最大+1
        strcpy(string, "performer=");
        strcat(string, FileTag->performer);
        string1 = convert_to_utf8(string);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
        free(string);
        free(string1);
    }
#endif
    /**************************
     * Set unsupported fields *
     **************************/
    /*
    for (int i=0;i<FileTag->other->GetSize();i++)
    {
        string1 = (char*)FileTag->other->GetAt(i);
        field.entry = (unsigned char*)string1;
        field.length = strlen(string1);
        FLAC__metadata_object_vorbiscomment_insert_comment(vc_block,vc_block->data.vorbis_comment.num_comments,field,true);
    }
    */

    /* Find the VORBIS_COMMENT block */
    /*
    while ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        if ( !FLAC__metadata_simple_iterator_next(iter) )
            break;
    }
    */

    /* Write tag */
    if ( FLAC__metadata_simple_iterator_get_block_type(iter) != FLAC__METADATA_TYPE_VORBIS_COMMENT )
    {
        /* End of metadata: no comment block, so insert one */
        write_ok = FLAC__metadata_simple_iterator_insert_block_after(iter,vc_block,true);
    }
    else
    {
        write_ok = FLAC__metadata_simple_iterator_set_block(iter,vc_block,true);
    }

    if ( !write_ok )
    {
        flac_error_msg = FLAC__Metadata_SimpleIteratorStatusString[FLAC__metadata_simple_iterator_status(iter)];
        //g_print(_("ERROR: Failed to write comments to file '%s' (%s).\n"),filename_in,flac_error_msg);
        FLAC__metadata_simple_iterator_delete(iter);
        FLAC__metadata_object_delete(vc_block);
        return FALSE;
    }else
    {
        //g_print(_("Written tag of '%s'\n"),g_basename(filename_in));
    }

    FLAC__metadata_simple_iterator_delete(iter);
    FLAC__metadata_object_delete(vc_block);

    // FIX ME! : Remove the ID3 tag if found....

    return TRUE;
}

bool LoadFileFLAC(FILE_INFO *pFile)
{
    File_Tag FileTag;
    memset(&FileTag, 0, sizeof(FileTag));
    FileTag.other = new CPtrArray();
    if(!Flac_Tag_Read_File_Tag(GetFullPath(pFile), &FileTag)){
        //return false;
    }
    Flac_Header_Read_File_Info(GetFullPath(pFile), &FileTag);

    SetTrackNameSI(pFile, FileTag.values[FLA_TITLE]);
    SetArtistNameSI(pFile, FileTag.values[FLA_ARTIST]);
    SetAlbumArtistSI(pFile, FileTag.values[FLA_ALBUMARTIST]);
    SetAlbumNameSI(pFile, FileTag.values[FLA_ALBUM]);
    SetYearSI(pFile, FileTag.values[FLA_YEAR]);
    SetTrackNumberSI(pFile, FileTag.values[FLA_TRACKNUMBER]);
    SetTrackTotalSI(pFile, FileTag.values[FLA_TRACKTOTAL]);
    SetDiscNumberSI(pFile, FileTag.values[FLA_DISCNUMBER]);
    SetDiscTotalSI(pFile, FileTag.values[FLA_DISCTOTAL]);
    SetGenreSI(pFile, FileTag.values[FLA_GENRE]);
    SetCommentSI(pFile, FileTag.values[FLA_COMMENT]);
    SetCopyrightSI(pFile, FileTag.values[FLA_COPYRIGHT]);
    SetWriterSI(pFile, FileTag.values[FLA_LYRICIST]);
    SetComposerSI(pFile, FileTag.values[FLA_COMPOSER]);
    SetOrigArtistSI(pFile, FileTag.values[FLA_PERFORMER]);
    SetSoftwareSI(pFile, FileTag.values[FLA_ENCODEDBY]);
    SetPlayTime(pFile, FileTag.duration);

    TCHAR format[256];
    _sntprintf_s(format, _TRUNCATE, _T("FLAC %dkbs, %dHz, %dbit, %dch"),
                 FileTag.bitrate, FileTag.samplerate, FileTag.bps, FileTag.channels);
    SetAudioFormat(pFile, format);

    int i;
    for(i = 0; i < FLA_LAST; i++){
        if(FileTag.values[i]){
            free(FileTag.values[i]);
        }
    }
    for (int i=0;i<FileTag.other->GetSize();i++) {
        free(FileTag.other->GetAt(i));
    }
    delete FileTag.other;
    return TRUE;
}

bool WriteFileFLAC(FILE_INFO *pFile)
{
    File_Tag FileTag;
    memset(&FileTag, 0, sizeof(FileTag));
    FileTag.other = new CPtrArray();
    //Flac_Tag_Read_File_Tag(GetFullPath(pFile), &FileTag); // FitaTag.other への設定

    FileTag.values[FLA_TITLE] =    (TCHAR*)GetTrackNameSI(pFile);
    FileTag.values[FLA_ARTIST] =   (TCHAR*)GetArtistNameSI(pFile);
    FileTag.values[FLA_ALBUMARTIST] = (TCHAR*)GetAlbumArtistSI(pFile);
    FileTag.values[FLA_ALBUM] =    (TCHAR*)GetAlbumNameSI(pFile);
    FileTag.values[FLA_YEAR] =     (TCHAR*)GetYearSI(pFile);
    FileTag.values[FLA_TRACKNUMBER] = (TCHAR*)GetTrackNumberSI(pFile);
    FileTag.values[FLA_TRACKTOTAL] = (TCHAR*)GetTrackTotalSI(pFile);
    FileTag.values[FLA_DISCNUMBER] = (TCHAR*)GetDiscNumberSI(pFile);
    FileTag.values[FLA_DISCTOTAL] = (TCHAR*)GetDiscTotalSI(pFile);
    FileTag.values[FLA_GENRE] =     (TCHAR*)GetGenreSI(pFile);
    FileTag.values[FLA_COMMENT] =  (TCHAR*)GetCommentSI(pFile);
    FileTag.values[FLA_COPYRIGHT] = (TCHAR*)GetCopyrightSI(pFile);
    FileTag.values[FLA_LYRICIST] = (TCHAR*)GetWriterSI(pFile);
    FileTag.values[FLA_COMPOSER] = (TCHAR*)GetComposerSI(pFile);
    FileTag.values[FLA_PERFORMER] = (TCHAR*)GetOrigArtistSI(pFile);
    FileTag.values[FLA_ENCODEDBY] = (TCHAR*)GetSoftwareSI(pFile);
    bool ret = Flac_Tag_Write_File_Tag(GetFullPath(pFile), &FileTag) ? TRUE : FALSE;
    //free(FileTag.track);
    //free(FileTag.track_total);
    for (int i=0;i<FileTag.other->GetSize();i++) {
        free(FileTag.other->GetAt(i));
    }
    delete FileTag.other;
    return ret;
}

#ifndef TEXT_READER_H
#define TEXT_READER_H

typedef enum _text_encode {
	text_encode_ansi,
	text_encode_utf8,
	text_encode_utf8_bom,
	text_encode_ucs2_big,
	text_encode_ucs2_little
}text_encode_t;


typedef struct _text_line {
    struct _text_line* next;
    wchar_t* text;
}text_line_t;

typedef struct _text_line_list {
    unsigned int count;
	text_line_t* begin;
	text_line_t* end;
}text_line_list;


#include <stdio.h>
#include <stdlib.h>


extern text_encode_t get_file_encode(FILE* file);
extern void   seek_to_begin(FILE* file, text_encode_t encode);
extern size_t fgetslen(FILE* file, text_encode_t encode, int bseek);
extern size_t fgetline(char* buff, FILE* file, text_encode_t encode);
extern unsigned int read_text_lines(FILE* file, text_line_list* list);
extern void remove_text_line_list(text_line_list* list);

extern int      UTF8IsValid(const char* su8, size_t us_len);
extern size_t   UTF16Length(const char* s, size_t slength);
extern size_t   UTF16FromUTF8(const char* sv, size_t sv_length, wchar_t* tbuf, size_t tlen);
extern wchar_t* UTF8ToUCS2(const char* utf8);

#endif //TEXT_READER_H
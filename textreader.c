
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <locale.h>
#include <sys/stat.h> 
#include "textreader.h"


text_encode_t get_file_encode(FILE* file){
    text_encode_t encode_type;
    unsigned char magic[2];

    encode_type = text_encode_ansi;
    if (file){
        fpos_t pos;
        fgetpos(file,&pos);
        fseek(file, 0, SEEK_SET);
        fread(magic, 2, 1, file);
        if (magic[0] == 0xEF && magic[1] == 0xBB) {
            //UTF-8
            unsigned char c = fgetc(file);
            if (c == 0xBF) {
                encode_type = text_encode_utf8_bom;
            }
        }
        else if (magic[0] == 0xFE && magic[1] == 0xFF) {
            //UCS2-big
            encode_type = text_encode_ucs2_big;
        }
        else if (magic[0] == 0xFF && magic[1] == 0xFE) {
            //UCS2-little
            encode_type = text_encode_ucs2_little;
        }

        if (encode_type == text_encode_ansi) {
            /*
            UTF-8 valid format list:
            0xxxxxxx
            110xxxxx 10xxxxxx
            1110xxxx 10xxxxxx 10xxxxxx
            11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
            111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
            1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx

            10      = 0x02u
            110     = 0x06u
            1110    = 0x0Eu
            11110   = 0x1Eu
            111110  = 0x3Eu
            1111110 = 0x7Eu
            */
            int bytes = 0;
            int all_ascii = 1;
            unsigned char chr = (unsigned char)fgetc(file);
            while (!feof(file)){
                if (bytes == 0 && (chr & 0x80u) != 0) {
                    all_ascii = 0;
                    break;
                }
                if (bytes == 0){
                    if (chr >> 5 == 0x06u) bytes = 1;
                    else if (chr >> 4 == 0x0Eu) bytes = 2;
                    else if (chr >> 3 == 0x1Eu) bytes = 3;
                    else if (chr >> 2 == 0x3Eu) bytes = 4;
                    else if (chr >> 1 == 0x7Eu) bytes = 5;
                }
                else
                {
                    if ((chr >> 6) != 0x02u){
                        break;
                    }
                    bytes--;
                }
                chr = fgetc(file);
            }
            if (!(bytes > 0 || all_ascii)){
                encode_type = text_encode_utf8;
            }
        }
        fsetpos(file, &pos);
    }
    return encode_type;
}

void seek_to_begin(FILE* file, text_encode_t encode) {
    if (encode == text_encode_utf8_bom) {
        fseek(file, 3, SEEK_SET);
    }
    else if (encode == text_encode_ucs2_big || encode == text_encode_ucs2_little) {
        fseek(file, 2, SEEK_SET);
    }
    else {
        fseek(file, 0, SEEK_SET);
    }
}

size_t fgetslen(FILE* file, text_encode_t encode, int bseek) {
    register int c1, c2;
    register size_t n;
    fpos_t pos;
    n = 0;
    if (!bseek) {
        fgetpos(file, &pos);
    }
    if (encode == text_encode_ucs2_big || encode == text_encode_ucs2_little) {
        while (1) {
            c1 = fgetc(file);
            if (feof(file)) {
                break;
            }
            n++;
            c2 = fgetc(file);
            if (feof(file)) {
                break;
            }
            n++;
            if (encode == text_encode_ucs2_big) {
                if (c1 == '\0' && c2 == '\n') {
                    break;
                }
            }
            else {

                if (c1 == '\n' && c2 == '\0') {
                    break;
                }
            }
        }
    }
    else {
        while (1) {
            c1 = fgetc(file);
            if (feof(file)) {
                break;
            }
            n++;
            if (c1 == '\n') {
                break;
            }
        }
    }
    if (!bseek) {
        fsetpos(file, &pos);
    }
    return n;
}

size_t fgetline(char* buff, FILE* file, text_encode_t encode) {
    register int c1, c2;
    register size_t n;
    n = 0;
    if (encode == text_encode_ucs2_big) {
        while (1) {
            c1 = fgetc(file);
            if (feof(file)) {
                break;
            }
            c2 = fgetc(file);
            if (feof(file)) {
                break;
            }
            buff[n] = c2; n++;
            buff[n] = c1; n++;
            if (c1 == '\0' && c2 == '\n') {
                break;
            }
        }
    }else if (encode == text_encode_ucs2_little){
        while (1) {
            c1 = fgetc(file);
            if (feof(file)) {
                break;
            }
            buff[n] = c1; n++;
            c2 = fgetc(file);
            if (feof(file)) {
                break;
            }
            buff[n] = c2; n++;
            if (c1 == '\n' && c2 == '\0') {
                break;
            }
        }
    }
    else {
        while (1) {
            c1 = fgetc(file);
            if (feof(file)) {
                break;
            }
            buff[n] = c1;
            n++;
            if (c1 == '\n') {
                break;
            }
        }
    }
    return n;
}

void add_line(text_line_list* list, wchar_t* str) {
    text_line_t* item;
    if (str == NULL || *str == '\0') return;
    item = (text_line_t*)malloc(sizeof(text_line_t));
    if (item) {
        item->next = NULL;
        item->text = str;

        if (list->begin == NULL)
            list->begin = item;

        if (list->end) {
            list->end->next = item;
        }

        list->end = item;

        list->count++;
    }
}

void remove_text_line_list(text_line_list* list){
    text_line_t* item;
    if (!list) return;

    item = list->begin;

    while (item) {
        text_line_t* next = item->next;
        free(item->text);
        free(item);
        item = next;
    }
}

unsigned int read_text_lines(FILE* file, text_line_list* list) {

    int encode;
    size_t len,n;
    char* buf;
    wchar_t* wbuf;

    if (!file || !list) return 0;

    list->end = list->begin = NULL;
    list->count = 0;

    encode = get_file_encode(file);

    seek_to_begin(file, encode);


    for (;;) {
        len = fgetslen(file, encode, 0);
        if (len == 0) break;

        buf = (char*)malloc(len + sizeof(wchar_t));
        if (buf) {
            n = fgetline((void*)buf, file, encode);
            memset(&buf[n],0,sizeof(wchar_t));

            if (encode == text_encode_ucs2_little || encode == text_encode_ucs2_big) {
                add_line(list, (wchar_t*)buf);
            }
            else if (encode == text_encode_utf8 || encode == text_encode_utf8_bom) {
                wchar_t *wstr=UTF8ToUCS2(buf);
                if (wstr){
                    add_line(list, wstr);
                }
                free(buf);
            }
            else {
                //The fixed buffer width is used here, and it can be changed to a lager buffer size if necessary.
                wbuf = (wchar_t*)malloc(2048 * sizeof(wchar_t));
                if (wbuf) {
                    n = mbstowcs(wbuf, buf, 2048);
                    if (n > 0) {
                        add_line(list, wbuf);
                    }
                    else {
                        free(wbuf);
                    }
                    free(buf);
                }
            }
        }
    }

    return list->count;
}


enum { UTF8MaskWidth = 0x7, UTF8MaskInvalid = 0x8 };

enum { SURROGATE_LEAD_FIRST = 0xD800 };
enum { SURROGATE_LEAD_LAST = 0xDBFF };
enum { SURROGATE_TRAIL_FIRST = 0xDC00 };
enum { SURROGATE_TRAIL_LAST = 0xDFFF };
enum { SUPPLEMENTAL_PLANE_FIRST = 0x10000 };

const unsigned char UTF8BytesOfLead[256] = {
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 00 - 0F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 10 - 1F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 20 - 2F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 30 - 3F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 40 - 4F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 50 - 5F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 60 - 6F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 70 - 7F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 80 - 8F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 90 - 9F
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // A0 - AF
1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // B0 - BF
1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // C0 - CF
2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // D0 - DF
3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // E0 - EF
4, 4, 4, 4, 4, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // F0 - FF
};

#define UTF16LengthFromUTF8ByteCount(byteCount) ((byteCount < 4) ? 1 : 2)

unsigned char TrailByteValue(unsigned char c) {
    // The top 2 bits are 0b10 to indicate a trail byte.
    // The lower 6 bits contain the value.
    return c & 0x3Fu; //0b00111111
}

int UTF8IsTrailByte(unsigned char ch){
    return (ch >= 0x80) && (ch < 0xC0);
}

int UTF8Classify(const unsigned char* us, size_t len) {
    // For the rules: http://www.cl.cam.ac.uk/~mgk25/unicode.html#utf-8
    size_t byteCount;

    if (us[0] < 0x80) {
        // ASCII
        return 1;
    }

    byteCount = UTF8BytesOfLead[us[0]];
    if (byteCount == 1 || byteCount > len) {
        // Invalid lead byte
        return UTF8MaskInvalid | 1;
    }

    if (!UTF8IsTrailByte(us[1])) {
        // Invalid trail byte
        return UTF8MaskInvalid | 1;
    }

    switch (byteCount) {
    case 2:
        return 2;

    case 3:
        if (UTF8IsTrailByte(us[2])) {
            if ((*us == 0xe0) && ((us[1] & 0xe0) == 0x80)) {
                // Overlong
                return UTF8MaskInvalid | 1;
            }
            if ((*us == 0xed) && ((us[1] & 0xe0) == 0xa0)) {
                // Surrogate
                return UTF8MaskInvalid | 1;
            }
            if ((*us == 0xef) && (us[1] == 0xbf) && (us[2] == 0xbe)) {
                // U+FFFE non-character - 3 bytes long
                return UTF8MaskInvalid | 3;
            }
            if ((*us == 0xef) && (us[1] == 0xbf) && (us[2] == 0xbf)) {
                // U+FFFF non-character - 3 bytes long
                return UTF8MaskInvalid | 3;
            }
            if ((*us == 0xef) && (us[1] == 0xb7) && (((us[2] & 0xf0) == 0x90) || ((us[2] & 0xf0) == 0xa0))) {
                // U+FDD0 .. U+FDEF
                return UTF8MaskInvalid | 3;
            }
            return 3;
        }
        break;

    default:
        if (UTF8IsTrailByte(us[2]) && UTF8IsTrailByte(us[3])) {
            if (((us[1] & 0xf) == 0xf) && (us[2] == 0xbf) && ((us[3] == 0xbe) || (us[3] == 0xbf))) {
                // *FFFE or *FFFF non-character
                return UTF8MaskInvalid | 4;
            }
            if (*us == 0xf4) {
                // Check if encoding a value beyond the last Unicode character 10FFFF
                if (us[1] > 0x8f) {
                    return UTF8MaskInvalid | 1;
                }
            }
            else if ((*us == 0xf0) && ((us[1] & 0xf0) == 0x80)) {
                // Overlong
                return UTF8MaskInvalid | 1;
            }
            return 4;
        }
        break;
    }

    return UTF8MaskInvalid | 1;
}

int UTF8IsValid(const char * su8, size_t us_len){
    const unsigned char* us = (const unsigned char*)su8;
    size_t remaining = us_len;
    while (remaining > 0) {
        const int utf8Status = UTF8Classify(us, remaining);
        if (utf8Status & UTF8MaskInvalid) {
            return 0;
        }
        else {
            const int lenChar = utf8Status & UTF8MaskWidth;
            us += lenChar;
            remaining -= lenChar;
        }
    }
    return remaining == 0;
}


size_t UTF16Length(const char* s, size_t slength) {
    size_t i, ulen;
    ulen = 0;
    for (i = 0; i < slength;) {
        const unsigned char ch = s[i];
        const unsigned int byteCount = UTF8BytesOfLead[ch];
        const unsigned int utf16Len = UTF16LengthFromUTF8ByteCount(byteCount);
        i += byteCount;
        ulen += (i > slength) ? 1 : utf16Len;
    }
    return ulen;
}

size_t UTF16FromUTF8(const char * sv, size_t sv_length, wchar_t* tbuf, size_t tlen) {
    size_t i, ui, outLen;
    ui = 0;
    for (i = 0; i < sv_length;) {
        unsigned char ch = sv[i];
        const unsigned int byteCount = UTF8BytesOfLead[ch];
        unsigned int value;

        if (i + byteCount > sv_length) {
            // Trying to read past end but still have space to write
            if (ui < tlen) {
                tbuf[ui] = ch;
                ui++;
            }
            break;
        }

        outLen = UTF16LengthFromUTF8ByteCount(byteCount);
        if (ui + outLen > tlen) {
            return -1;
        }

        /*
        UTF-8 valid format list:
        0xxxxxxx
        110xxxxx 10xxxxxx
        1110xxxx 10xxxxxx 10xxxxxx
        11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
        111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
        */

        i++;
        switch (byteCount) {
        case 1:
            tbuf[ui] = ch;
            break;
        case 2:
            value = (ch & 0x1F) << 6;
            ch = sv[i++];
            value += TrailByteValue(ch);
            tbuf[ui] = (wchar_t)(value);
            break;
        case 3:
            value = (ch & 0xF) << 12;
            ch = sv[i++];
            value += (TrailByteValue(ch) << 6);
            ch = sv[i++];
            value += TrailByteValue(ch);
            tbuf[ui] = (wchar_t)(value);
            break;
        default:
            // Outside the BMP so need two surrogates
            value = (ch & 0x7) << 18;
            ch = sv[i++];
            value += TrailByteValue(ch) << 12;
            ch = sv[i++];
            value += TrailByteValue(ch) << 6;
            ch = sv[i++];
            value += TrailByteValue(ch);
            tbuf[ui] = (wchar_t)(((value - 0x10000) >> 10) + SURROGATE_LEAD_FIRST);
            ui++;
            tbuf[ui] = (wchar_t)((value & 0x3ff) + SURROGATE_TRAIL_FIRST);
            break;
        }
        ui++;
    }
    return ui;
}

wchar_t* UTF8ToUCS2(const char* utf8)
{
    size_t len = strlen(utf8);
    size_t wcharSize = UTF16Length(utf8, len);
    wchar_t* w = malloc((wcharSize + 1)*sizeof(wchar_t));
    if (!w) return NULL;
    UTF16FromUTF8(utf8, len, w, wcharSize + 1);
    w[wcharSize] = 0;
    return w;
}


#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>
#include <locale.h>

#include "textreader.h"

int main(int argc, char **argv)
{
    if (argc<2){
        printf("usage: demo text.txt\n");
        return -1;
    }
    setlocale(LC_CTYPE, "");
    text_line_t* item;
    FILE *file = fopen(argv[1], "rb");
    if (!file){
        printf("open %s failed.\n");
        return -1;
    }

    text_line_list line_list;
    if (read_text_lines(file, &line_list)) {
        printf("linecount=%u\n", line_list.count);
        item = line_list.begin;
        while (item) {
            printf("%ls", item->text);

            //Note the GCC -fshort-wchar compilation option.
            const wchar_t *s=item->text;
            while(*s){
                printf("%C",*s);
                s++;
            }

            item = item->next;
        }
        remove_text_line_list(&line_list);
    }
    fclose(file);

    return 0;
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>

#define RESET "\x1B[0m"
#define RED "\x1B[91m"
#define BOLD "\x1B[1m"
#define UNDERLINE "\x1B[4m"
#define ERROR RED BOLD "error: " RESET
#define raise(fmt, ...) do {                                    \
    fprintf(stderr, ERROR fmt "\n" __VA_OPT__(,) __VA_ARGS__);  \
    exit(-1);                                                   \
} while (0);

const char* run(
    const char* body,
    unsigned char* data,
    size_t len,
    size_t index,
    bool is_root
);

int main(int argc, const char** argv) {
    const char* path = NULL;
    size_t len = 0;
    for (size_t i = 1; i < (size_t)argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            printf(
                "\n"
                "An interpreter for Brainfuck programs. (https://en.wikipedia.org/wiki/Brainfuck)\n"
                "\n"
                "usage:\t" BOLD "brainfuck" RESET " [-n " UNDERLINE "cells" RESET "] [" UNDERLINE "filename" RESET "]\n"
                "\n"
                "arguments:\n"
                "\t" "cells" RESET "\t\tthe number of cells in user data (defaults to 30000)\n"
                "\t" "filename" RESET "\tthe source file (reads from stdin if unspecified)\n"
                "\n"
                "This implementation uses one byte per cell, interpreted like a C one-byte unsigned integer: "
                "increments and decrements are performed modulo 256 (by 'wrapping-around'). "
                "Moving the pointer past one end of the tape makes it wrap around to the other side.\n"
                "\n"
            );
            return 0;
        }

        if (argv[i][0] != '-') {
            if (path != NULL) {
                raise("unexpected argument: '%s'", argv[i]);
            }
            path = argv[i];
            continue;
        }

        if (argv[i][1] == 'n') {
            if (len != 0) {
                raise("unexpected option: '%s'", argv[i]);
            }

            const char* len_string;
            if (argv[i][2] == '=') {
                len_string = argv[i] + 3;
            } else {
                if (argv[i][2] != '\0') {
                    raise("unexpected option: '%s'", argv[i]);
                }
                len_string = argv[i + 1];
                i++;
            }
            errno = 0;
            const char* str_end;
            // FIXME: pointer conversion?
            long long parsed_len = strtoll(len_string, (char**)&str_end, 10);
            if (
                (sizeof(size_t) < sizeof(parsed_len) && parsed_len >= (long long)(SIZE_MAX))
                || parsed_len < 0
                || str_end[0] != '\0'
                || errno == ERANGE
                || parsed_len == 0
            ) {
                raise("invalid number of cells: '%s'", len_string);
            }
            len = (size_t)parsed_len;
        }
    }

    FILE* file;
    if (path != NULL) {
        file = fopen(path, "r");
        if (file == NULL) {
            raise("failed to open file: %s (errno %d)", strerror(errno), errno);
        }
    } else {
        file = stdin;
    }

    if (len == 0) {
        len = 30000;
    }

    size_t source_code_len = 0;
    size_t next_read_len = 8;
    char* source_code = NULL;
    while (1) {
        next_read_len *= 2;
        source_code = realloc(source_code, source_code_len + next_read_len + 1);
        if (source_code == NULL) {
            raise("out of memory");
        }
        size_t nread = fread(source_code + source_code_len, 1, next_read_len, file);
        source_code_len += nread;
        if (nread != next_read_len) {
            if (ferror(file)) {
                raise("failed to read file");
            }
        }

        source_code[source_code_len] = '\0';
        if (nread == 0) {
            break;
        }
    }

    unsigned char* data = calloc(len, 1);
    if (data == NULL) {
        raise("out of memory");
    }

    run(source_code, data, len, 0, true);

    free(data);
    free(source_code);
    if (path != NULL) {
        fclose(file);
    }
}

const char* run(
    const char* body,
    unsigned char* data,
    size_t len,
    size_t index,
    bool is_root
) {
    const char* ip;
    for (ip = body; *ip != '\0';) {
        switch (*ip) {
            case '>':
                index++;
                if (index == len) {
                    index = 0;
                }
                ip++;
                break;
            
            case '<':
                if (index == 0) {
                    index = len - 1;
                } else {
                    index--;
                }
                ip++;
                break;

            case '+':
                data[index]++;
                ip++;
                break;

            case '-':
                data[index]--;
                ip++;
                break;

            case '.':
                putchar(data[index]);
                ip++;
                break;

            case ',':;
                int c = getchar();
                if (c == EOF) {
                    if (ferror(stdin)) {
                        raise("failed to read from stdin");
                    }
                    data[index] = 0;
                } else {
                    data[index] = c;
                }
                ip++;
                break;

            case '[':
                if (data[index]) {
                    ip = run(ip + 1, data, len, index, false);
                    continue;
                }
                size_t depth = 0;
                do {
                    if (*ip == '[') {
                        depth++;
                    } else if (*ip == ']') {
                        depth--;
                    }
                    ip++;
                    if (*ip == '\0') {
                        raise("unbalanced brackets");
                    }
                } while (depth != 0);
                break;

            case ']':
                if (is_root) {
                    raise("unbalanced brackets");
                }
                if (data[index]) {
                    ip = body;
                    continue;
                }
                return ip + 1;

            case ' ':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case '\v':
                ip++;
                break;

            default:
                raise("invalid character: '%c' (ASCII code 0x%x)", *ip, (int)*ip);
                break;
        }
    }

    if (!is_root) {
        raise("unbalanced brackets");
    }
    return ip;
}

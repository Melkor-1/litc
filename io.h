#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

/* 
 * To use, do this:
 *   #define IO_IMPLEMENTATION
 * before you include this file in *one* C to create the implementation.
 *
 * i.e. it should look like:
 * #include ...
 * #include ...
 *
 * #define IO_IMPLEMENTATION
 * #include "io.h"
 * ...
 * 
 * To make all functions have internal linkage, i.e. be private to the source
 * file, do this:
 *  #define `IO_STATIC` 
 * before including "io.h".
 *
 * i.e. it should look like:
 * #define IO_IMPLEMENTATION
 * #define IO_STATIC
 * #include "io.h"
 * ...
 *
 * You can #define IO_MALLOC, IO_REALLOC, and IO_FREE to avoid using malloc(),
 * realloc(), and free(). Note that all three must be defined at once, or none.
 */

#ifndef IO_DEF
    #ifdef IO_STATIC
        #define IO_DEF  static
    #endif              /* IO_STATIC */
#else
    #define IO_DEF  extern
#endif                  /* IO_DEF */

#if defined(__GNUC__) || defined(__clang__)
    #define ATTRIB_NONNULL(...)             __attribute__((nonnull (__VA_ARGS__)))
    #define ATTRIB_WARN_UNUSED_RESULT       __attribute__((warn_unused_result))
    #define ATTRIB_MALLOC                   __attribute__((malloc))
#else
    #define ATTRIB_NONNULL(...)             /**/
    #define ATTRIB_WARN_UNUSED_RESULT       /**/
    #define ATTRIB_MALLOC                   /**/ 
#endif                          /* defined(__GNUC__) || define(__clang__) */

#define IO_CHUNK_SIZE          (1024 * 64)

/* 
 * Reads the file pointed to by `stream` to a buffer and returns it.
 * The returned buffer is a nul-terminated string.
 * If `nbytes` is not NULL, it shall hold the size of the file. Otherwise it
 * shall hold 0.
 * 
 * Returns NULL on memory allocation failure. The caller is responsible for
 * freeing the returned pointer.
 */
IO_DEF char *io_read_file(FILE *stream, size_t *nbytes)
    ATTRIB_NONNULL(1) ATTRIB_WARN_UNUSED_RESULT ATTRIB_MALLOC;

/* 
 * Splits a string into a sequence of tokens. The `delim` argument 
 * specifies a set of bytes that delimit the tokens in the parsed string.
 * If `ntokens` is not NULL, it shall hold the amount of total tokens. Else it
 * shall hold 0.
 *
 * Returns an array of pointers to the tokens, or NULL on memory allocation
 * failure. The caller is responsible for freeing the returned pointer.
 */
IO_DEF char **io_split_by_delim(char *restrict s, const char *restrict delim,
    size_t *ntokens) 
    ATTRIB_NONNULL(1, 2) ATTRIB_WARN_UNUSED_RESULT ATTRIB_MALLOC;

/* 
 * Splits a string into lines.
 * A wrapper around `io_split_by_delim()`. It calls the function with "\n" as
 * the delimiter.
 *
 * Returns an array of pointers to the tokens, or NULL on memory allocation
 * failure. The caller is responsible for freeing the returned pointer.
 */
IO_DEF char **io_split_lines(char *s, size_t *nlines)
    ATTRIB_NONNULL(1) ATTRIB_WARN_UNUSED_RESULT ATTRIB_MALLOC;

/* 
 * Reads the next chunk of data from the stream referenced to by `stream`.
 * `chunk` must be a pointer to an array of size IO_CHUNK_SIZE. 
 *
 * Returns the number of bytes read on success, or zero elsewise. The chunk is 
 * not null-terminated.
 *
 * `io_read_next_chunk()` does not distinguish between end-of-file and error; the
 * routines `feof()` and `ferror()` must be used to determine which occured.
 */
IO_DEF size_t io_read_next_chunk(FILE stream[static 1], 
                                 char chunk[static IO_CHUNK_SIZE])
    ATTRIB_NONNULL(1, 2) ATTRIB_WARN_UNUSED_RESULT;

/* 
 * Reads the next line from the stream pointed to by `stream`. The returned line 
 * is terminated and does not contain a newline, if one was found.
 *
 * The memory pointed to by `size` shall contain the length of the 
 * line (including the terminating null character). Else it shall contain 0.
 *  
 * Upon successful completion a pointer is returned and the size of the line is 
 * stored in the memory pointed to by `size`, otherwise NULL is returned and
 * `size` holds 0.
 * 
 * `io_read_line()` does not distinguish between end-of-file and error; the routines
 * `feof()` and `ferror()` must be used to determine which occurred. The
 * function also returns NULL on a memory-allocation failure. 
 *
 * Although a null character is always supplied after the line, note that
 * `strlen(line)` will always be smaller than the value is `size` if the line
 * contains embedded null characters.
 */
IO_DEF char *io_read_line(FILE *stream, size_t *size)
    ATTRIB_NONNULL(1, 2) ATTRIB_WARN_UNUSED_RESULT ATTRIB_MALLOC;

/*
 * `size` should be a non-null pointer. On success, the function assigns `size`
 * with the number of bytes read and returns true, or returns false elsewise.
 * The function also returns false if the size of the file can not be
 * represented.
 *
 * Note: The file can grow between io_fsize() and a subsequent read.
 */
IO_DEF bool io_fsize(FILE *stream, uintmax_t *size) 
    ATTRIB_NONNULL(1, 2) ATTRIB_WARN_UNUSED_RESULT;

/* 
 * Writes `lines` to the file pointed to by `stream`.
 * A wrapper around 
 * On success, it returns true, or false elsewise.
 */
IO_DEF bool io_write_lines(FILE *stream, size_t nlines, char *lines[const static nlines]) 
    ATTRIB_NONNULL(1, 3);

/* 
 * Writes nbytes from the buffer pointed to by `data` to the file pointed to 
 * by `stream`. 
 *
 * On success, it returns true, or false elsewise.
 */
IO_DEF bool io_write_file(FILE *stream, size_t nbytes, const char data[static nbytes]) 
    ATTRIB_NONNULL(1, 3);

#endif                          /* IO_H */

#ifdef IO_IMPLEMENTATION

#if defined(IO_MALLOC) != defined(IO_REALLOC) || defined(IO_REALLOC) != defined(IO_FREE)
    #error  "Must define all or none of IO_MALLOC, IO_REALLOC, and IO_FREE."
#endif

#ifndef IO_MALLOC
#define IO_MALLOC(sz)       malloc(sz)
#define IO_REALLOC(p, sz)   realloc(p, sz)
#define IO_FREE(p)          free(p)
#endif

#undef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#define IO_TOKEN_CHUNK_SIZE    (1024 * 2)

#define GROW_CAPACITY(capacity, initial) \
        ((capacity) < initial ? initial : (capacity) * 2)

IO_DEF char *io_read_file(FILE *stream, size_t *nbytes)
{
    char *content = NULL;
    size_t len = 0;
    size_t capacity = 0;

    if (nbytes) {
        *nbytes = 0;
    }

    for (size_t rcount = 1; rcount > 0; len += rcount) {
        capacity = GROW_CAPACITY(capacity, IO_CHUNK_SIZE);

        void *const tmp = IO_REALLOC(content, capacity + 1);

        if (tmp == NULL) {
            IO_FREE(content);
            return NULL;
        }
        content = tmp;
        rcount = fread(content + len, 1, capacity - len, stream);

        if (rcount < capacity - len) {
            if (!feof(stream)) {
                IO_FREE(content);
                return content = NULL;
            }
            /* If we break on the first iteration. */
            len += rcount;
            break;
        }
    }

    if (nbytes) {
        *nbytes = len;
    }
    content[len] = '\0';
    return content;
}

IO_DEF char **io_split_by_delim(char *restrict s, const char *restrict delim,
    size_t *ntokens)
{
    char **tokens = NULL;
    size_t capacity = 0;
    size_t token_count = 0;

    if (ntokens) {
        *ntokens = 0;
    }

    while (s != NULL && *s != '\0') {
        if (token_count >= capacity) {
            capacity = GROW_CAPACITY(capacity, IO_TOKEN_CHUNK_SIZE);
            char **const tmp = IO_REALLOC(tokens, sizeof *tokens * capacity);

            if (tmp == NULL) {
                IO_FREE(tokens);
                return NULL;
            }
            tokens = tmp;
        }
        tokens[token_count++] = s;
        s = strpbrk(s, delim);

        if (s) {
            *s++ = '\0';
        }
    }

    if (ntokens) {
        *ntokens = token_count;
    }
    return tokens;
}

IO_DEF char **io_split_lines(char *s, size_t *nlines)
{
    return io_split_by_delim(s, "\n", nlines);
}

IO_DEF size_t io_read_next_chunk(FILE stream[static 1], 
                                 char chunk[static IO_CHUNK_SIZE])
{
    const size_t rcount = fread(chunk, 1, IO_CHUNK_SIZE, stream);

    if (rcount < IO_CHUNK_SIZE) {
        if (!feof(stream)) {
            /* A read error occured. */
            return 0;
        }

        if (rcount == 0) {
            return 0;
        }
    }
    
    return rcount;
}

IO_DEF char *io_read_line(FILE *stream, size_t *size)
{
    size_t count = 0;
    size_t capacity = 0;
    char *line = NULL;

    for (;;) {
        if (count >= capacity) {
            capacity = GROW_CAPACITY(capacity, BUFSIZ);
            char *const tmp = realloc(line, capacity + 1);

            if (tmp == NULL) {
                free(line);
                return NULL;
            }

            line = tmp;
        }

        const int c = getc(stream);

        if (c == EOF || c == '\n') {
            if (c == EOF) {
                if (feof(stream)) {
                    if (!count) {
                        free(line);
                        return NULL;
                    }
                    /* Return what was read. */
                    break;
                }
                /* Read error. */
                free(line);
                return NULL;
            } else {
                break;
            }
        } else {
            line[count] = (char) c;
        }
        ++count;
    }

    /* Shrink line to size if possible. */
    void *const tmp = realloc(line, count + 1);

    if (tmp) {
        line = tmp;
    }

    line[count] = '\0';
    *size = ++count;
    return line;
}

/* 
 * Reasons to not use `fseek()` and `ftell()` to compute the size of the file:
 * 
 * Subclause 7.12.9.2 of the C Standard [ISO/IEC 9899:2011] specifies the
 * following behavior when opening a binary file in binary mode:
 * 
 * >> A binary stream need not meaningfully support fseek calls with a whence 
 * >> value of SEEK_END.
 *
 * In addition, footnote 268 of subclause 7.21.3 says:
 *
 * >> Setting the file position indicator to end-of-file, as with 
 * >> fseek(file, 0, SEEK_END) has undefined behavior for a binary stream.
 *
 * For regular files, the file position indicator returned by ftell() is useful
 * only in calls to fseek. As such, the value returned may not be reflect the 
 * physical byte offset. 
 *
 */
bool io_fsize(FILE *stream, uintmax_t *size)
{
/*
 *   Windows supports fileno(), struct stat, and fstat() as _fileno(),
 *   _fstat(), and struct _stat.
 *
 *   See: https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/fstat-fstat32-fstat64-fstati64-fstat32i64-fstat64i32?view=msvc-170
 */

#ifdef _WIN32
    #define fileno _fileno
    #ifdef _WIN64
        #define fstat  _fstat64
        #define stat   __stat64
    #else
        /* Does this suffice for a 32-bit system? */
        #define fstat  _fstat
        #define stat   _stat
    #endif                          /* WIN64 */
#endif                              /* _WIN32 */

/* According to https://web.archive.org/web/20191012035921/http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system
 * __unix__ should suffice for IBM AIX, all distributions of BSD, and all
 * distributions of Linux, and Hewlett-Packard HP-UX. __unix suffices for Oracle
 * Solaris. Mac OSX and iOS compilers do not define the conventional __unix__,
 * __unix, or unix macros, so they're checked for separately. WIN32 is defined
 * on 64-bit systems too.
 */
#if defined(_WIN32) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__))
    struct stat st;

    /* rewind() returns no value. */
    rewind(stream);

    if (fstat(fileno(stream), &st) == 0) {
        *size = (uintmax_t) st.st_size;
        return true;
    }
    return false;
#else
    /* Fall back to the default and read it in chunks. */
    uintmax_t rcount = 0;
    char chunk[IO_CHUNK_SIZE];

    /* rewind() returns no value. */
    rewind(stream);

    do {
        rcount = fread(chunk, 1, IO_CHUNK_SIZE, stream);

        if ((*size + rcount) < *size) {
            /* Overflow. */
            return false;
        }
        *size += rcount;
    } while (rcount == IO_CHUNK_SIZE);
    return !ferror(stream);
#endif                          /* defined(_WIN32) || defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)) */
#undef fstat
#undef stat
#undef fileno
}

IO_DEF bool io_write_lines(FILE *stream, size_t nlines, 
        char *lines[const static nlines])
{
    for (size_t i = 0; i < nlines; ++i) {
        if (fprintf(stream, "%s\n", lines[i]) < 0) {
            return false;
        }
    }

    return true;
}

IO_DEF bool io_write_file(FILE *stream, size_t nbytes,
    const char data[static nbytes])
{
    return fwrite(data, 1, nbytes, stream) == nbytes;
}

#undef ATTRIB_NONNULL
#undef ATTRIB_WARN_UNUSED_RESULT
#undef ATTRIB_MALLOC
#undef TOKEN_IO_CHUNK_SIZE
#undef GROW_CAPACITY
#endif                          /* IO_IMPLEMENTATION */

#ifdef TEST_MAIN

#include <assert.h>

int main(int argc, char **argv)
{
    if (argc != 2) {
        fputs("Error: file argument missing.\n", stderr);
        return EXIT_FAILURE;
    }

    FILE *fp = fopen(argv[1], "r");
    if (!fp) {
        perror(argv[1]);
        return EXIT_FAILURE;
    }
    
    size_t nbytes = 0;
    char *const fbuf = io_read_file(fp, &nbytes);
    assert(fbuf && nbytes);
    assert(io_write_file(stdout, nbytes, fbuf));
    rewind(fp);

    size_t size = 0;
    bool rv = io_fsize(fp, &size);
    assert(rv);
    printf("Filesize: %zu.\n", size);

    size_t nlines = 0;
    char **lines = io_split_lines(fbuf, &nlines);
    assert(lines && nlines);
    assert(io_write_lines(stdout, nlines, lines));

    printf("Lines read: %zu.\n", nlines);

    for (size_t i = 0; i < nlines; ++i) {
        if (lines[i][0]) {
            size_t ntokens = 0;
            char **tokens = io_split_by_delim(lines[i], " \t", &ntokens);
            assert(tokens && ntokens);
            assert(io_write_lines(stdout, ntokens, tokens));
            free(tokens);
        }
    }
    
    rewind(fp);
 
    /* This can be allocated dynamically on the heap too. */
    char chunk[IO_CHUNK_SIZE];
    size_t chunk_size = -1;
    
    while (chunk_size = io_read_next_chunk(fp, chunk)) {
        printf("Read a chunk of size: %zu.\n", chunk_size); 
        puts(chunk);
    }
    
    rewind(fp);

    char *line = NULL;
    size_t line_size = 0;

    while ((line = io_read_line(fp, &line_size))) {
        line[strcspn(line, "\n")] = '\0';
        printf("Read a line of size: %zu.\n", line_size); 
        puts(line);
        putchar('\n');
        free(line);
    }

    free(fbuf);
    free(lines);    
    fclose(fp);
    
    return EXIT_SUCCESS;
}

#endif                          /* TEST_MAIN */

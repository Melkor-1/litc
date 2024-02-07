#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/* The function returns true if we are writing to stdout; if not then it only
 * returns true if we successfully closed the stream. Or viewed another way, it 
 * returns false only if we needed to close the stream but failed when doing so.
 */
static bool write_codelines(FILE *out, size_t nlines, char *lines[static nlines])
{
    bool code_mode = false;

    for (size_t i = 0; i < nlines; ++i) {
        if (code_mode) {
            if (!strcmp(lines[i], "\\end{code}")) {
                if (fprintf(out, "// %s\n", lines[i]) < 0) {
                    perror("fprintf()");
                    return false;
                }
                code_mode = false;
            } else {
                if (fprintf(out, "%s\n", lines[i]) < 0) {
                    perror("fprintf()");
                    return false;
                }
            }
        } else {
            if (fprintf(out, "// %s\n", lines[i]) < 0) {
                perror("fprintf()");
                return false;
            }
            
            if (!strcmp(lines[i], "\\begin{code}")) {
                code_mode = true;
            }
        }
    } 

    return out == stdout || !fclose(out);
}

static char *read_file(FILE * fp, size_t *nbytes)
{
    static const size_t page_size = 4096;
    char *content = NULL;
    size_t len = 0;

    for (size_t rcount = 1; rcount; len += rcount) {
        void *const tmp = realloc(content, len + page_size);

        if (!tmp) {
            free(content);
            content = NULL;
/* ENOMEM is not a C standard error code, yet quite common. */
#ifdef ENOMEM
            errno = ENOMEM;
#endif
            return NULL;
        }
        content = tmp;
        rcount = fread(content + len, 1, page_size - 1, fp);

        if (ferror(fp)) {
            free(content);
            content = NULL;
#ifdef ENOMEM
            errno = ENOMEM;
#endif
            return NULL;
        }
    }
    content[len] = '\0';
    *nbytes = len;
    return content;
}

static size_t split_lines(char *restrict s, char ***restrict lines)
{
    const size_t chunk_size = 1024;
    size_t capacity = 0;
    size_t line_count = 0;

    while (s && *s) {
        if (line_count >= capacity) {
            char **const new = realloc(*lines,
                sizeof **lines * (capacity += chunk_size));

            if (!new) {
                free(*lines);
                *lines = NULL;
#ifdef ENOMEM
                errno = ENOMEM;
#endif
                return 0;
            }
            *lines = new;
        }
        (*lines)[line_count++] = s;
        s = strchr(s, '\n');

        if (s) {
            *s++ = '\0';
        }
    }
    return line_count;
}

static FILE *xfopen(const char *path)
{
    errno = 0;
    FILE *const fp = fopen(path, "r");

    if (!fp) {
        fprintf(stderr, "Error: could not open file %s: %s.\n", path,
            errno ? strerror(errno) : "");
        exit(EXIT_FAILURE);
    }
    return fp;
}

int main(int argc, char **argv)
{
    /* Sanity check. POSIX requires the invoking process to pass a non-NULL 
     * argv[0]. 
     */
    if (!argv[0]) {
        fprintf(stderr,
            "A NULL argv[0] was passed through an exec system call.\n");
        return EXIT_FAILURE;
    }

    if (argc != 2) {
        fprintf(stderr, "Error: no file provided.\n"
            "Usage: %s <filename>.\n", argv[0]);
        return EXIT_FAILURE;
    }
    
    const char *input_path= argv[1];
    FILE *const input_fp = xfopen(input_path);
    size_t nbytes = 0;
    char *const content = read_file(input_fp, &nbytes);
    char **lines = NULL;
    const size_t num_lines = split_lines(content, &lines);

    if (!lines) {
        perror("fread()");
        fclose(input_fp);
        free(content);
        return EXIT_FAILURE;
    }
    
    if (!write_codelines(stdout, num_lines, lines)) {
        fclose(input_fp);
        free(content);
        free(lines);
        return EXIT_FAILURE;
    }

    fclose(input_fp);
    free(content);
    free(lines);
    return EXIT_SUCCESS;
}

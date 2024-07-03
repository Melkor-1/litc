#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE

#define _POSIX_C_SOURCE 200819L
#define _XOPEN_SOURCE   700

#define IO_IMPLEMENTATION
#define IO_STATIC
#include "io.h"

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* In C2X/C23 or later, nullptr is a keyword.
 * Patch up C18 (__STDC_VERSION__ == 201710L) and earlier versions.
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ <= 201710L
    #define nullptr ((void *)0)
#endif

static const char *argv0            = "litc";
static const char short_options[]   = "b:e:o:h";

static const struct option long_options[] = {
    { "begin",  required_argument,   nullptr,    'b' },
    { "end",    required_argument,   nullptr,    'e' },
    { "help",   no_argument,         nullptr,    'h' },
    { "output", required_argument,   nullptr,    'o' },
    { nullptr,  0,                   nullptr,     0  },
};

#define DEFAULT_BEGIN   "\\begin{code}"
#define DEFAULT_END     "\\end{code}"

static const char *bflag = DEFAULT_BEGIN;       /* Begin flag. */
static const char *eflag = DEFAULT_END; /* End flag. */
static FILE *output = NULL;     /* Output file stream. */
static FILE *input = NULL;      /* Input file stream. */
const char *input_fn = "stdin";

static void help(void)
{
    printf("Usage: %s [OPTIONS] SRC\n\n"
        "  %s - extract code from a LaTEX document.\n\n"
        "Options:\n"
        "  -b, --begin=MARKER   Line that denotes the beginning of the code\n"
        "                       block in the markup langugage. Default: %s.\n"
        "  -e, --end=MARKER     Line that denotes the end of the code block in\n"
        "                       the markup language. Default: %s.\n"
        "  -h, --help           Displays this message and exits.\n"
        "  -o, --output=FILE    Writes result to FILE instead of standard output.\n\n"
        "Note: The begin and end markers must be different.\n\n"
        "For Markdown, they can be:\n"
        "  ```python\n"
        "  # Some code here\n"
        "  ```\n", argv0, argv0, DEFAULT_BEGIN, DEFAULT_END);
    exit(EXIT_SUCCESS);
}

static void err_and_fail(void)
{
    fprintf(stderr, "The syntax of the command is incorrect.\n"
        "Use: %s -h for more information.\n", argv0);
    exit(EXIT_FAILURE);
}

static void parse_options(int argc, char *argv[static argc])
{
    int c = 0;

    output = stdout;

    while ((c =
            getopt_long(argc, argv, short_options, long_options,
                nullptr)) != -1) {
        switch (c) {
            case 'e':
            case 'b':
                if (optarg == nullptr) {
                    err_and_fail();
                }
                *(c == 'b' ? &bflag : &eflag) = optarg;
                break;
            case 'h':
                help();
                break;
            case 'o':
                /* If -o was provided more than once. */
                if (output != stdout) {
                    fprintf(stderr, "%s: error: multiple -o flags provided.\n", argv0);
                    err_and_fail();
                }

                errno = 0;
                output = fopen(optarg, "a");

                if (output == nullptr) {
                    fprintf(stderr,
                        "%s: failed to open file '%s' for writing: %s\n", argv0,
                        optarg, errno ? strerror(errno) : "unknown error");
                    exit(EXIT_FAILURE);
                }
                break;

                /* case '?' */
            default:
                err_and_fail();
                break;
        }

        if (strcmp(eflag, bflag) == 0) {
            fprintf(stderr, "%s: the start and end markers must be different "
                "(both are '%s')\n", argv0, eflag);
            exit(EXIT_FAILURE);
        }
    }
}

static int process(size_t nlines, char *lines[static nlines])
{
    /* ftruncate() sets errno. */
    if (output != stdout && ftruncate(fileno(output), 0) != 0) {
        perror("ftruncate()");
        return EXIT_FAILURE;
    }

    enum { COMMENT, CODE } mode = COMMENT;
    size_t lineno = 1;
    size_t num_code_blocks = 0;

    for (size_t i = 0; i < nlines; ++i, ++lineno) {
        if (mode == CODE) {
            if (strcmp(lines[i], bflag) == 0) {
                fprintf(stderr,
                    "%s: error: found begin marker '%s' in file '%s' line %zu in code mode.\n",
                    argv0, bflag, input_fn, lineno);
                return EXIT_FAILURE;
            } else if (strcmp(lines[i], eflag) == 0) {
                mode = COMMENT;
            } else if (fprintf(output, "%s\n", lines[i]) < 0) {
                fprintf(stderr,
                    "%s: error: failed to write to output file while reading file '%s' line %zu.\n",
                    argv0, input_fn, lineno);
                return EXIT_FAILURE;
            }
        } else {
            if (strcmp(lines[i], eflag) == 0) {
                fprintf(stderr,
                    "%s: error: found end marker '%s' in file '%s' line %zu while in comment mode.\n.",
                    argv0, eflag, input_fn, lineno);
                return EXIT_FAILURE;
            } else if (strcmp(lines[i], bflag) == 0) {
                ++num_code_blocks;
                mode = CODE;
            }

        }
    }

    if (mode != COMMENT) {
        fprintf(stderr, "%s: file '%s' missing a code end marker '%s'.\n",
            argv0, input_fn, eflag);
        return EXIT_FAILURE;
    }

    if (num_code_blocks == 0) {
        fprintf(stderr, "%s: file '%s' contained zero code blocks.\n", argv0,
            input_fn);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[])
{
    /* Sanity check. POSIX requires the invoking process to pass a non-null
     * argv[0]. 
     */
    if (argv[0] == nullptr) {
        fputs("A NULL argv[0] was passed through an exec system call.\n",
            stderr);
        return EXIT_FAILURE;
    }

    int status = EXIT_FAILURE;

    argv0 = argv[0];
    input = stdin;

    parse_options(argc, argv);

    if ((optind + 1) == argc) {
        errno = 0;
        input = fopen(argv[optind], "r");
        input_fn = argv[optind];
        if (input == nullptr) {
            fprintf(stderr, "%s: failed to read file '%s' for reading: %s\n",
                argv0, input_fn, errno ? strerror(errno) : "unknown error");
            goto cleanup_and_fail;
        }
    }

    else if (optind > argc) {
        err_and_fail();
    }

    // Since we are already assuming POSIX support, we can just dump io.h and
    // use getline().
    errno = 0;
    size_t nbytes = 0;
    char *const content = io_read_file(input, &nbytes);
    size_t nlines = 0;
    char **lines = io_split_lines(content, &nlines);

    if (lines == 0) {
        fprintf(stderr, "%s: failed to read file '%s': %s\n",
            argv0, input_fn, errno ? strerror(errno) : "unknown error");
        goto cleanup_and_fail;
    }

    if (process(nlines, lines) != EXIT_SUCCESS) {
        goto cleanup_and_fail;
    }

    status = EXIT_SUCCESS;

  cleanup_and_fail:
    if (input != stdin) {
        fclose(input);
    }

    if (output != stdout) {
        if (fclose(output) != 0) {
            fprintf(stderr, "%s: failed to close output file.\n", argv0);
            return status;
        }
    }

    return status;
}

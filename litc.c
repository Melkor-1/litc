#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE

#define _POSIX_C_SOURCE 200819L
#define _XOPEN_SOURCE   700

#define IO_IMPLEMENTATION
#define IO_STATIC
#include "io.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include <unistd.h>
#include <getopt.h>

/* C2X/C23 or later? */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202000L
    #include <stddef.h>             /* nullptr_t */
#else
    #include <stdbool.h>            /* bool, true, false */
    #define nullptr ((void *)0)
    typedef void *nullptr_t;
#endif                          /* nullptr, nullptr_t */

#define SHORT_OP_LIST   "b:e:o:h"
#define DEFAULT_BEGIN   "\\begin{code}"
#define DEFAULT_END     "\\end{code}"

/* Do we even need getopt? */
typedef struct {
    const char *bflag;          /* Begin flag. */
    const char *eflag;          /* End flag. */
    FILE *output;               /* Output to FILE. */
} flags;

static void help(void)
{
    printf("Usage: litc [OPTIONS] SRC\n\n"
        "  litc - extract code from a LaTEX document.\n\n"
        "Options:\n"
        "  -b, --begin          Line that denotes the beginning of the code\n"
        "                       block in the markup langugage. Default: %s.\n"
        "  -e, --end            Line that denotes the end of the code block in\n" 
        "                       the markup language. Default: %s.\n"
        "  -h, --help           Displays this message and exits.\n"
        "  -o, --output=FILE    Writes result to FILE instead of standard output.\n\n"
        "Note: The begin and end markers must be different.\n\n"
        "For Markdown, they can be:\n"
        "  ```python\n"
        "  # Some code here\n"
        "  ```\n",
        DEFAULT_BEGIN, DEFAULT_END);
    exit(EXIT_SUCCESS);
}

static void err_and_fail(void)
{
    fputs("The syntax of the command is incorrect.\n"
        "Try litc -h for more information.\n", stderr);
    exit(EXIT_FAILURE);
}

static void parse_options(const struct option   long_options[static 1],
                          flags                 opt_ptr[static 1], 
                          int                   argc, 
                          char *                argv[static argc])
{
    while (true) {
        const int c =
            getopt_long(argc, argv, SHORT_OP_LIST, long_options, nullptr);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'e':
            case 'b':
                if (optarg == nullptr) {
                    err_and_fail();
                }
                *(c == 'b' ? &opt_ptr->bflag : &opt_ptr->eflag) = optarg;
                break;
            case 'h':
                help();
                break;
            case 'o':
                /* If -o was provided more than once. */
                if (opt_ptr->output != stdout) {
                    fprintf(stderr, "Error: Multiple -o flags provided.\n");
                    err_and_fail();
                }

                errno = 0;
                opt_ptr->output = fopen(optarg, "a");

                if (opt_ptr->output == nullptr) {
                    perror(optarg);
                    exit(EXIT_FAILURE);
                }
                break;

                /* case '?' */
            default:
                err_and_fail();
                break;
        }
    }
}

static bool write_codelines(flags   options[static 1],
                            size_t  nlines, 
                            char *  lines[static nlines])
{
    /* Would these 7 variables be better expressed as a struct? */
    /* Perhaps:
     *
     * What would be a better name for this struct?
     *
     * struct {
     *     FILE *const f;
     *     const char *const begin_marker;
     *     const char *const end_marker;
     *     size_t ncodelines;       // No. of lines seen so far. 
     *     size_t curr_line;
     *     size_t last_begin_pos;
     *     bool code_mode;
     * } code = {options->output,
     *           options->bflag ? options->bflag : DEFAULT_BEGIN,
     *           options->eflag ? options->eflag : DEFAULT_END,
     *           1,
     *           1,
     *           0,
     *           false
     * }; 
     */
    const char *const begin_marker =
        options->bflag ? options->bflag : DEFAULT_BEGIN;
    const char *const end_marker =
        options->eflag ? options->eflag : DEFAULT_END;
    FILE *const f = options->output;

    if (f != stdout && ftruncate(fileno(f), 0)) {
        perror("seek()");
        return false;
    }

    bool code_mode = false;
    size_t ncodelines = 1;
    size_t curr_line = 1;
    size_t last_begin_pos = 0;

    for (size_t i = 0; i < nlines; ++i, ++curr_line) {
        if (code_mode) {
            if (strcmp(lines[i], begin_marker) == 0) {
                fprintf(stderr, "Error: %s missing, %s started at line: %zu.\n",
                    end_marker, begin_marker, last_begin_pos);
                goto cleanup_and_fail;
            }

            if (strcmp(lines[i], end_marker) == 0) {
                code_mode = false;
            } else {
                if (fprintf(f, "%s\n", lines[i]) < 0) {
                    perror("fprintf()");
                    goto cleanup_and_fail;
                }
                ++ncodelines;
            }
        } else {
            if (strcmp(lines[i], end_marker) == 0) {
                fprintf(stderr, "Error: spurious %s at line: %zu.\n",
                    end_marker, curr_line);
                goto cleanup_and_fail;
            }

            code_mode = strcmp(lines[i], begin_marker) == 0;

            if (code_mode) {
                /* Only print newlines after the first code block. */
                if (ncodelines > 1) {
                    fputc('\n', f);
                }

                last_begin_pos = curr_line;
            }
        }
    }

    if (code_mode) {
        fprintf(stderr, "Error: %s missing. %s started at line: %zu.\n",
            end_marker, begin_marker, last_begin_pos);
        goto cleanup_and_fail;
    }

    if (ncodelines == 0) {
        fprintf(stderr, "Error: no code blocks were found in the file.\n");
        goto cleanup_and_fail;
    }

    goto success;

  cleanup_and_fail:
    if (f != stdout) {
        fclose(f);
    }

    return false;

  success:
    return f == stdout || !fclose(f);
}

int main(int argc, char *argv[])
{

    /* Sanity check. POSIX requires the invoking process to pass a non-null
     * argv[0]. 
     */
    if (!argv) {
        fputs("A NULL argv[0] was passed through an exec system call.\n",
            stderr);
        return EXIT_FAILURE;
    }

    static const struct option long_options[] = {
        { "begin",  required_argument,  nullptr,    'b' },
        { "end",    required_argument,  nullptr,    'e' },
        { "help",   no_argument,        nullptr,    'h' },
        { "output", required_argument,  nullptr,    'o' },
        { nullptr,  0,                  nullptr,     0  },
    };

    FILE *in_file = stdin;
    flags options = { nullptr, nullptr, stdout };

    parse_options(long_options, &options, argc, argv);

    if ((optind + 1) == argc) {
        in_file = fopen(argv[optind], "r");
        if (!in_file) {
            perror(argv[optind]);

            if (options.output) {
                fclose(options.output);
            }
            return EXIT_FAILURE;
        }
    }

    else if (optind > argc) {
        err_and_fail();
    }

    size_t nbytes = 0;
    char *const content = io_read_file(in_file, &nbytes);
    size_t nlines = 0;
    char **lines = io_split_lines(content, &nlines);
    int status = EXIT_FAILURE;

    if (!lines) {
        perror("fread()");
        goto cleanup_and_fail;
    }

    if (!write_codelines(&options, nlines, lines)) {
        goto cleanup_and_fail;
    }

    status = EXIT_SUCCESS;

  cleanup_and_fail:
    /* As we're exiting, we don't need to free anything. */
    /* free(lines); */
    /* free(content); */

    if (in_file != stdin) {
        fclose(in_file);
    }

    return status;
}

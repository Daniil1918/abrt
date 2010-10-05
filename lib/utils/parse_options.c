
#include <getopt.h>
#include "abrtlib.h"
#include "parse_options.h"

static int parse_opt_size(const struct options *opt)
{
    unsigned size = 1;
    for (; opt->type != OPTION_END; opt++)
        size++;

    return size;
}

#define USAGE_OPTS_WIDTH 24
#define USAGE_GAP         2

void parse_usage_and_die(const char * const * usage, const struct options *opt)
{
    fprintf(stderr, _("usage: %s\n"), *usage++);

    while (*usage && **usage)
        fprintf(stderr, _("   or: %s\n"), *usage++);

    fputc('\n', stderr);

    for (; opt->type != OPTION_END; opt++)
    {
        size_t pos;
        int pad;

        pos = fprintf(stderr, "    ");
        if (opt->short_name)
            pos += fprintf(stderr, "-%c", opt->short_name);

        if (opt->short_name && opt->long_name)
            pos += fprintf(stderr, ", ");

        if (opt->long_name)
            pos += fprintf(stderr, "--%s", opt->long_name);

        if (opt->argh)
            pos += fprintf(stderr, " <%s>", opt->argh);

        if (pos <= USAGE_OPTS_WIDTH)
            pad = USAGE_OPTS_WIDTH - pos;
        else
        {
            fputc('\n', stderr);
            pad = USAGE_OPTS_WIDTH;
        }
        fprintf(stderr, "%*s%s\n", pad + USAGE_GAP, "", opt->help);
    }
    fputc('\n', stderr);
    exit(1);
}

void parse_opts(int argc, char **argv, const struct options *opt,
                const char * const usage[])
{
    int size = parse_opt_size(opt);

    struct strbuf *shortopts = strbuf_new();

    struct option *longopts = xcalloc(sizeof(struct options), size);
    int ii;
    for (ii = 0; ii < size - 1; ++ii)
    {
        longopts[ii].name = opt[ii].long_name;

        switch (opt[ii].type)
        {
            case OPTION_BOOL:
                longopts[ii].has_arg = no_argument;
                if (opt[ii].short_name)
                    strbuf_append_char(shortopts, opt[ii].short_name);
                break;
            case OPTION_STRING:
                longopts[ii].has_arg = required_argument;
                if (opt[ii].short_name)
                    strbuf_append_strf(shortopts, "%c:", opt[ii].short_name);
                break;
            case OPTION_END:
                break;
            case OPTION_INTEGER:
                break;
        }
        longopts[ii].flag = 0;
        longopts[ii].val = opt[ii].short_name;
    }

    longopts[ii].name = 0;
    longopts[ii].has_arg = 0;
    longopts[ii].flag = 0;
    longopts[ii].val = 0;

    int option_index = 0;
    while (1)
    {
        int c = getopt_long(argc, argv, shortopts->buf, longopts, &option_index);

        if (c == -1)
            break;

        if (c == '?')
        {
            free(longopts);
            strbuf_free(shortopts);
            parse_usage_and_die(usage, opt);
        }

        for (ii = 0; ii < size - 1; ++ii)
        {
            if (opt[ii].short_name == c)
            {

                switch (opt[ii].type)
                {
                    case OPTION_BOOL:
                        *(int*)opt[ii].value += 1;
                        break;
                    case OPTION_INTEGER:
                        *(int*)opt[ii].value = xatoi(optarg);
                        break;
                    case OPTION_STRING:
                        if (optarg)
                            *(char**)opt[ii].value = (char*)optarg;
                        break;
                    case OPTION_END:
                        break;
                }
            }
        }
    }

    free(longopts);
    strbuf_free(shortopts);
}

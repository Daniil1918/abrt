/*
    SOSreport.cpp

    Copyright (C) 2009  RedHat inc.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#include "abrtlib.h"
#include "abrt_types.h"
#include "abrt_exception.h"
#include "SOSreport.h"
#include "debug_dump.h"
#include "abrt_exception.h"
#include "comm_layer_inner.h"

using namespace std;

static char *ParseFilename(char *p)
{
    /*
    the sosreport's filename is embedded in sosreport's output.
    It appears on the line after the string in 'sosreport_filename_marker',
    it has leading spaces, and a trailing newline.  This function trims
    any leading and trailing whitespace from the filename.
    */
    static const char sosreport_filename_marker[] =
          "Your sosreport has been generated and saved in:";

    p = strstr(p, sosreport_filename_marker);
    if (!p)
        return p;
    p = skip_whitespace(p + sizeof(sosreport_filename_marker)-1);
    char *end = strchrnul(p, '\n');
    while (end > p && isspace(*end))
        *end-- = '\0';
    return p[0] == '/' ? p : NULL;
}

void CActionSOSreport::Run(const char *pActionDir, const char *pArgs, int force)
{
    if (!force)
    {
        CDebugDump dd;
        dd.Open(pActionDir);
        bool bt_exists = dd.Exist("sosreport.tar.bz2") || dd.Exist("sosreport.tar.xz");
        if (bt_exists)
        {
            VERB3 log("%s already exists, not regenerating", "sosreport.tar.bz2");
            return;
        }
    }

    static const char command_default[] =
        "cd -- '%s' || exit 1;"
        "nice sosreport --tmp-dir . --batch"
        " --only=anaconda --only=bootloader"
        " --only=devicemapper --only=filesys --only=hardware --only=kernel"
        " --only=libraries --only=memory --only=networking --only=nfsserver"
        " --only=pam --only=process --only=rpm -k rpm.rpmva=off --only=ssh"
        " --only=startup --only=yum 2>&1;"
        "rm sosreport*.md5 2>/dev/null;"
        "mv sosreport*.tar.bz2 sosreport.tar.bz2 2>/dev/null;"
        "mv sosreport*.tar.xz  sosreport.tar.xz  2>/dev/null;"
        ;
    static const char command_prefix[] =
        "cd -- '%s' || exit 1;"
        "nice sosreport --tmp-dir . --batch %s 2>&1;"
        "rm sosreport*.md5 2>/dev/null;"
        "mv sosreport*.tar.bz2 sosreport.tar.bz2 2>/dev/null;"
        "mv sosreport*.tar.xz  sosreport.tar.xz  2>/dev/null;"
        ;
    string command;

    vector_string_t args;
    parse_args(pArgs, args, '"');

    if (args.size() == 0 || args[0] == "")
    {
        command = ssprintf(command_default, pActionDir);
    }
    else
    {
        command = ssprintf(command_prefix, pActionDir, args[0].c_str());
    }

    update_client(_("Running sosreport: %s"), command.c_str());
    string output = command;
    output += '\n';
    char *command_out = run_in_shell_and_save_output(/*flags:*/ 0, command.c_str(), /*dir:*/ NULL, /*size_p:*/ NULL);
    output += command_out;
    update_client(_("Finished running sosreport"));
    VERB3 log("sosreport output:'%s'", output.c_str());

// Not needed: now we use "sosreport --tmp-dir DUMPDIR"
#if 0
    // Parse:
    // "Your sosreport has been generated and saved in:
    //  /tmp/sosreport-XXXX.tar.bz2"
    // Note: ParseFilename modifies its parameter and returns pointer
    // which points somewhere inside it.
    char *sosreport_filename = xstrdup(ParseFilename(command_out));
    free(command_out);
    if (!sosreport_filename)
    {
        throw CABRTException(EXCEP_PLUGIN, "Can't find filename in sosreport output");
    }

    string sosreport_dd_filename = concat_path_file(pActionDir, "sosreport.tar");
    char *ext = strrchr(sosreport_filename, '.');
    if (ext && strcmp(ext, ".tar") != 0)
    {
        // Assuming it's .bz2, .gz or some such
        sosreport_dd_filename += ext;
    }
    CDebugDump dd;
    dd.Open(pActionDir);
    //Not useful: dd.SaveText("sosreportoutput", output);
    off_t sz = copy_file(sosreport_filename, sosreport_dd_filename.c_str(), 0644);

    // don't want to leave sosreport-XXXX.tar.bz2 in /tmp
    unlink(sosreport_filename);
    // sosreport-XXXX.tar.bz2.md5 too
    unsigned len = strlen(sosreport_filename);
    sosreport_filename = (char*)xrealloc(sosreport_filename, len + sizeof(".md5")-1 + 1);
    strcpy(sosreport_filename + len, ".md5");
    unlink(sosreport_filename);

    if (sz < 0)
    {
        dd.Close();
        CABRTException e(EXCEP_PLUGIN,
                "Can't copy '%s' to '%s'",
                sosreport_filename,
                sosreport_dd_filename.c_str()
        );
        free(sosreport_filename);
        throw e;
    }
    free(sosreport_filename);
#endif
}

PLUGIN_INFO(ACTION,
            CActionSOSreport,
            "SOSreport",
            "0.0.2",
            "Runs sosreport, saves the output",
            "gavin@redhat.com",
            "https://fedorahosted.org/abrt/wiki",
            "");

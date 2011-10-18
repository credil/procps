// Copyright 2004 Nicholas Miell
//
// This file may be used subject to the terms and conditions of the
// GNU Library General Public License Version 2 as published by the
// Free Software Foundation.This program is distributed in the hope
// that it will be useful, but WITHOUT ANY WARRANTY; without even the
// implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
// PURPOSE. See the GNU Library General Public License for more
// details.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>

#include "proc/version.h"

static void die(const char *msg) NORETURN;
static void die(const char *msg)
{
     fputs(msg, stderr);
     exit(1);
}

static void version(void) NORETURN;
static void version(void)
{
     printf("pwdx (%s)\n", procps_version);
     exit(0);
}

int main(int argc, char* argv[])
{
     regex_t re;
     int i;

     if (argc < 2)
          die("Usage: pwdx pid...\n");

     // Allowed on the command line:
     //
     // --version
     // -V
     // /proc/nnnn
     // nnnn
     //
     // where nnnn is any number that doesn't begin with 0.
     //
     // If --version or -V are present, further arguments are ignored
     // completely.
        
     regcomp(&re, "^((/proc/+)?[1-9][0-9]*|-V|--version)$",
             REG_EXTENDED|REG_NOSUB);

     for (i = 1; i < argc; i++) {
          if (regexec(&re, argv[i], 0, NULL, 0) != 0) {
               char buf[27 + strlen (argv[i]) + 1];  // Constant 27 is the length of the error string "pwdx: ... "
               snprintf(buf, sizeof buf, "pwdx: invalid process id: %s\n", argv[i]);
               die(buf);
          }
          if (!strcmp("-V", argv[i]) || !strcmp("--version", argv[i]))
               version();
     }

     regfree(&re);

     int alloclen = 128;
     char *pathbuf = malloc(alloclen);

     for (i = 1; i < argc; i++) {
          char * s;
          int len;
          char buf[10 + strlen(argv[i]) + 1]; // Constant 10 is the length of strings "/proc/" + "/cwd" + 1
          
          // At this point, all arguments are in the form /proc/nnnn
          // or nnnn, so a simple check based on the first char is
          // possible
          if (argv[i][0] != '/')
               snprintf(buf, sizeof buf, "/proc/%s/cwd", argv[i]);
          else
               snprintf(buf, sizeof buf, "%s/cwd", argv[i]);

          // buf contains /proc/nnnn/cwd symlink name on entry, the
          // target of that symlink on return
          while ((len = readlink(buf, pathbuf, alloclen)) == alloclen) {
               alloclen *= 2;
               pathbuf = realloc(pathbuf, alloclen);
          }

          if (len < 0) {
               s = strerror(errno == ENOENT ? ESRCH : errno);
          } else {
               pathbuf[len] = 0;
               s = pathbuf;
          }

          printf("%s: %s\n", argv[i], s);
     }

     free(pathbuf);

     return 0;
}

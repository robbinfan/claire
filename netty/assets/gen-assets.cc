// Copyright (c) 2013 The claire-netty Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

FILE* OpenOrDie(const char* fname, const char* mode)
{
    FILE* f = fopen(fname, mode);
    if (f == NULL)
    {
        perror(fname);
        exit(-1);
    }
    return f;
}

void ConvertResourceName(const char* resource, char* buffer, size_t length)
{
    const char *name = strrchr(resource, '/');
    if (name == NULL)
    {
        name = resource;
    }
    else
    {
        name++;
    }

    assert(strlen(name)+1 < length);
    memcpy(buffer, name, strlen(name)+1);
    for (size_t i = 0;i < strlen(buffer); i++)
    {
        if (buffer[i] == '.' || buffer[i] == '-')
        {
            buffer[i] = '_';
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 3)
    {
        fprintf(stderr,
                "USAGE: %s {name} {assets... }\n\n"
                "  Creates {name}.cc {name.h} from the contents of {assets...}\n",
                argv[0]);
        return -1;
    }

    const char* name = argv[1];

    char cc_name[256];
    snprintf(cc_name, sizeof(cc_name), "%s.cc", name);

    char h_name[256];
    snprintf(h_name, sizeof(h_name), "%s.h", name);

    FILE* cc = OpenOrDie(cc_name, "w");
    fprintf(cc, "#include <stdlib.h>\n#include \"%s.h\"\n", name);

    FILE* hh = OpenOrDie(h_name, "w");
    fprintf(hh, "#pragma once\n#include <stdlib.h>\r\n");

    for (int i = 2; i < argc; ++i)
    {
        char asset_name[256];
        ConvertResourceName(argv[i], asset_name, sizeof asset_name);

        FILE* asset = OpenOrDie(argv[i], "r");
        fprintf(cc, "const char ASSET_%s[] = {\r\n", asset_name);
        fprintf(hh,
                "extern const char ASSET_%s[];\r\n"
                "extern const size_t ASSET_%s_length;\r\n",
                asset_name, asset_name);

        char buf[256];
        size_t n = 0;
        size_t line = 0;
        do
        {
            n = fread(buf, 1, sizeof(buf), asset);
            for (size_t j = 0; j < n; j++)
            {
                fprintf(cc, "0x%02x, ", (buf[j] & 0x7f));
                if (++line == 10)
                {
                    fprintf(cc, "\n");
                    line = 0;
                }
            }
        } while (n > 0);

        if (line > 0)
            fprintf(cc, "\n");
        fprintf(cc, "};\n");
        fprintf(cc, "const size_t ASSET_%s_length = sizeof(ASSET_%s);\r\n", asset_name, asset_name);

        fclose(asset);
    }
    fclose(cc);
    fclose(hh);
    return 0;
}


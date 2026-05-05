/*
 Copyright (C) 2004-2006 Grame

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU Lesser General Public License as published by
 the Free Software Foundation; either version 2.1 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#ifndef __JackSystemDeps_POSIX__
#define __JackSystemDeps_POSIX__

#if defined(__linux__) && !defined(__cplusplus)
#define _GNU_SOURCE 1
#define __USE_GNU 1
#include <stdbool.h>
#endif

#include <inttypes.h>
#include <sys/types.h>
#include <signal.h>
#include <dlfcn.h>
#ifdef __linux__
#include <stdio.h>
#include <sys/mman.h>
#include "JackError.h"
#endif

#ifndef UINT32_MAX 
#define UINT32_MAX 4294967295U
#endif

#define DRIVER_HANDLE void*
#define LoadDriverModule(name) dlopen((name), RTLD_NOW | RTLD_GLOBAL)
#define UnloadDriverModule(handle) dlclose((handle))
#define GetDriverProc(handle, name) dlsym((handle), (name))

#define JACK_HANDLE void*
#define UnloadJackModule(handle) dlclose((handle));
#define GetJackProc(handle, name) dlsym((handle), (name));

#ifdef __linux__
static inline void* LoadJackModule(const char* name)
{
    FILE* const module_file = fopen(name, "rb");
    if (module_file == NULL) {
        jack_error("Failed to open jack module file %s", name);
        return NULL;
    }

    fseek(module_file, 0, SEEK_END);
    const long size = ftell(module_file);
    fseek(module_file, 0, SEEK_SET);

    do {
        const int shm_fd = memfd_create(name, MFD_CLOEXEC);
        if (shm_fd < 0) {
            jack_error("Failed to create memory file for %s", name);
            break;
        }

        if (ftruncate(shm_fd, size) < 0) {
            jack_error("Failed to truncate memory file for %s", name);
            close(shm_fd);
            break;
        }

        char buf[8192];
        bool ok = false;
        for (long i = 0; i < size;)
        {
            const int r = fread(buf, 1, 8192, module_file);
            if (r == 0) {
                ok = true;
                break;
            }
            const int w = write(shm_fd, buf, r);
            if (r != w) {
                jack_error("Failed to write memory file for %s", name);
                break;
            }
        }

        if (! ok)
            break;

        snprintf(buf, 8192, "/proc/%d/fd/%d", getpid(), shm_fd);

        void* const handle = dlopen(buf, RTLD_NOW | RTLD_LOCAL);
        close(shm_fd);

        if (handle == NULL) {
            jack_error("Failed to load memory file for %s", name);
            break;
        }

        fclose(module_file);

        return handle;

    } while (false);

    fclose(module_file);

    return dlopen(name, RTLD_NOW | RTLD_LOCAL);
}
#else
#define LoadJackModule(name) dlopen((name), RTLD_NOW | RTLD_LOCAL);
#endif

#define JACK_DEBUG (getenv("JACK_CLIENT_DEBUG") && strcmp(getenv("JACK_CLIENT_DEBUG"), "on") == 0)

#endif

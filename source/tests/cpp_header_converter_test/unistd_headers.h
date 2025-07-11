#pragma once

#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <signal.h>
#include <poll.h>

#include <unistd.h>
#include <spawn.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <sys/sendfile.h>

//#include <errno.h>

#include <dlfcn.h>

#include <sys/wait.h>
#include <time.h>

#include <dirent.h>

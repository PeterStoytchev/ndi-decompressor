#pragma once

#define __PROFILE

#ifdef __PROFILE
#include "optik/optick.h"
#define PROFILE_FUNC(name) OPTICK_EVENT(name)
#define PROFILE_FRAME(name) OPTICK_FRAME(name)
#define PROFILE_THREAD() OPTICK_THREAD()
#else
#define PROFILE_FUNC(name)
#define PROFILE_FRAME(name)
#define PROFILE_THREAD()
#endif
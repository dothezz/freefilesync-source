#ifndef BUILDINFO_H_INCLUDED
#define BUILDINFO_H_INCLUDED

namespace Utility
{
//determine build info
//seems to be safer than checking for _WIN64 (defined on windows for 64-bit compilations only) while _WIN32 is always defined
static const bool is32BitBuild = sizeof(void*) == 4;
static const bool is64BitBuild = sizeof(void*) == 8;
}

#endif // BUILDINFO_H_INCLUDED

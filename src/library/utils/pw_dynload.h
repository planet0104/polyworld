#pragma once

// Minimal dynamic-library helpers for cppprops (Unix .so / Windows .dll).

#if defined(_WIN32) && !defined(__CYGWIN__)

#include <windows.h>

inline void *pw_dlopen(const char *path)
{
	return (void *)LoadLibraryA(path);
}

inline void *pw_dlsym(void *handle, const char *symbol)
{
	return (void *)GetProcAddress((HMODULE)handle, symbol);
}

inline const char *pw_dlerror()
{
	return "LoadLibrary/GetProcAddress failed";
}

inline int pw_dlclose(void *handle)
{
	return FreeLibrary((HMODULE)handle) ? 0 : -1;
}

#else

#include <dlfcn.h>

inline void *pw_dlopen(const char *path)
{
	return dlopen(path, RTLD_LAZY);
}

inline void *pw_dlsym(void *handle, const char *symbol)
{
	return dlsym(handle, symbol);
}

inline const char *pw_dlerror()
{
	const char *err = dlerror();
	return err ? err : "unknown dl error";
}

inline int pw_dlclose(void *handle)
{
	return dlclose(handle);
}

#endif

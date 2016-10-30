/**
 * @brief      Internal functions and defines
 */
#ifndef HENKAKU_HEADER
#define HENKAKU_HEADER

// TODO: Have this define in the SDK or something
#ifndef __VITA_KERNEL__
#define __VITA_KERNEL__
#endif

/** Logging function */
#ifdef ENABLE_LOGGING
#define LOG(fmt, ...) printf("[%s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LOG(fmt, ...)
#endif

#endif // HENKAKU_HEADER

#ifndef QILBM_DEBUG_H
#define QILBM_DEBUG_H
#pragma once

#include <cstdio>

#define LOG_MESSAGE(FILE, LEVEL, FMT, ...) \
    std::fprintf((FILE), "%s:%d: [QILBM] %s: %s: " FMT "\n", __FILE__, __LINE__, (LEVEL), __func__ __VA_OPT__(,) __VA_ARGS__)

#define LOG_MESSAGE_OBJ(FILE, LEVEL, FMT, OBJ, ...) \
    std::fprintf((FILE), "%s:%d: [QILBM] %s: %s: " FMT " ", __FILE__, __LINE__, (LEVEL), __func__ __VA_OPT__(,) __VA_ARGS__); \
    (OBJ).print((FILE)); \
    std::fputc('\n', (FILE))

#ifdef NDEBUG
    #define LOG_DEBUG(FMT, ...)
    #define LOG_DEBUG_OBJ(FMT, OBJ, ...)
#else
    #define LOG_DEBUG(FMT, ...) LOG_MESSAGE(stderr, "DEBUG", FMT, __VA_ARGS__)
    #define LOG_DEBUG_OBJ(FMT, OBJ, ...) LOG_MESSAGE_OBJ(stderr, "DEBUG", FMT, OBJ, __VA_ARGS__)
#endif

#define LOG_WARNING(FMT, ...) LOG_MESSAGE(stderr, "WARNING", FMT, __VA_ARGS__)
#define LOG_WARNING_OBJ(FMT, OBJ, ...) LOG_MESSAGE_OBJ(stderr, "WARNING", FMT, OBJ, __VA_ARGS__)

#define LOG_INFO(FMT, ...) LOG_MESSAGE(stdout, "INFO", FMT, __VA_ARGS__)
#define LOG_INFO_OBJ(FMT, OBJ, ...) LOG_MESSAGE_OBJ(stdout, "INFO", FMT, OBJ, __VA_ARGS__)

#endif

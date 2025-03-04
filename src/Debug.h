#ifndef QILBM_DEBUG_H
#define QILBM_DEBUG_H
#pragma once

#ifdef NDEBUG
    #define DEBUG_LOG(FMT, ...)
#else
    #include <cstdio>
    #define DEBUG_LOG(FMT, ...) std::fprintf(stderr, "%s:%d: [QILBM] %s: " FMT "\n", __FILE__, __LINE__, __func__ __VA_OPT__(,) __VA_ARGS__)
#endif

#endif

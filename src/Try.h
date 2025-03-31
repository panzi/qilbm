#ifndef QILBM_TRY_H
#define QILBM_TRY_H
#pragma once

#include "ILBM.h"
#include "Debug.h"

#define STR_INTERN(EXPR) #EXPR
#define STR(EXPR) STR_INTERN(EXPR)

#define TRY(RESULT) {                               \
        qilbm::Result __qilbm_result = (RESULT);    \
        if (__qilbm_result != qilbm::Result_Ok) {   \
            LOG_DEBUG("%s: %s",                     \
                qilbm::result_name(__qilbm_result), \
                STR(RESULT));                       \
            return __qilbm_result;                  \
        }                                           \
    }

#define PASS_IF(COND, RESULT, ...) {                \
        qilbm::Result __qilbm_result = (RESULT);    \
        if (__qilbm_result != qilbm::Result_Ok) {   \
            LOG_DEBUG("%s: %s",                     \
                qilbm::result_name(__qilbm_result), \
                STR(RESULT));                       \
            { __VA_ARGS__ }                         \
            if (!(COND)) {                          \
                return __qilbm_result;              \
            }                                       \
        }                                           \
    }

#define IO(OK) \
    if (!(OK)) {                            \
        LOG_DEBUG("IO Error: %s", STR(OK)); \
        return qilbm::Result_IOError;       \
    }

#endif

#ifndef QILBM_TRY_H
#define QILBM_TRY_H
#pragma once

#include "ILBM.h"

#define TRY(RESULT) {                       \
        qilbm::Result __result = (RESULT);  \
        if (__result != qilbm::Result_Ok) { \
            return __result;                \
        }                                   \
    }

#define IO(OK) if (!(OK)) { return qilbm::Result_IOError; }

#endif

#pragma once

///Use oslib
#include "TString.h"

// Use Directshow
#include "Dshow.h"
#include "streams.h"

// Using WMSDK
#include <wmsdk.h>
#include <wmsysprf.h>

//
#include "ComHelper.h"

#ifdef XIAOZAIFILTERS_EXPORTS
#define XIAOZAIFILTERS_API __declspec(dllexport)
#else
#define XIAOZAIFILTERS_API __declspec(dllimport)
#endif


/**
* Print no output.
*/
#define XZ_LOG_QUIET    -8

/**
* Something went really wrong and we will crash now.
*/
#define XZ_LOG_PANIC     0

/**
* Something went wrong and recovery is not possible.
* For example, no header was found for a format which depends
* on headers or an illegal combination of parameters is used.
*/
#define XZ_LOG_FATAL     8

/**
* Something went wrong and cannot losslessly be recovered.
* However, not all future data is affected.
*/
#define XZ_LOG_ERROR    16

/**
* Something somehow does not look correct. This may or may not
* lead to problems. An example would be the use of '-vstrict -2'.
*/
#define XZ_LOG_WARNING  24

/**
* Standard information.
*/
#define XZ_LOG_INFO     32

/**
* Detailed information.
*/
#define XZ_LOG_VERBOSE  40

/**
* Stuff which is only useful for libav* developers.
*/
#define XZ_LOG_DEBUG    48

/**
* Extremely verbose debugging, useful for libav* development.
*/
#define XZ_LOG_TRACE    56
// Include the latest possible header file( GL version header )
#if __ANDROID_API__ >= 24
#include <GLES3/gl32.h>
#elif __ANDROID_API__ >= 21

#include <GLES3/gl31.h>

#else
#include <GLES3/gl3.h>
#endif

#include "stdint.h"
#include "../common/JNILogHelper.h"
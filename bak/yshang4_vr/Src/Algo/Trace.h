/*++

Module Name:

    Trace.h

Abstract:

    Header file for the debug tracing related function defintions and macros.

Environment:

    Kernel mode

--*/
#ifndef __TRACE_H__
#define __TRACE_H__
//
// Define the tracing flags.
//
// Tracing GUID - 4df7d33c-8b7e-4291-bd88-404be89324ff
//

#define WPP_CONTROL_GUIDS                                              \
    WPP_DEFINE_CONTROL_GUID(                                           \
        IVSFHubHIDMiniTraceGuid, (4df7d33c,8b7e,4291,bd88,404be89324ff), \
                                                                            \
        WPP_DEFINE_BIT(MYDRIVER_ALL_INFO)                              \
        WPP_DEFINE_BIT(TRACE_DRIVER)                                   \
        WPP_DEFINE_BIT(TRACE_DEVICE)                                   \
        WPP_DEFINE_BIT(TRACE_QUEUE)                                    \
		WPP_DEFINE_BIT(TRACE_SENSOR_ENGINE)                            \
		WPP_DEFINE_BIT(TRACE_SENSOR)                            \
		WPP_DEFINE_BIT(TRACE_SENSOR_COLLECTION_DISCRETE)               \
		WPP_DEFINE_BIT(TRACE_ALGO)               \
		WPP_DEFINE_BIT(TRACE_DSDT)               \
        )                             

#define WPP_FLAG_LEVEL_LOGGER(flag, level)                                  \
    WPP_LEVEL_LOGGER(flag)

#define WPP_FLAG_LEVEL_ENABLED(flag, level)                                 \
    (WPP_LEVEL_ENABLED(flag) &&                                             \
     WPP_CONTROL(WPP_BIT_ ## flag).Level >= level)

#define WPP_LEVEL_FLAGS_LOGGER(lvl,flags) \
           WPP_LEVEL_LOGGER(flags)
               
#define WPP_LEVEL_FLAGS_ENABLED(lvl, flags) \
           (WPP_LEVEL_ENABLED(flags) && WPP_CONTROL(WPP_BIT_ ## flags).Level >= lvl)

//
// This comment block is scanned by the trace preprocessor to define our
// Trace function.
//
// begin_wpp config
// FUNC Trace{FLAG=MYDRIVER_ALL_INFO}(LEVEL, MSG, ...);
// FUNC TraceLog(LEVEL, FLAGS, MSG, ...);
// end_wpp
//

#endif
#define FX_SETTINGS_NAME "RichterStrip"
#define FX_SETTINGS_MATCH_NAME "BAKU89 RichterStrip"
#define FX_SETTINGS_CATEGORY "Distort"
#define FX_SETTINGS_DESCRIPTION "(c) 2020 Baku Hashimoto"

// For debugging
#ifdef DEBUG
#ifndef IS_PIPL
#include <chrono>
#include <iostream>
#endif

#define FX_LOG(log) \
    std::cout << "[" << FX_SETTINGS_NAME << "]" << log << std::endl

#define FX_DEBUG_TIME_START(name) auto name = std::chrono::system_clock::now();
#define FX_DEBUG_TIME_END(name, message)                                     \
    FX_LOG(message << " time ="                                              \
                   << std::chrono::duration_cast<std::chrono::milliseconds>( \
                          std::chrono::system_clock::now() - name)           \
                          .count()                                           \
                   << "ms");
#else
#define FX_LOG(log)
#define FX_DEBUG_TIME_START(name)
#define FX_DEBUG_TIME_END(name, message)
#endif

#ifndef BASIC_PLATFORM_REPORT_H
#define BASIC_PLATFORM_REPORT_H

namespace gs {

    static const char *log_enabled = std::getenv("GS_LOG");

    #define GS_LOG(...)                                         \
        do {                                                    \
            if (gs::log_enabled) {                              \
                fprintf(stdout, "%s:%d ", __FILE__, __LINE__);  \
                fprintf(stdout, __VA_ARGS__);                   \
                fprintf(stdout, "\n");                          \
            }                                                   \
        } while (0)

}
#endif //BASIC_PLATFORM_REPORT_H

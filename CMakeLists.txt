add_library(qbox INTERFACE)
target_include_directories(qbox INTERFACE include)


install(FILES
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/libqbox.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/mailbox.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/memory.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/router.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/uart.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/cpu-arm-cortex-a53.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/cpu-arm-cortex-m4.h"
	"${CMAKE_CURRENT_SOURCE_DIR}/include/libqbox/cpu-arm-cortex-m7.h"
	DESTINATION include/libqbox)

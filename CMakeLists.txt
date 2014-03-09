cmake_minimum_required(VERSION 2.8)

project(pixbufloader-vtf)

# ---- dependencies -----------------------------------------------------------
find_package(PkgConfig REQUIRED)

pkg_check_modules(PIXBUF REQUIRED gdk-pixbuf-2.0)
pkg_check_modules(VTFLIB REQUIRED VTFLib)

execute_process(COMMAND
	"${PKG_CONFIG_EXECUTABLE}" gdk-pixbuf-2.0 --variable=gdk_pixbuf_moduledir
	OUTPUT_VARIABLE PIXBUF_MODULEDIR OUTPUT_STRIP_TRAILING_WHITESPACE)

# ---- compiler flags ---------------------------------------------------------
if(MSVC)
	# Force to always compile with W4
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wextra -pedantic -Werror -Wno-write-strings -O3")
endif()

include_directories(
	${PIXBUF_INCLUDE_DIRS}
	${VTFLIB_INCLUDE_DIRS})

add_definitions(-DGDK_PIXBUF_ENABLE_BACKEND)

if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	add_definitions(-D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE)
endif()

# ---- build library ----------------------------------------------------------
add_library(pixbufloader-vtf SHARED pixbufloader-vtf.cpp)
target_link_libraries(pixbufloader-vtf ${PIXBUF_LIBRARIES} ${VTFLIB_LIBRARIES})

# ---- install target ---------------------------------------------------------
install(TARGETS pixbufloader-vtf DESTINATION "${PIXBUF_MODULEDIR}")

find_program(GDK_PIXBUF_QUERY_LOADERS "gdk-pixbuf-query-loaders")

if("${GDK_PIXBUF_QUERY_LOADERS}" STREQUAL "")
	if(${CMAKE_SYSTEM_PROCESSOR} STREQUAL "x86_64")
		find_program(GDK_PIXBUF_QUERY_LOADERS "gdk-pixbuf-query-loaders-64")
	else()
		find_program(GDK_PIXBUF_QUERY_LOADERS "gdk-pixbuf-query-loaders-32")
	endif()
endif()

if("${GDK_PIXBUF_QUERY_LOADERS}" STREQUAL "")
	message(WARNING "Could not find gdk-pixbuf-query-loaders binary.
The pixbuf loader registry will not be automatically updated on install.")
else()
	install(CODE "execute_process(COMMAND \"${GDK_PIXBUF_QUERY_LOADERS}\" --update-cache)")
endif()

# ---- uninstall target -------------------------------------------------------
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
	"${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
	IMMEDIATE @ONLY)

add_custom_target(uninstall
	COMMAND ${CMAKE_COMMAND} -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

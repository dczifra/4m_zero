cmake_minimum_required(VERSION 3.15)
project(4m_zero)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
if (${CMAKE_BUILD_TYPE} STREQUAL "release")
add_compile_options(-Wall -Wcast-qual -pedantic -Wextra -Wshadow -m64 -DUSE_PTHREADS -DIS_64BIT -msse -DUSE_SSE2 -msse2 -flto=auto)
add_link_options(-Wall -Wcast-qual -fno-exceptions -pedantic -Wextra -Wshadow -m64 -DUSE_PTHREADS -DIS_64BIT -msse -DUSE_SSE2 -msse2 -flto=auto)
add_link_options(-Wl,--no-as-needed)
else()
add_compile_options(-Wall -Wcast-qual -pedantic -Wextra -Wshadow -m64 -DUSE_PTHREADS -DIS_64BIT -msse -DUSE_SSE2 -msse2 -g)
add_link_options(-m64 -Wl,--no-as-needed)
endif()
add_subdirectory(source)
file(COPY ${PROJECT_SOURCE_DIR}/data DESTINATION ${PROJECT_BINARY_DIR})
add_subdirectory(data)

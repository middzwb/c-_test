set(test_srcs
    reorder.cc
    black_magic.cc
    test.cc)
add_executable(test ${test_srcs})

find_package(Threads)
target_link_libraries(test
    ${CMAKE_THREAD_LIBS_INIT})

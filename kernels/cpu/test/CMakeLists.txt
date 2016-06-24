FIND_PACKAGE(Threads REQUIRED)

LIST(APPEND READDY_CPU_TEST_SOURCES CPUTestMain.cpp)
LIST(APPEND READDY_CPU_TEST_SOURCES CPUTestKernel.cpp)

ADD_EXECUTABLE(runUnitTests_cpu ${READDY_CPU_TEST_SOURCES} ${GOOGLETEST_INCLUDE} ${ReaDDy_INCLUDE_DIRS} ${CPU_INCLUDE_DIR})
TARGET_INCLUDE_DIRECTORIES(runUnitTests_cpu PUBLIC ${ReaDDy_INCLUDE_DIRS} ${CPU_INCLUDE_DIR} ${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
TARGET_LINK_LIBRARIES(runUnitTests_cpu ReaDDy ReaDDy_kernel_cpu gtest gtest_main ${CMAKE_THREAD_LIBS_INIT})

ADD_TEST(NAME readdy-tests-cpu COMMAND runUnitTests_cpu)

IF (READDY_INSTALL_UNIT_TEST_EXECUTABLE)
    INSTALL(TARGETS runUnitTests_cpu
            RUNTIME DESTINATION bin
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib)
ENDIF (READDY_INSTALL_UNIT_TEST_EXECUTABLE)
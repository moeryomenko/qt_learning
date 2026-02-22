find_package(Qt6 REQUIRED COMPONENTS Test Core)

# -----------------------------------------------------------------------
# Helper macro: create a single test executable and register with CTest
# -----------------------------------------------------------------------
macro(add_qt_test TARGET SOURCE)
  qt_add_executable(${TARGET} ${SOURCE})
  target_link_libraries(${TARGET} PRIVATE Qt6::Test Qt6::Core)
  add_test(NAME ${TARGET} COMMAND ${TARGET})
endmacro()

# -----------------------------------------------------------------------
# Individual test executables
# -----------------------------------------------------------------------
add_qt_test(tst_apiclient      tst_apiclient.cc)
add_qt_test(tst_signalingclient tst_signalingclient.cc)
add_qt_test(tst_icecoding       tst_icecoding.cc)

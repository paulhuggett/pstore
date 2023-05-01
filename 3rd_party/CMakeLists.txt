if (PSTORE_IS_INSIDE_LLVM)
    # compiling inside LLVM. We don't add the googletest library
    # because LLVM provides a copy for us.
else ()
  if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/googletest/CMakeLists.txt")
    # Tell gtest to link against the "Multi-threaded Debug DLL runtime
    # library" on Windows.
    set (gtest_force_shared_crt On CACHE BOOL "Always use msvcrt.dll")
    # We don't want to install either gtest or gmock.
    set (INSTALL_GTEST Off CACHE BOOL "Disable gtest install")
    set (INSTALL_GMOCK Off CACHE BOOL "Disable gmock install")
    add_subdirectory (googletest)
    foreach (target gtest gmock gmock_main gtest_main)
      set_target_properties (${target} PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED Yes
        CXX_EXTENSIONS No
      )
      set (gclang_options -Wno-implicit-int-float-conversion)
      target_compile_definitions (${target} PUBLIC GTEST_REMOVE_LEGACY_TEST_CASEAPI_=1)
      target_compile_options (${target} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${gclang_options}>
      )
      target_link_options (${target} PRIVATE
        $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:${gclang_options}>
      )
    endforeach()
  endif ()
endif (PSTORE_IS_INSIDE_LLVM)

add_subdirectory (peejay)

#eof 3rd_party/CMakeLists.txt

if (PSTORE_NPM_EXECUTABLE)
  add_custom_target (pstore-broker-ui-build
    COMMAND npm update
    COMMAND npm run package
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMENT "Packaging pstore-broker-ui"
  )
endif ()

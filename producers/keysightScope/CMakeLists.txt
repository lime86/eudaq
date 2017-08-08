INCLUDE_DIRECTORIES( include )

add_executable(KeysightScopeControl src/KeysightScopeControl.cxx src/KeysightScopeController.cc)
add_executable(KeysightScopeProducer src/KeysightScopeProducer.cc src/KeysightScopeController.cc)

target_link_libraries(KeysightScopeControl EUDAQ ${EUDAQ_THREADS_LIB})
target_link_libraries(KeysightScopeProducer EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS KeysightScopeControl
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

INCLUDE_DIRECTORIES( include )

add_executable(OscilloscopeControl src/OscilloscopeControl.cxx src/KeysightScopeController.cc)
add_executable(KeysightScopeOnTrigger src/KeysightScopeOnTrigger.cxx src/KeysightScopeController.cc)
add_executable(OscilloscopeProducer src/OscilloscopeProducer.cc src/KeysightScopeController.cc)

target_link_libraries(OscilloscopeControl EUDAQ ${EUDAQ_THREADS_LIB})
target_link_libraries(KeysightScopeOnTrigger EUDAQ ${EUDAQ_THREADS_LIB})
target_link_libraries(OscilloscopeProducer EUDAQ ${EUDAQ_THREADS_LIB})

INSTALL(TARGETS KeysightScopeOnTrigger OscilloscopeControl OscilloscopeProducer
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)

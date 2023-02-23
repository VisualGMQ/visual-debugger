set(VISUAL_DEBUGGER_SRC
    main.cpp net.cpp)
add_executable(debugger ${VISUAL_DEBUGGER_SRC})
target_link_libraries(debugger PRIVATE ${LOWLEVEL_NAME} nuklear tinyfiledialogs)
CopyDLL(debugger)

add_executable(test_client test_data_send_client.cpp)
add_executable(data_reader data_reader.cpp)
target_compile_features(test_client PRIVATE cxx_std_14)
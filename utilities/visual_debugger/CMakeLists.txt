aux_source_directory(./ VISUAL_DEBUGGER_SRC)
add_executable(debugger ${VISUAL_DEBUGGER_SRC})
target_link_libraries(debugger PRIVATE ${LOWLEVEL_NAME} nuklear tinyfiledialogs)
CopyDLL(debugger)
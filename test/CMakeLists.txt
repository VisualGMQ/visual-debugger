add_executable(utility_test ./utility_test.cpp)
target_link_libraries(utility_test PRIVATE dbglib)
add_test(NAME utility_test COMMAND $<TARGET_FILE:utility_test>)

add_executable(packet_test ./packet_test.cpp)
target_link_libraries(packet_test PRIVATE dbglib)
add_test(NAME packet_test COMMAND $<TARGET_FILE:packet_test>)

add_executable(client ./client.cpp)
target_link_libraries(client PRIVATE dbglib)
add_test(NAME client COMMAND $<TARGET_FILE:client>)
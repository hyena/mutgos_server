add_executable(mutgos_server mutgos_server_main.cpp)

target_link_libraries(mutgos_server mutgos_utilities)
target_link_libraries(mutgos_server mutgos_dbinterface)
target_link_libraries(mutgos_server mutgos_executor)
target_link_libraries(mutgos_server mutgos_events)
target_link_libraries(mutgos_server mutgos_comminterface)
target_link_libraries(mutgos_server mutgos_security)
target_link_libraries(mutgos_server mutgos_primitives)
target_link_libraries(mutgos_server mutgos_softcode)
target_link_libraries(mutgos_server mutgos_angelscriptinterface)
target_link_libraries(mutgos_server mutgos_useragent)
target_link_libraries(mutgos_server mutgos_logging)

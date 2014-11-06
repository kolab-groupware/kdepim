add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )
add_definitions( -DQT_NO_CAST_FROM_BYTEARRAY )

macro_optional_add_subdirectory(sendlateragent)
macro_optional_add_subdirectory(archivemailagent)
macro_optional_add_subdirectory(mailfilteragent)
macro_optional_add_subdirectory(notesagent)
#comment until 4.13
#add_subdirectory(followupreminderagent)
install(FILES folderarchiveagent.desktop DESTINATION "${CMAKE_INSTALL_PREFIX}/share/akonadi/agents")

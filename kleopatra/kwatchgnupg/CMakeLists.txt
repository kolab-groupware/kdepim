set(kwatchgnupg_SRCS
  ../utils/kdlogtextwidget.cpp
  ../utils/gnupg-helper.cpp
  ../utils/hex.cpp
  kwatchgnupgmainwin.cpp
  kwatchgnupgconfig.cpp
  aboutdata.cpp
  tray.cpp
  main.cpp
)

if(WIN32)
  set(kwatchgnupg_SRCS ${kwatchgnupg_SRCS} ../utils/gnupg-registry.c)
endif()

include_directories(..)

kde4_add_executable(kwatchgnupg ${kwatchgnupg_SRCS})

add_definitions(-DKDE_DEFAULT_DEBUG_AREA=5153)

target_link_libraries(kwatchgnupg
  ${KDE4_KDEUI_LIBS}
  kleo
  ${QGPGME_LIBRARIES}
)

install(TARGETS kwatchgnupg ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES kwatchgnupg.png kwatchgnupg2.png DESTINATION ${DATA_INSTALL_DIR}/kwatchgnupg/pics)

install(FILES kwatchgnupgui.rc DESTINATION ${DATA_INSTALL_DIR}/kwatchgnupg)

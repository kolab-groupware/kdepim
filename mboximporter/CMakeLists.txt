project(mboximporter)

add_definitions( -DQT_NO_CAST_FROM_ASCII )
add_definitions( -DQT_NO_CAST_TO_ASCII )
set(mboximporter_SRCS
  main.cpp
  mboxmainwindow.cpp
  mboximporterinfogui.cpp
  mboximportwidget.cpp
  mboximportkernel.cpp
)

kde4_add_ui_files(mboximporter_SRCS 
                  ui/mboximportwidget.ui
)

include_directories(${Boost_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/mailimporter ${CMAKE_SOURCE_DIR}/mailcommon ${CMAKE_SOURCE_DIR}/messagecomposer)
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

kde4_add_executable(mboximporter ${mboximporter_SRCS})

target_link_libraries(mboximporter
  ${KDE4_KIO_LIBS}
  ${KDEPIMLIBS_AKONADI_LIBS}
  ${KDEPIMLIBS_KPIMIDENTITIES_LIBS}
  mailimporter
  mailcommon
)

install(TARGETS mboximporter  ${INSTALL_TARGETS_DEFAULT_ARGS} )

install(PROGRAMS mboximporter.desktop DESTINATION ${XDG_APPS_INSTALL_DIR})

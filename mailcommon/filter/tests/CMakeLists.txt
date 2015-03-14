set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(
  ../
)

macro( add_filter_test _source )
  set( _test ${_source} ../filter${_source} ${ARGN})
  get_filename_component( _name ${_source} NAME_WE )
  kde4_add_unit_test( filter${_name}test TESTNAME filter-${_name} ${_test} )
  target_link_libraries( filter${_name}test  mailcommon kdepim ${KDEPIMLIBS_KMIME_LIBS} ${QT_QTTEST_LIBRARY} ${QT_QTGUI_LIBRARY} ${QT_QTCORE_LIBRARY} ${KDE4_KDEUI_LIBS} ${QT_QTXML_LIBRARY} ${KDEPIMLIBS_AKONADI_LIBS} ${KDEPIMLIBS_KPIMIDENTITIES_LIBS})
endmacro()

add_filter_test(actionpipethrough.cpp ../filteractionwithcommand.cpp ../filteractionwithurl.cpp)
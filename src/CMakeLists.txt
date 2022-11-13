qt_add_executable(application main.cc)

qt_add_qml_module(application
  URI main
  VERSION 1.0
  QML_FILES main.qml)

target_compile_definitions(
  application
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(application PRIVATE Qt6::Gui Qt6::Quick)

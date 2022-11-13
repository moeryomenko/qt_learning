add_executable(application)

target_sources(application PRIVATE main.cc)

target_compile_definitions(
  application
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_link_libraries(application PRIVATE Qt6::Core Qt6::Qml Qt6::Quick
                                          Qt6::QuickControls2)

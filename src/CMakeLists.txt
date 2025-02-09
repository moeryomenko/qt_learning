qt_add_executable(application main.cc)

add_subdirectory(ui)

target_compile_definitions(
    application
    PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>
)

target_link_libraries(
    application
    PRIVATE ui Qt6::Gui Qt6::Quick Qt6::QuickControls2
)

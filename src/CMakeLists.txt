qt_add_executable(application
  main.cc
  ApiClient.hh          ApiClient.cc
  SignalingClient.hh    SignalingClient.cc
  PortalScreencast.hh   PortalScreencast.cc
  PipeWireNodeModel.hh  PipeWireNodeModel.cc
  VideoFrameSink.hh     VideoFrameSink.cc
  VideoTile.hh          VideoTile.cc
  WebRtcPeerManager.hh  WebRtcPeerManager.cc
)

add_subdirectory(ui)

if(BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

target_compile_definitions(
  application
  PRIVATE $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:QT_QML_DEBUG>)

target_include_directories(application PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${GST_INCLUDE_DIRS}
  ${PW_INCLUDE_DIRS}
)

target_link_directories(application PRIVATE
  ${GST_LIBRARY_DIRS}
  ${PW_LIBRARY_DIRS}
)

target_link_libraries(application PRIVATE
  ui
  Qt6::Gui
  Qt6::Quick
  Qt6::Svg
  Qt6::Network
  Qt6::WebSockets
  Qt6::DBus
  Qt6::Multimedia
  ${GST_LIBRARIES}
  ${PW_LIBRARIES}
)

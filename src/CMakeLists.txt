qt_add_executable(application
  main.cc
  ApiClient.h          ApiClient.cpp
  SignalingClient.h    SignalingClient.cpp
  PortalScreencast.h   PortalScreencast.cpp
  PipeWireNodeModel.h  PipeWireNodeModel.cpp
  VideoFrameSink.h     VideoFrameSink.cpp
  VideoTile.h          VideoTile.cpp
  WebRtcPeerManager.h  WebRtcPeerManager.cpp
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

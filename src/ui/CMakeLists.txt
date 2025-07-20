qt_add_resources(ui_icons icons.qrc)

qt_add_qml_module(ui
  URI pages
  VERSION 1.0
  QML_FILES 
    main.qml
    ServerSidebar.qml
    ServerIcon.qml
    ChannelSidebar.qml
    ChannelCategory.qml
    ChannelItem.qml
    ChatArea.qml
    MessageBubble.qml
    MemberSidebar.qml
    MemberSection.qml
    MemberItem.qml
    IconButton.qml
    MobileNavigation.qml
    MobileNavButton.qml
    SvgIcon.qml)

target_sources(ui PRIVATE ${ui_icons})

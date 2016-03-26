# install default config files to /etc/xdg
install(FILES
    lxqt.conf	
    session.conf
    windowmanagers.conf
    globalkeyshortcuts.conf
    DESTINATION "${LXQT_ETC_XDG_DIR}/lxqt"
    COMPONENT Runtime
)
install(DIRECTORY openbox
    DESTINATION "${LXQT_ETC_XDG_DIR}/lxqt"
    COMPONENT Runtime
)

configure_file(pcmanfm-qt/lxqt/settings.conf.in pcmanfm-qt/lxqt/settings.conf @ONLY)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/pcmanfm-qt/lxqt/settings.conf"
    DESTINATION "${LXQT_ETC_XDG_DIR}/pcmanfm-qt/lxqt"
    COMPONENT Runtime
)

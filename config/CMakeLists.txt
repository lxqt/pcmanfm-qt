# install default config files to /etc/xdg
install(DIRECTORY lxsession/LXDE-Qt    DESTINATION ${LXQT_ETC_XDG_DIR}/lxsession/)
install(DIRECTORY openbox    DESTINATION ${LXQT_SHARE_DIR}/)
install(DIRECTORY pcmanfm-qt/lxqt    DESTINATION ${LXQT_ETC_XDG_DIR}/pcmanfm-qt/)

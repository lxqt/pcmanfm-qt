# install default config files to /etc/xdg
install(DIRECTORY lxsession/LXDE-Qt    DESTINATION ${LXQT_ETC_XDG_DIR}/lxsession/)
# install(DIRECTORY lxsession/openbox    DESTINATION ${LXQT_ETC_XDG_DIR}/lxsession/)
install(DIRECTORY pcmanfm-qt/lxqt    DESTINATION ${LXQT_ETC_XDG_DIR}/pcmanfm-qt/)

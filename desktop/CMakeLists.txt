

########### install files ###############

install( PROGRAMS org.kde.konsole-plus.desktop  DESTINATION ${KDE_INSTALL_APPDIR} )
install( FILES org.kde.konsole-plus.appdata.xml DESTINATION ${KDE_INSTALL_METAINFODIR} )
install(FILES org.kde.konsole-plus.desktop DESTINATION ${KDE_INSTALL_DATADIR}/kglobalaccel)

install(FILES konsole-plus.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
install( PROGRAMS konsole-plus-run.desktop  DESTINATION ${KDE_INSTALL_DATADIR}/kio/servicemenus )

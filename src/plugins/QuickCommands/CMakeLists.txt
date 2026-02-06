
ki18n_wrap_ui(EXTRA_QUICKCOMMANDSPLUGIN_SRCS qcwidget.ui)

kcoreaddons_add_plugin(konsole_plus_quickcommandsplugin
SOURCES
    quickcommandsplugin.cpp
    quickcommandswidget.cpp
    quickcommandsmodel.cpp
    filtermodel.cpp
    ${EXTRA_QUICKCOMMANDSPLUGIN_SRCS}
INSTALL_NAMESPACE
    "konsole-plus-plugins"
)

configure_file(konsole_quickcommands.in.json konsole_quickcommands.json)
target_link_libraries(konsole_plus_quickcommandsplugin
    konsoleprivate
    konsoleapp
)

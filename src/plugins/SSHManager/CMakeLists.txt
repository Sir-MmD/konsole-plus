
find_package(OpenSSL REQUIRED)

ki18n_wrap_ui(extra_sshplugin_SRCS sshwidget.ui)

ecm_qt_declare_logging_category(
    extra_sshplugin_SRCS
    HEADER sshmanagerplugindebug.h
    IDENTIFIER SshManagerPluginDebug
    CATEGORY_NAME org.kde.konsole-plus.plugin.sshmanager
    DESCRIPTION "Konsole Plus plugin (SSH Manager)"
    DEFAULT_SEVERITY Warning
    EXPORT KONSOLE
)

kcoreaddons_add_plugin(konsole_plus_sshmanagerplugin
SOURCES
    sshmanagerplugin.cpp
    sshmanagerpluginwidget.cpp
    sshmanagermodel.cpp
    sshmanagerfiltermodel.cpp
    sshtreeview.cpp
    sshcryptohelper.cpp
    ${extra_sshplugin_SRCS}
INSTALL_NAMESPACE
    "konsole-plus-plugins"
)

configure_file(konsole_sshmanager.in.json konsole_sshmanager.json)
target_link_libraries(konsole_plus_sshmanagerplugin
    konsoleprivate
    konsoleapp
    OpenSSL::Crypto
)

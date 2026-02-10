<p align="center">
  <img src="data/icons/sc-apps-konsole-plus.svg" alt="Konsole Plus" width="128" />
</p>

<h1 align="center">Konsole Plus</h1>

<p align="center">
  A fork of <a href="https://konsole.kde.org">KDE Konsole</a> with an enhanced SSH Manager.
  <br />
  Installs alongside the original Konsole without conflicts.
</p>

---

## Features

- **Encrypted credential storage** -- stored passwords and passphrases are encrypted with a master password
- **Remote filesystem mounting** -- mount remote filesystems automatically on connect via rclone
- **Profile import / export** -- back up and share SSH profiles as JSON with optional encryption and duplicate handling
- **SOCKS5 proxy support** -- connect through a proxy per profile
- **Clean connection output** -- hides the SSH command, shows connection status only
- **Automatic tab naming** -- tabs are named after the SSH profile
- **Multi-select operations** -- bulk-connect or bulk-delete profiles
- **Duplicate Session** -- right-click a tab to open another connection to the same SSH host
- **Reconnect Session** -- right-click a tab to reconnect a disconnected SSH session
- **Quick Connect** -- type `user@host` or `user@host:port` in the SSH Manager panel to connect without a saved profile
- **SSH error reasons** -- on connection failure, shows specific error details (wrong password, timeout, refused, etc.)
- **Tab SSH status indicators** -- colored circles on tabs show connection state (gray/orange/green/red)
- **Custom tab icon and color** -- set per-profile icons and colors in the SSH profile editor, or right-click any tab to customize
- **Lock Tab** -- right-click a tab to lock it and prevent accidental closing
- **Compose Bar** -- bottom panel to type and send commands to the current or all sessions at once
- **Tab Manager** -- side panel with a tree view of all open tabs
- **Double-click to duplicate** -- double-click an SSH tab to duplicate it, or double-click a local tab to open a new one
- **Open SFTP** -- right-click an SSH tab to open the remote filesystem in Dolphin via SFTP, using the saved SSH credentials automatically

## Build

Requires Qt 6.5+ and KDE Frameworks 6. Install dependencies for your distro, then build and install.

<details>
<summary>Arch Linux</summary>

```bash
sudo pacman -S --needed base-devel cmake extra-cmake-modules \
  qt6-base qt6-multimedia \
  kbookmarks kconfig kconfigwidgets kcoreaddons \
  kcrash kdbusaddons kglobalaccel kguiaddons \
  ki18n kiconthemes kio knewstuff \
  knotifications knotifyconfig kparts kpty \
  kservice ktextwidgets kwidgetsaddons \
  kwindowsystem kxmlgui kdoctools \
  icu zlib openssl libxkbcommon \
  openssh sshpass rclone fuse3
```
</details>

<details>
<summary>Fedora</summary>

```bash
sudo dnf install cmake gcc-c++ extra-cmake-modules \
  qt6-qtbase-devel qt6-qtmultimedia-devel \
  kf6-kbookmarks-devel kf6-kconfig-devel kf6-kconfigwidgets-devel \
  kf6-kcoreaddons-devel kf6-kcrash-devel kf6-kdbusaddons-devel \
  kf6-kglobalaccel-devel kf6-kguiaddons-devel kf6-ki18n-devel \
  kf6-kiconthemes-devel kf6-kio-devel kf6-knewstuff-devel \
  kf6-knotifications-devel kf6-knotifyconfig-devel kf6-kparts-devel \
  kf6-kpty-devel kf6-kservice-devel kf6-ktextwidgets-devel \
  kf6-kwidgetsaddons-devel kf6-kwindowsystem-devel kf6-kxmlgui-devel \
  kf6-kdoctools-devel \
  libicu-devel zlib-devel openssl-devel libxkbcommon-devel \
  openssh-clients sshpass rclone fuse3
```
</details>

<details>
<summary>Debian 13+ (Trixie) / Ubuntu 25.04+</summary>

KF6 packages are **not available** in Debian 12 (Bookworm) or Ubuntu 24.04 (Noble).

```bash
sudo apt install cmake g++ extra-cmake-modules \
  qt6-base-dev qt6-multimedia-dev \
  libkf6bookmarks-dev libkf6config-dev libkf6configwidgets-dev \
  libkf6coreaddons-dev libkf6crash-dev libkf6dbusaddons-dev \
  libkf6globalaccel-dev libkf6guiaddons-dev libkf6i18n-dev \
  libkf6iconthemes-dev libkf6kio-dev libkf6newstuff-dev \
  libkf6notifications-dev libkf6notifyconfig-dev libkf6parts-dev \
  libkf6pty-dev libkf6service-dev libkf6textwidgets-dev \
  libkf6widgetsaddons-dev libkf6windowsystem-dev libkf6xmlgui-dev \
  libkf6doctools-dev \
  libicu-dev zlib1g-dev libssl-dev libxkbcommon-dev \
  openssh-client sshpass rclone fuse3
```
</details>

```bash
git clone https://github.com/Sir-MmD/konsole-plus.git
cd konsole-plus
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel $(nproc)
sudo cmake --install build
```

## Uninstall

```bash
sudo xargs rm -f < build/install_manifest.txt
```

## License

GPL-2.0-or-later (same as upstream [KDE Konsole](https://invent.kde.org/utilities/konsole))

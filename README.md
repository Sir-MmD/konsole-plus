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

- **Clean SSH connections** -- commands are hidden from the terminal, showing only a "Connecting to ..." message with green OK / red FAILED status
- **Smart tab naming** -- tabs are automatically named after the SSH profile
- **Safe reconnection** -- opening a profile while already connected spawns a new tab
- **Remote filesystem mounting** -- automatically mount remote filesystems via rclone when connecting, with mount deduplication across tabs
- **SSH key passphrase support** -- dedicated field for encrypted private keys
- **Password encryption** -- AES-256-GCM encryption for all stored credentials with a master password (PBKDF2, 100k iterations)
- **Import / Export** -- save and load SSH profiles as JSON, optionally encrypted
- **Multi-select** -- Ctrl+click / Shift+click to bulk-connect or bulk-delete profiles

## Build

Install dependencies for your distro, then build and install.

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
<summary>Debian / Ubuntu</summary>

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

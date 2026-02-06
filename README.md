# Konsole Plus

A fork of [KDE Konsole](https://konsole.kde.org) with an enhanced SSH Manager plugin.

## What's New

This fork improves the built-in SSH Manager plugin with the following features:

### SSH Connection Improvements
- **Clean connection experience** -- SSH commands (sshpass, etc.) are hidden from the terminal; only a "Connecting to ..." message is shown
- **Connection status** -- Green **OK** on successful connection, red **FAILED** on failure
- **Smart tab naming** -- Tabs are automatically named after the SSH profile identifier or hostname
- **Safe reconnection** -- Clicking an SSH profile while already connected opens a new tab instead of crashing
- **SSHFS deduplication** -- Multiple tabs to the same host share a single rclone/SSHFS mount with automatic cleanup

### SSH Key Passphrase Support
- New "Key Passphrase" field in the SSH profile editor for encrypted private keys

### Password Encryption at Rest
- Encrypt stored passwords, key passphrases, and proxy passwords using **AES-256-GCM**
- Master password with **PBKDF2-HMAC-SHA256** key derivation (100,000 iterations)
- Lazy unlock -- master password is only prompted when you actually perform an operation (connect, edit, export)
- Backwards compatible with existing plaintext configurations

### Import / Export
- Export all SSH profiles to a **JSON** file
- Optionally encrypt the export with a separate passphrase
- Import profiles from JSON files (encrypted or plain) and merge into the current configuration

### Multi-Select
- **Ctrl+click** and **Shift+click** to select multiple SSH profiles
- Bulk delete selected entries
- Double-click with multiple selection connects to all selected profiles

## Runtime Dependencies

The SSH Manager plugin requires these tools to be installed on your system:

| Tool | Required | Purpose |
|------|----------|---------|
| `openssh` | Yes | SSH client (`ssh` command) |
| `sshpass` | For password auth | Provides passwords to SSH non-interactively |
| `rclone` | For SSHFS | Remote filesystem mounting via SFTP |
| `fuse2` / `fuse3` | For SSHFS | FUSE support for rclone mount |

### Arch Linux

```bash
sudo pacman -S openssh sshpass rclone fuse2
```

### Fedora / RHEL / CentOS

```bash
sudo dnf install openssh-clients sshpass rclone fuse
```

### Debian / Ubuntu

```bash
sudo apt install openssh-client sshpass rclone fuse3
```

## Building from Source

### Step 1: Install build dependencies

Pick your distro and run the command to install everything needed for compilation.

<details>
<summary>Arch Linux</summary>

```bash
sudo pacman -S base-devel cmake extra-cmake-modules qt6-base qt6-multimedia \
  kf6-ki18n kf6-kconfig kf6-kconfigwidgets kf6-kcoreaddons kf6-kcrash \
  kf6-kguiaddons kf6-kiconthemes kf6-kio kf6-knewstuff kf6-knotifications \
  kf6-knotifyconfig kf6-kparts kf6-kservice kf6-ktextwidgets \
  kf6-kwidgetsaddons kf6-kwindowsystem kf6-kxmlgui kf6-kbookmarks \
  kf6-kpty kf6-kdbusaddons kf6-kglobalaccel kf6-kdoctools \
  openssl icu
```

</details>

<details>
<summary>Fedora</summary>

```bash
sudo dnf install cmake extra-cmake-modules gcc-c++ \
  qt6-qtbase-devel qt6-qtmultimedia-devel \
  kf6-ki18n-devel kf6-kconfig-devel kf6-kconfigwidgets-devel \
  kf6-kcoreaddons-devel kf6-kcrash-devel kf6-kguiaddons-devel \
  kf6-kiconthemes-devel kf6-kio-devel kf6-knewstuff-devel \
  kf6-knotifications-devel kf6-knotifyconfig-devel kf6-kparts-devel \
  kf6-kservice-devel kf6-ktextwidgets-devel kf6-kwidgetsaddons-devel \
  kf6-kwindowsystem-devel kf6-kxmlgui-devel kf6-kbookmarks-devel \
  kf6-kpty-devel kf6-kdbusaddons-devel kf6-kglobalaccel-devel \
  kf6-kdoctools-devel openssl-devel libicu-devel
```

</details>

<details>
<summary>Debian / Ubuntu</summary>

```bash
sudo apt install cmake extra-cmake-modules g++ gettext \
  qt6-base-dev qt6-multimedia-dev \
  libkf6i18n-dev libkf6config-dev libkf6configwidgets-dev \
  libkf6coreaddons-dev libkf6crash-dev libkf6guiaddons-dev \
  libkf6iconthemes-dev libkf6kio-dev libkf6newstuff-dev \
  libkf6notifications-dev libkf6notifyconfig-dev libkf6parts-dev \
  libkf6service-dev libkf6textwidgets-dev libkf6widgetsaddons-dev \
  libkf6windowsystem-dev libkf6xmlgui-dev libkf6bookmarks-dev \
  libkf6pty-dev libkf6dbusaddons-dev libkf6globalaccel-dev \
  libkf6doctools-dev libssl-dev libicu-dev
```

</details>

### Step 2: Clone the repository

```bash
git clone https://github.com/Sir-MmD/konsole-plus.git
cd konsole-plus
```

### Step 3: Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr
cmake --build build --parallel $(nproc)
```

### Step 4: Install

```bash
sudo cmake --install build
```

After installation, you can run `konsole-plus` from your application launcher or terminal.

## Upstream

This fork is based on [KDE Konsole](https://invent.kde.org/utilities/konsole). All original Konsole features, documentation, and keyboard/color scheme data remain unchanged.

## License

GPL-2.0-or-later (same as upstream Konsole)

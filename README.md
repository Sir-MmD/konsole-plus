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

## Building

Standard KDE/CMake build:

```bash
mkdir build && cd build
cmake .. -DCMAKE_INSTALL_PREFIX=/usr
make -j$(nproc)
sudo make install
```

Requires: Qt 6.5+, KDE Frameworks 6, OpenSSL 3.0+

## Upstream

This fork is based on [KDE Konsole](https://invent.kde.org/utilities/konsole). All original Konsole features, documentation, and keyboard/color scheme data remain unchanged.

## License

GPL-2.0-or-later (same as upstream Konsole)

# mOS platform architecture

## Portable device OS

The primary mOS target remains the XIAO ESP32-S3-class device with its round display and SX1262 LoRa hardware. This is a microcontroller target: it runs firmware and a compact native UI, not a desktop Linux distribution.

Keep the device OS small, responsive, and independent of removable storage. Core services, settings, and essential Mission Link functions stay on internal flash. Optional TF storage is an add-on for user files, exports, and larger logs.

## Static nodes

mOS Static is the headless/low-resource profile for fixed mesh nodes. It shares protocol and service logic with Portable while omitting the watch UI and optional interactive apps where appropriate.

## Optional Linux desktop companion

A future Linux application may provide a desktop control center for configuring, monitoring, and managing nearby mOS devices. It would be a companion, not the operating system running on the ESP32-S3.

KDE Plasma is the preferred desktop experience for that optional Linux companion. The app should integrate naturally with Plasma (desktop launcher, system tray/status area where appropriate, notifications, and a coherent Qt/Kirigami interface) while remaining usable on other Linux desktops. It must not require Plasma libraries for its core protocol or device-management logic.

Potential companion features:
- Device and mesh overview, including node/contact names and link health.
- Configuration editor with validation before sending settings to a device.
- Message and event history with explicit privacy controls.
- Firmware/version information and update workflow.
- Export/import of contacts and configuration backups.

## Separation rules

- Keep the embedded firmware build independent of Linux, Qt, and KDE dependencies.
- Share protocol definitions and test vectors where practical; do not share platform-specific UI code.
- Treat the desktop companion as a separate future target with its own build and release pipeline.
- Do not assume a Linux host is always present; every device must remain independently usable.
- Radio transmission remains disabled until the hardware, regional rules, and RF behaviour are verified.

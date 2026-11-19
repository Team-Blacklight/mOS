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

## mOS Link desktop companion

The proposed USB framing, pairing, display, and input contract is documented in [MISSION_LINK_USB_PROTOCOL.md](MISSION_LINK_USB_PROTOCOL.md). It is a draft until the board USB configuration and cryptographic implementation are selected and validated.

mOS Link is the desktop-side companion and USB bridge for Mission Link-compatible XIAO devices. It is distinct from the embedded mOS firmware and from the optional general-purpose Linux device-management UI described above; the projects may share libraries and UI components where that is sensible.

### First-connect and trust flow

1. The host detects a connected, supported Mission Link device using a documented USB interface and identifies its model and protocol capabilities.
2. The host displays a clear trust prompt, for example: "Do you trust this XIAO?" The prompt identifies the device and explains the requested permissions.
3. If the user approves, the host displays a progress state such as **“Downloading mOS companion app”** and downloads the official mOS Link installer/package from the configured official distribution source. Show download progress and clear failure/retry feedback. USB enumeration alone must never trigger a download; the user must approve the trust prompt first.
4. When the download completes, hand off to the normal host-operating-system installer or package manager. Do not silently execute the installer or bypass OS security prompts; the user confirms installation through the standard OS flow.
5. The companion performs an authenticated pairing handshake with the device. Trust is stored per device and can be revoked from either side.
6. Screen mirroring and input forwarding are separate, explicit session permissions. Users can end the session at any time.

### Display and input bridge

- **Display mirroring:** mOS Link receives display frames or UI updates from the XIAO over the USB transport and renders them in a resizable desktop window. The embedded round display remains functional and authoritative; mirroring does not require the device to become a USB display adapter.
- **Keyboard and mouse forwarding:** while the user has an active, focused mOS Link session and has enabled input forwarding, keyboard and pointer events are translated into the device's documented input-event format and delivered to the mOS UI.
- Input forwarding must not act as a general-purpose host keystroke logger. Capture only events intended for the focused companion window, stop forwarding immediately when the session loses focus or is disconnected, and provide a visible enabled/disabled indicator.
- The device UI should show when a desktop session is connected and when remote input is enabled. A device-side confirmation may be required for first-time pairing or elevated actions.
- Do not expose arbitrary host files, shell access, or unrestricted USB/HID injection as a side effect of approving screen mirroring.

### Pairing and transport security

- Use a versioned, documented USB protocol with capability negotiation; do not trust a device based only on its USB product name or serial string.
- Pairing must authenticate the device and host, resist replay, and establish fresh session keys. Use a reviewed standard cryptographic handshake and authenticated encryption rather than inventing a protocol.
- Bind stored trust to a device identity backed by a cryptographic key, not a user-editable display name.
- Make permissions least-privilege and independently revocable: device identification, display stream, input forwarding, and any future file transfer are distinct capabilities.
- Fail closed on malformed frames, unsupported protocol versions, failed authentication, or interrupted handshakes. Do not silently downgrade to plaintext.
- Provide a clear "Forget device" action that removes the host's stored trust record. Device-side reset/revocation must also invalidate prior pairings.

### Platform and packaging

- Target Windows and Linux first only when their USB and packaging paths are supportable; keep the transport and protocol libraries platform-neutral.
- On Linux, provide a Qt/Kirigami interface that feels native on KDE Plasma but remains usable on other desktop environments.
- Use normal signed/repository-based distribution and operating-system installation prompts. The device may advertise the official download location, but it must not pretend to be a trusted installer or bypass host security controls.
- Keep the companion optional: the XIAO must remain independently usable without a desktop connection.

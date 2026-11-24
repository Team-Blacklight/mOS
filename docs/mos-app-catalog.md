# mOS starting app catalog

This is the initial launcher catalog. These are product requirements, not yet implemented UI apps.

## Messages

**Status:** core app, planned.

- Show conversations grouped by saved contact or raw Mission Link ID.
- Compose and receive text over Mission Link.
- Offer Bluetooth as an alternate transport when a compatible peer/service is available; do not imply Bluetooth automatically bridges to the LoRa mesh.
- Show queued, sent, delivered, and failed as distinct states.
- Keep offline/queued messages visible and make capacity limits understandable.
- Never label a message delivered solely because it was handed to a radio.

## Settings

**Status:** core app, planned.

- Device preferences such as time/date, display, sound, and haptics where supported by selected hardware.
- Connectivity status and supported connection configuration.
- Device/firmware information and diagnostics.
- Expose only settings that the detected hardware and firmware actually support.
- Do not present unverified radio pins or unsupported radio controls as working.

## Contacts

**Status:** core app, planned.

- Store a display name and the person's Mission Link human-readable ID.
- Keep that ID distinct from the internal numeric radio node address.
- Allow opening a message conversation from a contact.
- Define a persistence strategy before promising contacts survive reset.
- Avoid assuming the phone-number-shaped `(777)-XXX-XXXX` ID is a real telephone number or can be called through the cellular network.

## Card Stuff Yes (CSY)

**Status:** planned pre-installed application; implementation not started.

- Include CSY in the default mOS firmware/app image and show it in the launcher.
- Build a native, display-sized interface for the selected XIAO ESP32-S3 screen; do not embed or depend on a desktop browser.
- Connect to the existing CSY account and game services over Wi-Fi when available.
- Keep device-specific credentials secure; use an explicit device-pairing/authentication flow rather than embedding shared account secrets in firmware.
- Support the CSY features that fit the device's memory, controls, screen, and network constraints. Treat collection browsing and lightweight account/game views as initial candidates; do not promise full desktop feature parity.
- Show clear offline, signed-out, connecting, and service-error states. Do not represent cached data as live.
- Respect server-side authorization and catalogue validation; the device is an untrusted client.
- Enforce minimum supported CSY API/client versions through a verified update path. If a mandatory update cannot be verified, block CSY access and offer retry or exit.
- Keep CSY separate from Mission Link; CSY networking must not silently use or interfere with mesh messaging.
- Do not enable remote app installation or arbitrary code execution as part of the initial built-in app.

## App Store

**Status:** visible placeholder; inactive.

- May appear in the launcher from the beginning.
- Opening it should clearly explain that the store is not available yet.
- Installation, downloads, updates, and package execution stay disabled until an app lifecycle/API, package format, signature verification, permissions, and rollback/recovery strategy exist.
- No pretend download buttons or false success states.

## Launcher and shared rules

- Initial entries: Messages, Settings, Contacts, App Store, and the planned pre-installed CSY app.
- The inactive App Store should be visually distinguished but not hidden.
- Use clear focus/selection states suitable for the final display and input hardware; exact layout waits until the screen and controls are selected.
- Core Mission Link messaging should not require a paired phone.
- Bluetooth is an optional connection path, not a requirement for the watch's basic operation.

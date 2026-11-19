# Mission Link USB Companion Protocol

Status: proposed protocol contract; not yet implemented or hardware-validated.

This document defines the USB data channel used by mOS Link to communicate with a Mission Link-compatible XIAO. It covers device discovery, authenticated pairing, display mirroring, and scoped keyboard/mouse input forwarding.

The USB protocol is separate from the LoRa mesh packet format. USB messages must never be forwarded to the radio automatically.

## Design goals

- Work over a documented USB data interface supported by the selected XIAO firmware and desktop operating systems.
- Keep framing and message parsing platform-neutral.
- Negotiate protocol versions and capabilities before enabling features.
- Authenticate both ends before sending private display data or accepting remote input.
- Keep display mirroring and input forwarding independently permissioned.
- Fail closed on malformed, oversized, unauthenticated, or unsupported messages.
- Remain optional: disconnecting the desktop must not interrupt normal device operation.

## Transport

The initial implementation should use a dedicated USB CDC data interface, subject to validation against the final board USB configuration and host-driver behaviour. Do not assume that a USB product name, serial string, or VID/PID alone proves device identity.

The companion must distinguish the mOS Link data interface from any serial console, firmware flashing interface, or unrelated USB interface. The final interface descriptors and identifiers must be assigned and documented before implementation.

USB enumeration is discovery only. It does not authorize pairing, trigger a download, or enable input forwarding.

## Frame envelope

All multi-byte integers are little-endian. A transport frame has this fixed 16-byte header followed by its payload:

| Offset | Size | Field | Meaning |
|---:|---:|---|---|
| 0 | 4 | Magic | ASCII `MLNK` |
| 4 | 1 | Major | Protocol major version |
| 5 | 1 | Minor | Protocol minor version |
| 6 | 2 | Type | Message type |
| 8 | 2 | Flags | Reserved; send zero unless defined |
| 10 | 2 | Payload length | Number of payload bytes |
| 12 | 4 | Sequence | Per-direction sequence number |

The initial maximum payload is 4096 bytes. Implementations must reject larger lengths before allocating or copying payload data. The receiver must handle partial reads, multiple frames in one read, and frames split across reads. Unknown message types may be ignored only when the negotiated protocol explicitly marks them optional; otherwise terminate the session safely.

The header is framing metadata, not an authentication mechanism. Integrity and confidentiality are provided by the authenticated session described below. Do not treat a checksum or CRC as a substitute for authentication.

## Message types

Initial proposed type assignments:

| Type | Name | Direction | Purpose |
|---:|---|---|---|
| 0x0001 | HELLO | Both | Exchange protocol ranges and basic capabilities |
| 0x0002 | HELLO_ACK | Both | Select compatible protocol version |
| 0x0010 | DEVICE_INFO | Device to host | Model, firmware version, random session identifier, public-key fingerprint |
| 0x0011 | CAPABILITIES | Both | Advertise supported optional features and limits |
| 0x0020 | PAIR_INIT | Both | Begin authenticated pairing handshake |
| 0x0021 | PAIR_REPLY | Both | Continue authenticated pairing handshake |
| 0x0022 | PAIR_CONFIRM | Both | Confirm the authenticated pairing transcript |
| 0x0023 | PAIR_REVOKE | Both | Revoke the current trust relationship |
| 0x0030 | PERMISSION_REQUEST | Either | Request a specific session capability |
| 0x0031 | PERMISSION_RESULT | Either | Grant or deny a requested capability |
| 0x0040 | DISPLAY_CONFIG | Device to host | Display dimensions, orientation, pixel format, frame limits |
| 0x0041 | DISPLAY_FRAME | Device to host | Display frame or bounded frame update |
| 0x0042 | DISPLAY_CONTROL | Host to device | Request pause/resume or a supported display mode |
| 0x0050 | INPUT_EVENT | Host to device | Focus-scoped keyboard or pointer event |
| 0x0051 | INPUT_STATE | Both | Report whether remote input is enabled |
| 0x0060 | PING | Both | Keepalive and round-trip measurement |
| 0x0061 | PONG | Both | Keepalive response |
| 0x00F0 | ERROR | Both | Structured protocol error |

These values are reserved for the initial protocol draft. Implementations must not silently reinterpret a known type.

## Version and capability negotiation

1. Both sides send HELLO with supported major/minor ranges and a bounded capability list.
2. The peers select the highest mutually supported version within the same major version.
3. If there is no compatible major version, stop and show an actionable compatibility error.
4. Exchange DEVICE_INFO and CAPABILITIES.
5. Authenticate and pair before enabling private display or input capabilities.

Capabilities are explicit identifiers, not free-form executable commands. Unknown optional capabilities are ignored; unknown required capabilities cause negotiation to fail safely.

## Pairing and session security

Use a reviewed, maintained implementation of a standard authenticated key exchange; do not design custom cryptography. The implementation choice must be reviewed before coding. The preferred direction is a mutually authenticated Noise pattern with static device identity keys and ephemeral session keys, using a supported standard cipher suite and transcript authentication.

Requirements:

- The device and host authenticate cryptographic identities, not display names or USB descriptors.
- First-time pairing requires a user-visible approval on the host and, where practical, confirmation on the device.
- Display a short authentication fingerprint or comparison code so users can detect a wrong-device pairing.
- Derive fresh session keys for every connection. Never reuse session traffic keys across reconnects.
- Protect every post-handshake frame with authenticated encryption and enforce monotonic per-direction sequence numbers to reject replay and reordering.
- Persist only the minimum trust material required. Protect private keys using the strongest supported platform storage; document limitations of the selected hardware and firmware configuration.
- Revocation on either side invalidates the stored trust relationship. A factory reset must not preserve old pairing authorization.
- Authentication failure, transcript mismatch, sequence failure, or unsupported security parameters terminates the session. There is no plaintext fallback.

The exact Noise pattern, cipher suite, key encoding, provisioning flow, and library version are implementation gates and must be recorded here before interoperability tests are written.

## Display mirroring

The XIAO remains the authoritative device UI. The companion receives a stream of display frames or bounded updates and renders them in a desktop window.

DISPLAY_CONFIG defines dimensions, orientation, pixel format, and maximum frame size. DISPLAY_FRAME carries a frame identifier, update metadata, and bounded image data. The initial implementation should prefer simple full-frame updates before adding dirty rectangles or compression.

The receiver must validate dimensions, arithmetic, offsets, and payload sizes before allocating image buffers. Apply backpressure: if the host cannot keep up, the device may drop obsolete intermediate frames and send the newest complete frame. Never let a slow display consumer block Mission Link core services or the device's local UI.

The display stream requires its own granted permission. Closing the window or revoking permission stops display transmission.

## Keyboard and mouse forwarding

Input forwarding is an application-level mOS input channel. It is not unrestricted USB HID injection and must not turn the XIAO into a remote-control bridge for the host computer.

- Forward only events captured by the focused mOS Link window.
- Require a separate explicit permission for input forwarding.
- Show a persistent, visible enabled indicator on both host and device.
- Stop forwarding immediately when the window loses focus, the session disconnects, or permission is revoked.
- Send a release-all/reset-input-state event on focus loss and disconnect so the device cannot retain stuck keys or buttons.
- Use a small, documented set of key usages and pointer/button events; reject unknown values and out-of-range coordinates.
- Do not capture global shortcuts, background keystrokes, clipboard contents, or input from other applications.
- The device accepts remote input only while its local policy allows it and the authenticated session has the input capability.

INPUT_EVENT should include an event kind, a bounded usage/button code, pointer coordinates where relevant, and a monotonically increasing event sequence. Coordinate mapping must use the active DISPLAY_CONFIG dimensions and reject out-of-range values.

## Errors and resource limits

- Enforce maximum frame, message, and queue sizes at every layer.
- Avoid unbounded dynamic allocation on the embedded target.
- Rate-limit malformed-message error responses to avoid loops.
- Close or reset a session after repeated framing, authentication, or sequence violations.
- Keep USB bridge processing from starving display refresh, battery management, or Mission Link scheduling.
- Log protocol errors without recording private display contents or key values.

## Download and installation flow

The trust prompt and download UX are described in [PLATFORM_ARCHITECTURE.md](PLATFORM_ARCHITECTURE.md). USB enumeration alone must not download or execute software. After explicit approval, the host may display "Downloading mOS companion app" with progress and fetch the official package. Installation still proceeds through the normal operating-system installer or package manager with user confirmation.

## Implementation sequence

1. Confirm the exact XIAO board revision, USB interface configuration, and host support matrix.
2. Select and review the cryptographic handshake library and identity-key storage strategy.
3. Implement host-testable frame encode/decode with fragmentation, concatenation, invalid-length, and version tests.
4. Implement HELLO, capability negotiation, and DEVICE_INFO without enabling privileged capabilities.
5. Implement authenticated pairing and revocation with negative tests.
6. Implement display configuration and full-frame mirroring with strict resource limits.
7. Implement focused-window input forwarding behind a separate permission and device-side indicator.
8. Add cross-platform companion packaging and end-to-end tests.

No radio transmit behaviour is defined by this USB protocol. The USB companion must not enable or bypass the existing radio-transmit safety gate.

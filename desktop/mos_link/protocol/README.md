# mOS Link protocol core

This directory contains the first platform-neutral host-side implementation of the Mission Link USB frame envelope.

## Current scope

- C++17 library with no Qt, OS, USB, or ESP-IDF dependency.
- Encodes and incrementally decodes the documented 16-byte MLNK header.
- Handles partial reads and multiple frames in one input chunk.
- Rejects bad magic, non-zero reserved flags, null input with non-zero length, and payloads larger than the protocol's 4096-byte limit.
- Uses caller-owned output storage; the decoder retains at most one incomplete frame between calls.

## Explicitly not implemented

This is framing only, not a secure session. It does not authenticate peers, encrypt payloads, validate message-specific schemas, negotiate versions, or open USB devices. Do not use it to accept untrusted remote input or expose private display data. Those features require the reviewed authenticated handshake and permission model in docs/MISSION_LINK_USB_PROTOCOL.md.

The USB CDC interface remains unassigned pending physical board and descriptor validation. This library must not be interpreted as proof that USB companion communication works on the XIAO hardware.

## Build and test

From the repository root:

    cmake -S desktop/mos_link/protocol -B build/mos-link-protocol
    cmake --build build/mos-link-protocol
    ctest --test-dir build/mos-link-protocol --output-on-failure

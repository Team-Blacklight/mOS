#include "system_app.h"
#include "mos_layout.h"
#include "mos_gesture.h"
#include <stdio.h>

namespace mos {
void SystemApp::render(Canvas& c, uint32_t) {
  drawHeader(c, "System");
  const DeviceInfo* d = sys().device;
  char buf[64];
  int y = headerH() + 1;
  auto line = [&](const char* s) { c.drawText(0, y, s); y += FONT_H; };
  if (d) {
    snprintf(buf, sizeof(buf), "Radio %s", d->radioState); line(buf);
    snprintf(buf, sizeof(buf), "TX %s  %s", d->rfEnabled ? "ON" : "OFF", d->preset); line(buf);
    if (d->receiving) snprintf(buf, sizeof(buf), "RSSI %d SNR %d", d->rssi, d->snr);
    else snprintf(buf, sizeof(buf), "RSSI --");
    line(buf);
    if (d->batteryPercent >= 0) snprintf(buf, sizeof(buf), "Batt %d%%", d->batteryPercent);
    else snprintf(buf, sizeof(buf), "Batt --");
    line(buf);
    snprintf(buf, sizeof(buf), "%s", d->nodeIdHex); line(buf);
  }
  const StorageService* storage = sys().storage;
  if (!storage) {
    line("TF storage N/A");
    return;
  }
  const VolumeInfo& info = storage->info();
  switch (info.state) {
    case VolumeState::Unchecked: line("TF: not checked"); break;
    case VolumeState::Unavailable: line("TF: no card"); break;
    case VolumeState::Ready: line("TF: ready"); break;
    case VolumeState::ReadOnly: line("TF: read only"); break;
    case VolumeState::IoError: line("TF: I/O error"); break;
  }
  if (info.present()) {
    const uint64_t freeKiB = info.freeBytes / 1024;
    const uint64_t totalKiB = info.capacityBytes / 1024;
    snprintf(buf, sizeof(buf), "Free %llu/%llu KiB",
             static_cast<unsigned long long>(freeKiB),
             static_cast<unsigned long long>(totalKiB));
    line(buf);
  }
}

bool SystemApp::onGesture(const GestureEvent& ev, uint32_t nowMs) {
  if (ev.gesture != Gesture::Tap) return false;

  // The TF status line follows the five device rows rendered when device
  // information is available. With no device data, the TF line is first.
  const int16_t rowY = static_cast<int16_t>(
      headerH() + 1 + (sys().device ? 5 * FONT_H : 0));
  if (ev.y < rowY || ev.y >= rowY + FONT_H) return false;

  INotifier* notifier = sys().notifier;
  StorageService* storage = sys().storage;
  if (!storage) {
    if (notifier) notifier->notify("TF storage N/A", nowMs);
    return true;
  }

  // A tap on an unavailable TF row retries detection, so reinsertion does
  // not require a reboot. A mounted card uses the same row for safe eject.
  if (!storage->available()) {
    const VolumeInfo& refreshed = storage->refresh();
    if (notifier) {
      const char* message = "No TF card found";
      if (refreshed.state == VolumeState::Ready) message = "TF card ready";
      else if (refreshed.state == VolumeState::ReadOnly) message = "TF read only";
      else if (refreshed.state == VolumeState::IoError) message = "TF check failed";
      notifier->notify(message, nowMs);
    }
    return true;
  }

  const bool ejected = storage->eject();
  if (notifier) {
    notifier->notify(ejected ? "TF safe to remove" : "TF eject failed", nowMs);
  }
  return true;
}
}  // namespace mos

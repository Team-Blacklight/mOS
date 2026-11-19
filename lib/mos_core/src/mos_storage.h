#pragma once
// Optional removable storage contract for mOS Portable.
//
// The system must boot and run built-in apps without removable storage. A board
// adapter implements IRemovableVolume only after its pinout and filesystem are
// verified. This interface deliberately contains no GPIO or Arduino dependencies.
#include <stddef.h>
#include <stdint.h>

namespace mos {
enum class VolumeState : uint8_t {
  Unchecked,       // board adapter has not attempted detection yet
  Unavailable,     // no card, unsupported filesystem, or failed mount
  Ready,           // mounted and writable
  ReadOnly,        // mounted but writes are not permitted
  IoError          // an operation failed; adapter should stop unsafe retries
};

// Bounded directory entry returned by adapters; names are single path components.
struct StorageEntry {
  char name[32] = {};
  bool directory = false;
  uint32_t sizeBytes = 0;
};

struct VolumeInfo {
  VolumeState state = VolumeState::Unchecked;
  uint64_t capacityBytes = 0;
  uint64_t freeBytes = 0;
  bool present() const { return state == VolumeState::Ready || state == VolumeState::ReadOnly; }
  bool writable() const { return state == VolumeState::Ready; }
};

class IRemovableVolume {
 public:
  virtual ~IRemovableVolume() {}
  // Probe and mount if possible. Must not format the card automatically.
  virtual VolumeInfo probe() = 0;
  // Read/write operations are relative to the mounted volume root.
  virtual bool read(const char* path, uint8_t* out, size_t capacity, size_t& bytesRead) = 0;
  virtual bool write(const char* path, const uint8_t* data, size_t length) = 0;
  virtual bool remove(const char* path) = 0;
  // List one directory. An empty path means the volume root. Implementations
  // must return at most capacity entries and never truncate an entry name.
  virtual bool list(const char*, StorageEntry*, size_t, size_t&) { return false; }
  // Flush pending filesystem data and unmount before the card is physically
  // removed. Adapters that cannot guarantee this must leave the default false.
  virtual bool eject() { return false; }
};

// Thin policy wrapper shared by the shell and apps. A null adapter is valid and
// represents hardware without removable storage (including mOS Static).
class StorageService {
 public:
  explicit StorageService(IRemovableVolume* volume = nullptr) : volume_(volume) {}

  VolumeInfo refresh() {
    if (!volume_) {
      info_.state = VolumeState::Unavailable;
      info_.capacityBytes = info_.freeBytes = 0;
      return info_;
    }
    info_ = volume_->probe();
    return info_;
  }

  const VolumeInfo& info() const { return info_; }
  bool available() const { return info_.present(); }

  // Safe removal is explicit: the adapter must flush and unmount successfully
  // before the service reports the volume as unavailable.
  bool eject() {
    if (!volume_ || !info_.present() || !volume_->eject()) {
      if (volume_ && info_.present()) info_.state = VolumeState::IoError;
      return false;
    }
    info_.state = VolumeState::Unavailable;
    info_.capacityBytes = info_.freeBytes = 0;
    return true;
  }

  bool read(const char* path, uint8_t* out, size_t capacity, size_t& bytesRead) {
    bytesRead = 0;
    if (!volume_ || !info_.present() || !safePath(path) || !out || capacity == 0) return false;
    if (!volume_->read(path, out, capacity, bytesRead) || bytesRead > capacity) {
      info_.state = VolumeState::IoError;
      return false;
    }
    return true;
  }

  bool write(const char* path, const uint8_t* data, size_t length) {
    if (!volume_ || !info_.writable() || !safePath(path) || (!data && length > 0)) return false;
    if (!volume_->write(path, data, length)) {
      info_.state = VolumeState::IoError;
      return false;
    }
    return true;
  }

  bool remove(const char* path) {
    if (!volume_ || !info_.writable() || !safePath(path)) return false;
    if (!volume_->remove(path)) {
      info_.state = VolumeState::IoError;
      return false;
    }
    return true;
  }

  // Directory reads are allowed on read-only cards. Pass "" to list the root.
  // A full output buffer is not an error: adapters may return the first
  // capacity entries, allowing callers to page through large directories later.
  bool list(const char* path, StorageEntry* entries, size_t capacity, size_t& count) {
    count = 0;
    if (!volume_ || !info_.present() || !path ||
        (*path != '\\0' && !safePath(path)) ||
        (capacity > 0 && !entries)) return false;
    if (!volume_->list(path, entries, capacity, count) || count > capacity) {
      count = 0;
      info_.state = VolumeState::IoError;
      return false;
    }
    return true;
  }

 private:
  // Only relative paths within the volume are accepted. Adapters must still
  // use a filesystem API safely; this prevents common absolute/traversal forms.
  static bool safePath(const char* path) {
    if (!path || !*path || path[0] == '/' || path[0] == '\\') return false;
    const char* component = path;
    for (const char* p = path;; ++p) {
      const unsigned char ch = static_cast<unsigned char>(*p);
      if (ch == ':' || (ch < 0x20 && ch != '\0')) return false;
      if (ch == '/' || ch == '\\' || ch == '\0') {
        const size_t len = static_cast<size_t>(p - component);
        if (len == 0 || (len == 1 && component[0] == '.') ||
            (len == 2 && component[0] == '.' && component[1] == '.')) return false;
        if (ch == '\0') break;
        component = p + 1;
      }
    }
    return true;
  }

  IRemovableVolume* volume_ = nullptr;
  VolumeInfo info_{};
};
}  // namespace mos

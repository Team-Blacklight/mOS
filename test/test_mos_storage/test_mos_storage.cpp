#include <unity.h>
#include <stdio.h>
#include "mos_storage.h"

class FakeVolume : public mos::IRemovableVolume {
 public:
  mos::VolumeInfo next{mos::VolumeState::Ready, 1024, 512};
  bool readOk = true, writeOk = true, removeOk = true, ejectOk = true;
  size_t lastWriteLength = 0;
  int readCalls = 0, writeCalls = 0, removeCalls = 0, ejectCalls = 0, listCalls = 0;
  bool listOk = true;
  bool reportOversizedRead = false;
  mos::VolumeInfo probe() override { return next; }
  bool read(const char*, uint8_t*, size_t, size_t& bytesRead) override {
    ++readCalls;
    if (!readOk) return false;
    bytesRead = reportOversizedRead ? 99 : 1;
    return true;
  }
  bool write(const char*, const uint8_t*, size_t length) override {
    ++writeCalls;
    lastWriteLength = length;
    return writeOk;
  }
  bool remove(const char*) override { ++removeCalls; return removeOk; }
  bool list(const char*, mos::StorageEntry* entries, size_t capacity, size_t& count) override {
    ++listCalls;
    if (!listOk) return false;
    count = capacity > 0 ? 1 : 0;
    if (count) {
      snprintf(entries[0].name, sizeof(entries[0].name), "notes.txt");
      entries[0].directory = false;
      entries[0].sizeBytes = 123;
    }
    return true;
  }
  bool eject() override { ++ejectCalls; return ejectOk; }
};

void test_missing_adapter_is_safe() {
  mos::StorageService storage;
  storage.refresh();
  TEST_ASSERT_FALSE(storage.available());
  uint8_t out[4] = {}; size_t n = 99;
  TEST_ASSERT_FALSE(storage.read("file.bin", out, sizeof(out), n));
  TEST_ASSERT_EQUAL_UINT32(0, n);
  TEST_ASSERT_FALSE(storage.write("file.bin", out, sizeof(out)));
  TEST_ASSERT_FALSE(storage.eject());
}

void test_ready_volume_allows_io() {
  FakeVolume fake;
  mos::StorageService storage(&fake);
  storage.refresh();
  TEST_ASSERT_TRUE(storage.available());
  uint8_t out[4] = {}; size_t n = 0;
  TEST_ASSERT_TRUE(storage.read("file.bin", out, sizeof(out), n));
  TEST_ASSERT_EQUAL_UINT32(1, n);
  TEST_ASSERT_TRUE(storage.write("file.bin", out, sizeof(out)));
  TEST_ASSERT_EQUAL_UINT32(sizeof(out), fake.lastWriteLength);
}

void test_read_only_volume_refuses_mutation() {
  FakeVolume fake;
  fake.next.state = mos::VolumeState::ReadOnly;
  fake.ejectOk = false;
  mos::StorageService storage(&fake);
  storage.refresh();
  uint8_t data = 7;
  TEST_ASSERT_TRUE(storage.available());
  TEST_ASSERT_FALSE(storage.write("file.bin", &data, 1));
  TEST_ASSERT_FALSE(storage.remove("file.bin"));
  TEST_ASSERT_FALSE(storage.eject());
  TEST_ASSERT_EQUAL((int)mos::VolumeState::IoError, (int)storage.info().state);
}

void test_paths_must_stay_inside_volume() {
  FakeVolume fake;
  mos::StorageService storage(&fake);
  storage.refresh();
  uint8_t data = 7; size_t n = 0;
  TEST_ASSERT_FALSE(storage.read("../secret", &data, 1, n));
  TEST_ASSERT_FALSE(storage.read("/absolute", &data, 1, n));
  TEST_ASSERT_FALSE(storage.write("folder/../../secret", &data, 1));
  TEST_ASSERT_FALSE(storage.remove("C:/secret"));
  TEST_ASSERT_EQUAL_INT(0, fake.readCalls);
  TEST_ASSERT_EQUAL_INT(0, fake.writeCalls);
  TEST_ASSERT_EQUAL_INT(0, fake.removeCalls);
}

void test_directory_listing_supports_root_and_read_only() {
  FakeVolume fake;
  fake.next.state = mos::VolumeState::ReadOnly;
  mos::StorageService storage(&fake);
  storage.refresh();
  mos::StorageEntry entries[2];
  size_t count = 99;
  TEST_ASSERT_TRUE(storage.list("", entries, 2, count));
  TEST_ASSERT_EQUAL_UINT32(1, count);
  TEST_ASSERT_EQUAL_STRING("notes.txt", entries[0].name);
  TEST_ASSERT_FALSE(entries[0].directory);
  TEST_ASSERT_EQUAL_UINT32(123, entries[0].sizeBytes);
  TEST_ASSERT_EQUAL_INT(1, fake.listCalls);
}

void test_directory_listing_rejects_unsafe_paths_and_bad_counts() {
  FakeVolume fake;
  mos::StorageService storage(&fake);
  storage.refresh();
  mos::StorageEntry entries[1];
  size_t count = 0;
  TEST_ASSERT_FALSE(storage.list("../outside", entries, 1, count));
  TEST_ASSERT_EQUAL_INT(0, fake.listCalls);

  fake.listOk = false;
  TEST_ASSERT_FALSE(storage.list("folder", entries, 1, count));
  TEST_ASSERT_EQUAL((int)mos::VolumeState::IoError, (int)storage.info().state);
  TEST_ASSERT_EQUAL_UINT32(0, count);
}

void test_directory_listing_rejects_adapter_overflow() {
  class OverflowVolume : public FakeVolume {
   public:
    bool list(const char*, mos::StorageEntry*, size_t, size_t& count) override {
      count = 2;
      return true;
    }
  } fake;
  mos::StorageService storage(&fake);
  storage.refresh();
  mos::StorageEntry entry[1];
  size_t count = 0;
  TEST_ASSERT_FALSE(storage.list("", entry, 1, count));
  TEST_ASSERT_EQUAL_UINT32(0, count);
  TEST_ASSERT_EQUAL((int)mos::VolumeState::IoError, (int)storage.info().state);
}

void test_oversized_read_result_is_io_error() {
  FakeVolume fake;
  fake.reportOversizedRead = true;
  mos::StorageService storage(&fake);
  storage.refresh();
  uint8_t data = 7; size_t n = 0;
  TEST_ASSERT_FALSE(storage.read("file.bin", &data, 1, n));
  TEST_ASSERT_EQUAL((int)mos::VolumeState::IoError, (int)storage.info().state);
}

void test_io_failure_marks_volume_unhealthy() {
  FakeVolume fake;
  fake.writeOk = false;
  mos::StorageService storage(&fake);
  storage.refresh();
  uint8_t data = 7;
  TEST_ASSERT_FALSE(storage.write("file.bin", &data, 1));
  TEST_ASSERT_EQUAL((int)mos::VolumeState::IoError, (int)storage.info().state);
  TEST_ASSERT_FALSE(storage.available());
}

void test_eject_flushes_and_marks_volume_unavailable() {
  FakeVolume fake;
  mos::StorageService storage(&fake);
  storage.refresh();
  TEST_ASSERT_TRUE(storage.available());
  TEST_ASSERT_TRUE(storage.eject());
  TEST_ASSERT_EQUAL_INT(1, fake.ejectCalls);
  TEST_ASSERT_FALSE(storage.available());
  TEST_ASSERT_EQUAL((int)mos::VolumeState::Unavailable, (int)storage.info().state);
  TEST_ASSERT_EQUAL_UINT32(0, storage.info().capacityBytes);
  uint8_t data = 7;
  TEST_ASSERT_FALSE(storage.write("after-eject.bin", &data, 1));
  TEST_ASSERT_EQUAL_INT(0, fake.writeCalls);
}

void test_failed_eject_blocks_further_io() {
  FakeVolume fake;
  fake.ejectOk = false;
  mos::StorageService storage(&fake);
  storage.refresh();
  TEST_ASSERT_FALSE(storage.eject());
  TEST_ASSERT_EQUAL((int)mos::VolumeState::IoError, (int)storage.info().state);
  uint8_t data = 7;
  TEST_ASSERT_FALSE(storage.write("still-mounted.bin", &data, 1));
  TEST_ASSERT_EQUAL_INT(0, fake.writeCalls);
}


void test_reprobe_handles_removal_and_reinsertion() {
  FakeVolume fake;
  mos::StorageService storage(&fake);
  storage.refresh();
  TEST_ASSERT_TRUE(storage.available());

  // Adapter reports no card after physical removal.
  fake.next.state = mos::VolumeState::Unavailable;
  storage.refresh();
  TEST_ASSERT_FALSE(storage.available());
  uint8_t data = 7;
  TEST_ASSERT_FALSE(storage.write("offline.bin", &data, 1));

  // A later probe after reinsertion may mount the card again.
  fake.next = {mos::VolumeState::Ready, 2048, 1024};
  storage.refresh();
  TEST_ASSERT_TRUE(storage.available());
  TEST_ASSERT_EQUAL_UINT32(2048, storage.info().capacityBytes);
  TEST_ASSERT_TRUE(storage.write("online.bin", &data, 1));
}

void test_read_failure_marks_volume_unhealthy() {
  FakeVolume fake;
  fake.readOk = false;
  mos::StorageService storage(&fake);
  storage.refresh();
  uint8_t data = 0; size_t bytesRead = 0;
  TEST_ASSERT_FALSE(storage.read("broken.bin", &data, 1, bytesRead));
  TEST_ASSERT_EQUAL_UINT32(0, bytesRead);
  TEST_ASSERT_EQUAL((int)mos::VolumeState::IoError, (int)storage.info().state);
  TEST_ASSERT_FALSE(storage.available());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_missing_adapter_is_safe);
  RUN_TEST(test_ready_volume_allows_io);
  RUN_TEST(test_read_only_volume_refuses_mutation);
  RUN_TEST(test_paths_must_stay_inside_volume);
  RUN_TEST(test_directory_listing_supports_root_and_read_only);
  RUN_TEST(test_directory_listing_rejects_unsafe_paths_and_bad_counts);
  RUN_TEST(test_directory_listing_rejects_adapter_overflow);
  RUN_TEST(test_oversized_read_result_is_io_error);
  RUN_TEST(test_io_failure_marks_volume_unhealthy);
  RUN_TEST(test_eject_flushes_and_marks_volume_unavailable);
  RUN_TEST(test_failed_eject_blocks_further_io);
  RUN_TEST(test_reprobe_handles_removal_and_reinsertion);
  RUN_TEST(test_read_failure_marks_volume_unhealthy);
  return UNITY_END();
}

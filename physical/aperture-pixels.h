/* SPDX-License-Identifier: MIT */
// Diagnostic only: clone the driver's existing CPU aperture read-only.
// No physical mapping, register write, allocation ioctl or GGTT modification.
#ifndef RENKU_APERTURE_PIXELS_H
#define RENKU_APERTURE_PIXELS_H
#include <OS.h>
#include <intel_extreme.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// Standalone single-threaded diagnostic only. A fault while reading the
// existing global block mapping returns normally so the caller can release its
// lock.
static sigjmp_buf sApertureReadJump;
static volatile sig_atomic_t sApertureReadActive = 0;
static volatile sig_atomic_t sApertureGuardInUse = 0;
static void ApertureReadFault(int signal) {
  if (sApertureReadActive) {
    sApertureReadActive = 0;
    siglongjmp(sApertureReadJump, 1);
  }
  _exit(128 + signal);
}
class ApertureReadGuard {
public:
  ApertureReadGuard() : fReady(false) {
    if (sApertureGuardInUse)
      return;
    struct sigaction action = {};
    action.sa_handler = ApertureReadFault;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGSEGV, &action, &fOldSegv) != 0)
      return;
    if (sigaction(SIGBUS, &action, &fOldBus) != 0) {
      sigaction(SIGSEGV, &fOldSegv, NULL);
      return;
    }
    sApertureGuardInUse = 1;
    fReady = true;
  }
  ~ApertureReadGuard() {
    if (!fReady)
      return;
    sApertureReadActive = 0;
    sigaction(SIGBUS, &fOldBus, NULL);
    sigaction(SIGSEGV, &fOldSegv, NULL);
    sApertureGuardInUse = 0;
  }
  bool Copy(const volatile uint32 *source, uint32 *target, size_t count) {
    if (!fReady)
      return false;
    if (sigsetjmp(sApertureReadJump, 1) != 0)
      return false;
    sApertureReadActive = 1;
    for (size_t i = 0; i < count; i++)
      target[i] = source[i];
    sApertureReadActive = 0;
    return true;
  }

private:
  bool fReady;
  struct sigaction fOldSegv, fOldBus;
  ApertureReadGuard(const ApertureReadGuard &);
  ApertureReadGuard &operator=(const ApertureReadGuard &);
};

// Caller holds shared accelerant_lock so backing allocation cannot be freed.
// app_server may still render; this captures CPU-visible pixels, not an atomic
// desktop screenshot or proof that display scanout reads the same backing.
static int ReadAperturePixels(const intel_shared_info &info,
                              const char *path = NULL,
                              bool allowGuardedOriginal = false) {
  if (info.gpu_pci_vendor != 0x8086 || info.gpu_pci_device != 0x46d1 ||
      info.gpu_pci_revision != 0 ||
      !info.device_type.IsModel(INTEL_MODEL_ALDM) ||
      info.bits_per_pixel != 32 || info.bytes_per_row != 7680 ||
      info.current_mode.virtual_width != 1920 ||
      info.current_mode.virtual_height != 1080 ||
      (info.frame_buffer_offset & 4095) || !info.graphics_memory ||
      info.frame_buffer !=
          (addr_t)info.graphics_memory + info.frame_buffer_offset) {
    fprintf(stderr, "Aperture pixel geometry/address guard failed\n");
    return 1;
  }
  const size_t bytes = 7680U * 1080U;
  if (info.frame_buffer_offset > info.graphics_memory_size ||
      bytes > info.graphics_memory_size - info.frame_buffer_offset) {
    fprintf(stderr, "Aperture framebuffer bounds guard failed\n");
    return 1;
  }
  ssize_t cookie = 0;
  area_info area;
  area_id existing = -1;
  unsigned existingProtection = 0;
  while (get_next_area_info(B_SYSTEM_TEAM, &cookie, &area) == B_OK) {
    if (strcmp(area.name, "intel graphics aperture") != 0)
      continue;
    printf(
        "Existing aperture area id=%ld address=%p size=%lu protection=%08lx\n",
        (long)area.area, area.address, (unsigned long)area.size,
        (unsigned long)area.protection);
    if (area.address != (void *)info.graphics_memory ||
        area.size != info.graphics_memory_size)
      continue;
    if (existing >= B_OK) {
      fprintf(stderr, "Ambiguous existing aperture area\n");
      return 1;
    }
    existing = area.area;
    existingProtection = area.protection;
  }
  if (existing < B_OK) {
    fprintf(stderr, "Matching existing aperture area unavailable; no new "
                    "mapping attempted\n");
    return 1;
  }
  void *address = NULL;
  area_id clone = clone_area("readonly existing aperture pixels", &address,
                             B_ANY_ADDRESS, B_READ_AREA, existing);
  if (clone < B_OK) {
    fprintf(stderr, "Existing aperture readonly clone denied: %ld\n",
            (long)clone);
    if (!allowGuardedOriginal || !(existingProtection & B_READ_AREA)) {
      fprintf(stderr, "Guarded original-address read not enabled/allowed; no "
                      "mapping attempted\n");
      return 1;
    }
    // B_READ_AREA on the exact existing global B_ANY_KERNEL_BLOCK_ADDRESS
    // aperture is required. Never treat a kernel pointer as generally readable.
    address = info.graphics_memory;
    printf("Using explicitly requested fault-guarded original aperture address "
           "%p\n",
           address);
  } else {
    area_info actual;
    if (get_area_info(clone, &actual) != B_OK ||
        actual.size != info.graphics_memory_size ||
        !(actual.protection & B_READ_AREA) ||
        (actual.protection & B_WRITE_AREA)) {
      fprintf(stderr, "Readonly clone protection/size guard failed\n");
      delete_area(clone);
      return 1;
    }
  }
  ApertureReadGuard guard;
  const volatile uint32 *pixels =
      (const volatile uint32 *)((const uint8 *)address +
                                info.frame_buffer_offset);
  const unsigned sampleRows[] = {0, 100, 500, 1000, 1079};
  for (unsigned y : sampleRows) {
    uint32 values[16];
    if (!guard.Copy(pixels + y * 1920, values, 16)) {
      fprintf(stderr,
              "Aperture read fault at row %u; returning through lock cleanup\n",
              y);
      if (clone >= B_OK)
        delete_area(clone);
      return 1;
    }
    for (unsigned x = 0; x < 16; x++)
      printf("aperture-pixel[%u,%u]=%08lx\n", x, y, (unsigned long)values[x]);
    for (unsigned x = 100; x <= 1000; x += 100) {
      uint32 value;
      if (!guard.Copy(pixels + y * 1920 + x, &value, 1)) {
        fprintf(stderr, "Aperture read fault at %u,%u\n", x, y);
        if (clone >= B_OK)
          delete_area(clone);
        return 1;
      }
      printf("aperture-pixel[%u,%u]=%08lx\n", x, y, (unsigned long)value);
    }
  }
  if (path == NULL) {
    if (clone >= B_OK)
      delete_area(clone);
    return 0;
  }
  FILE *output = fopen(path, "wb");
  if (!output) {
    perror("Aperture output");
    if (clone >= B_OK)
      delete_area(clone);
    return 1;
  }
  bool ok = fprintf(output, "P6\n1920 1080\n255\n") > 0;
  uint32 hash = 2166136261U;
  uint8 row[1920 * 3];
  for (unsigned y = 0; y < 1080 && ok; y++) {
    uint32 values[1920];
    if (!guard.Copy(pixels + y * 1920, values, 1920)) {
      ok = false;
      break;
    }
    for (unsigned x = 0; x < 1920; x++) {
      uint32 pixel = values[x];
      row[x * 3] = (uint8)(pixel >> 16);
      row[x * 3 + 1] = (uint8)(pixel >> 8);
      row[x * 3 + 2] = (uint8)pixel;
      for (unsigned b = 0; b < 4; b++)
        hash = (hash ^ (uint8)(pixel >> (8 * b))) * 16777619U;
    }
    ok = fwrite(row, 1, sizeof(row), output) == sizeof(row);
  }
  if (fclose(output) != 0)
    ok = false;
  printf("CPU-aperture framebuffer fnv1a32=%08lx output=%s complete=%u\n",
         (unsigned long)hash, path, (unsigned)ok);
  if (clone >= B_OK)
    delete_area(clone);
  return ok ? 0 : 1;
}
#endif

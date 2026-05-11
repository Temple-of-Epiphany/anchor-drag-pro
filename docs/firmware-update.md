# How to Update Your Anchor Drag Pro

**Version:** 0.1.0
**Date:** 2026-05-11
**Audience:** End customers

This is the customer-facing firmware update procedure. It also serves
as the source for the anchordrag.com/docs/firmware-update page.

## What you'll need

- A microSD card — **4 GB or larger**, **FAT32 or exFAT** format
  (most cards come this way; see [SD card formatting](#sd-card-formatting) below if you're unsure)
- A computer with an SD card reader (built-in slot or USB adapter)
- About 5 minutes

You do **not** need any cables, special software, or internet access on the device itself.

---

## SD card formatting

Anchor Drag Pro reads firmware from a microSD card formatted in one
of two filesystems:

| Card size | Format | Notes |
|---|---|---|
| **4 GB – 32 GB** | **FAT32** | The default for almost all cards in this size range. Recommended. |
| **64 GB and larger** | **exFAT** | The default for high-capacity cards. Also supported. |

**Cards smaller than 4 GB** technically work (FAT16) but are rare to find
new — and we don't recommend them because firmware updates plus future
log files exceed their capacity.

**Other formats** (NTFS, ext4, APFS, HFS+) are **not supported**. The
device will refuse to mount them and you'll see a warning. If your card
isn't FAT32 or exFAT, reformat it as FAT32 using your computer's disk
utility before continuing.

### Reformatting a card

- **macOS:** Disk Utility → select the card → Erase → Format: MS-DOS (FAT) for ≤32 GB, ExFAT for >32 GB
- **Windows:** File Explorer → right-click the card → Format → File system: FAT32 or exFAT
- **Linux:** `mkfs.vfat -F 32 /dev/sdX1` (FAT32) or `mkfs.exfat /dev/sdX1` (exFAT)

We recommend dedicating an SD card to your Anchor Drag Pro device — keep
firmware updates, your config file, and (eventually) data logs on it.

---

## Updating in three steps

### Step 1 — Download the new firmware

Visit https://anchordrag.com/downloads and select the latest release.

Download **both** files for your device variant:

- `anchor-drag-pro_v<VERSION>_<variant>.bin` — the firmware itself
- `anchor-drag-pro_v<VERSION>_<variant>.sha256` — a fingerprint your device uses to verify the file downloaded correctly

For example, version 0.3.0 of the Display variant:

```
anchor-drag-pro_v0.3.0_display.bin
anchor-drag-pro_v0.3.0_display.sha256
```

> **Which variant do I have?** The variant is printed on the back of your
> device and on the box. If you only see one set of files, it's because
> the firmware works on every variant — that's fine, just download both.

### Step 2 — Copy to the SD card

1. Insert the SD card in your computer
2. Open the SD card to view its contents
3. Find or create a folder named exactly **`firmware`** at the top level of the card
4. Copy **both downloaded files** into the `firmware` folder
5. Eject the SD card safely:
   - **macOS:** drag the card icon to the Trash, or right-click → Eject
   - **Windows:** right-click the card icon in File Explorer → Eject

The SD card should now look like this:

```
SD CARD
└── firmware/
    ├── anchor-drag-pro_v0.3.0_display.bin
    └── anchor-drag-pro_v0.3.0_display.sha256
```

You can leave older firmware files in the `firmware` folder if you want —
the device always picks the newest version. Or delete them to keep things tidy.

### Step 3 — Install the update on the device

1. **Power off** your Anchor Drag Pro device
2. If a different SD card is in the device, remove it
3. Insert the SD card with the new firmware
4. **Power the device back on**

The touchscreen will show:

```
   Firmware update available

   Current:  v0.2.0
   New:      v0.3.0

   [  Update  ]   [  Skip  ]
```

5. Tap **Update**
6. The device shows a progress bar — this takes 1-2 minutes
7. The device automatically reboots when the update finishes
8. The new version number appears on the home screen

**You're done.** The new firmware is installed and active.

---

## Frequently Asked Questions

### What if I tap Skip instead of Update?

Nothing happens — the device boots normally on the current firmware. You
can install the update later by power-cycling the device with the SD card
still inserted.

### What if the update fails partway through?

The device automatically rolls back to the previous version on the next
boot. You'll see a warning on the screen, but the device is still
working with your old firmware. Try the update again, or visit
[support](https://anchordrag.com/support) if it keeps failing.

### Can I make the device unusable ("brick" it) with a bad firmware update?

**No.** Anchor Drag Pro has built-in protection:

1. Before flashing, the device verifies the firmware file matches the
   `.sha256` fingerprint. A corrupted download is refused.
2. After flashing, the new firmware runs a brief self-test. If it
   doesn't pass within 30 seconds — including if the device crashes
   or freezes — the previous firmware is automatically restored on the
   next boot.

We can't make it impossible to break a device, but firmware updates are
specifically designed to be safe. Your alarm will still work even after
a bad update attempt.

### Can I skip versions? (e.g., v0.1.0 → v0.5.0)

Yes. The device just takes the highest-version `.bin` file in the
`firmware` folder. You don't need to install intermediate versions.

### Can I downgrade to an older version?

Not via the regular update process — the device only installs versions
*newer* than the running one. If you genuinely need to downgrade,
contact [support](https://anchordrag.com/support).

### Do I need to be at the helm during the update?

No. The update only happens at boot, only after you tap **Update** on
the touchscreen, and the device is otherwise normal during the rest of
the time. You can install the update before going out, before docking,
or whenever is convenient.

### Will the update erase my settings?

No. Your configuration (alarm distance, GPS source preferences, etc.) is
stored in a separate area of the device's memory and is preserved across
firmware updates.

### How often do you release updates?

Typically 1–4 times per year, mostly bug fixes and security improvements.
We don't release updates for the sake of releasing — every update has a
specific reason listed in the release notes on the downloads page.

### Can I get notified of new releases?

Yes, the [downloads page](https://anchordrag.com/downloads) has a feed
you can subscribe to. We don't send marketing email — only release
announcements when a new firmware is available.

---

## Troubleshooting

### "Firmware update available" never appears

Check, in order:

1. **SD card readable?** Boot logs should show "SD card mounted." If not, the card may be wrongly formatted (see [SD card formatting](#sd-card-formatting)) or unreadable.
2. **Folder named exactly `firmware`?** Lowercase, no extra characters. The device looks for that exact name.
3. **Both `.bin` and `.sha256` files present?** A `.bin` without a sidecar is silently skipped — without the fingerprint, the device can't verify the file is intact.
4. **Filename matches the pattern** `anchor-drag-pro_v<VERSION>.bin`? If you renamed the file, the device won't recognize it. Use the original filename.
5. **Version actually newer than running?** The device only updates to a higher version. Check the version on the home screen.

### "Firmware verification failed" or "SHA256 mismatch"

The `.bin` file was corrupted during download or transfer. Re-download
both files (a clean download includes a fresh `.sha256`), copy to the SD
card again, and retry.

### "Update failed" mid-flash

Most common cause: SD card removed mid-update. Power off, reinsert the
SD card cleanly, power back on, and tap Update again. The device's
auto-rollback ensures the previous firmware still works in the meantime.

### Device boots with the OLD version after I confirmed the update

If the new firmware crashed during its self-test, the auto-rollback
kicks in. Try downloading the firmware again (in case of corruption),
or contact [support](https://anchordrag.com/support) — that's a real
issue we want to know about.

---

## For Developers

If you're a firmware developer rather than an end customer, see:

- [`docs/release-pipeline.md`](./release-pipeline.md) — How firmware gets from `git tag` to the public download
- [`platform/esp32/main/ota.c`](../platform/esp32/main/ota.c) — The on-device update logic

For the OTA file naming and SHA256 sidecar format conventions:

```
anchor-drag-pro_v<MAJOR>.<MINOR>.<PATCH>.bin
anchor-drag-pro_v<MAJOR>.<MINOR>.<PATCH>.sha256
```

The `.sha256` file is in standard `sha256sum` format:

```
<64 hex chars>  <filename>
```

Generated locally with `shasum -a 256 <bin> > <sha256>` or by the
release pipeline (`.github/workflows/release.yml`).

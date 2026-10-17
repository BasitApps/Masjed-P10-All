# Approved deep-review fixes — 2026-10-11

Implemented in the actual P10 and sibling `Masjed_TA_All_v2` sources. Existing
owner edits were retained; originals of the touched production files were saved
under the local `deep-fixes-20261011/before` artifact directory before edits.
The approved prayer formula and simple root POST protocol remain unchanged.
No Android BIN publication, APK build/install or firmware release-version bump
is part of this batch. A later owner-approved live check uploaded only the P10
4×1 candidate to COM5; M73/M73s were not uploaded.

## Six findings closed

1. Both families reject OTA during pending restart. Upload chunks, aborts and
   completion of a subsequent request cannot overwrite the approved image or
   cancel its restart. Busy completion returns HTTP503.
2. Both build-local HTTP overlays reject request-argument allocation failure
   before plain-body access; GET and POST paths are covered. Vendor SDK/lib
   sources were not edited.
3. M73 saves verify the temporary config before atomic replacement and RAM
   publication. Settings handlers no longer re-read after commit and leave a
   mixed runtime state on a subsequent read failure.
4. M73 rejects the entire multipart request after a second file. A third file
   cannot recover that failed request; completion/abort permits a fresh request.
5. M73 accepts only complete, valid 8–63-byte AP password files, rejecting
   short reads, embedded NULs and invalid lengths. Unverified passwords keep
   WiFi off. Legacy numeric-password fallback requires confirmed file absence.
6. New M73 config saves have a magic/version/size envelope and CRC32 over the
   encrypted payload and envelope, verified before decryption or migration.

## Review follow-ups included

- Preserve the first upload object if allocating the second fails, ensuring
  the abort callback releases request state. Overlays are byte-identical.
- M73's `StorageFileLookup` distinguishes LittleFS errors from `NOENT`; both
  password loading and all four startup absence checks use it. A small local
  compatibility accessor uses the pinned Core3.1.2 LittleFS implementation,
  without unsafe downcasts of the global FS object or SDK changes. Re-review
  this accessor if the SDK/filesystem implementation changes.
- If a failed factory reset cannot compensate the config write, stop treating
  it as committed and keep ECO consistent with the actual current config.
- When an iqama/location/settings save recovers an untrusted config, refresh
  time and prayer caches before normal operation/acknowledgement.

## Compatibility and limits

M73 storage format becomes2; the `Configuration` payload layout and firmware
versions remain unchanged. All four supported legacy raw layouts load without
reset or boot write, preserving other settings. Existing valid100–120-minute
iqamas normalize to99, as approved previously. The next explicit save upgrades
to CRC protection, even when values did not change.

Legacy raw records cannot retroactively gain a checksum. CRC detects accidental
corruption, not malicious tampering or firmware copying. An older M73 firmware
cannot read the new protected record: a downgrade requires an explicit storage
conversion/compatible reader, not merely flashing the older BIN. No conversion
tool, A/B journal or cross-file power-loss transaction was added.

## Verification

- `test/firmware_fault_checks.py`:27/27 production-function fault cases passed.
  Covers pending-restart OTA, multi-file rejection and clean retry in both
  families; failed config writes/readback/rename; every protected record byte
  flipped once; every truncated new-record length; four legacy layouts and
  explicit migration; malformed passwords; lookup failures; reset compensation;
  empty/partial startup stores; and recovery cache refresh.
- `test/http_parser_fault_checks.py`:16/16 real-parser cases passed, including
  plain GET/POST allocation failure and second-file OOM with abort notification.
- Existing local regression runner:25 baseline +14 prior fix cases passed;
  three C++ suites and the three M73 settings tests passed. The old source-grep
  settings assertions were replaced by actual behavior tests.
- Existing integration runner passed MP3 packet/pacing, Android map binding and
  iqama-limit fixtures, plus11 multipart cases. These are stand-in hardware/UI
  tests, not a new Android build or physical-phone test.
- P10 real-struct persistence scenarios:7/7; shared safe-mount faults:7/7.
- Six shared core headers and both parser overlays are byte-identical.
- P10 release builds1×1,2×1,3×1,4×1 and M73/M73s builds passed. M73 builds ran
  with `BASIT_PUBLISH_ASSETS=0`; both explicitly logged validation-only, no copy.
- Final independent bounded source review reported no remaining Critical or
  Important findings. The reviewer did not independently execute tests/builds;
  the main agent ran the checks above against the final sources.
- `git diff --check` passed in both repositories. Existing PrayerTimes static
  declarations produce warnings; vendor files were deliberately left untouched.

New fault cases were first run against the faulty production behavior, confirmed
failing, then rerun after fixes. A missing return in the renamed multipart test
entrypoint was a harness issue and was corrected separately.

Physical save/RTC/OTA/display/audio, real flash failures/power interruption and
long-run validation of this new candidate remain pending. Host tests model
flash and filesystem behavior; they do not establish power-loss durability or
the cause of historical intermittent hardware resets.

## Owner-approved live P10 check

The final `p10_4x1_release` was uploaded over COM5; esptool verified the flash
write and the board returned to `profile=4x1`. The AP remained reachable at
`192.168.1.177`. Controlled `/setDateTime` jumps exercised pre-prayer (three
minutes), azan, iqama countdown, last minute, iqama, sleep and azkar in both
large and small font modes. Every phase returned `rtc=1` and `timeValid=1`;
uptime increased from 205 to 316 seconds and no new reset was reported. The
time was restored afterward and final `getData` confirmed `fS=0`, original
location, iqama values and sleep settings.

The live check proves firmware state transitions and device health, but the
panel has no pixel/readback telemetry. The owner must visually confirm that the
large-font countdown is static and that the Arabic wording is legible.
M73/M73s physical validation and power-loss/long-soak testing remain pending.

### Local build identity (not published releases)

| Build | BIN SHA256 |
|---|---|
| P10 1×1 release | `e287bda7f462a8baece5f39413ebef377c37e74aa35acbabc6e631af16a9b3b4` |
| P10 2×1 release | `33a20f71bd0f2f91b39ec0a70bc1b14e6470ee85d7718c134a113f9863f3f10d` |
| P10 3×1 release | `e6b748845409ed574819e03810507072eedc59b70d297f0d5d7d96907d8be71e` |
| P10 4×1 release | `afa69c9dc640e90f46c34624946276f87b28ae083849cbd791b97b691f59122f` |
| M73 | `5c3c535983cda4d37b41c3c799d2cd45542a353df298243c6349b69fff9ad95c` |
| M73s | `963f99882e877301bfc5dc685b076e09d5c221b478c391b6f081745cfa78c613` |

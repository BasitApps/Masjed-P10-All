# Cross-model release readiness audit — 2026-10-17

**Decision: do not distribute a final market-wide update yet.** Six current
firmware profiles and Android build successfully, but verified storage/password
failure paths, legacy capability mismatches and incomplete release validation
remain. This was an audit: no production fix, version bump, hardware operation,
Android asset replacement, APK installation, signing or publication was done.

## Scope and evidence

Reviewed current P10, M73/M73s, Android and the located historical M71, M71s,
M722, P170 and P170s sources. Inspected all 12 catalogued BIN assets. O72's
bundled identity was verified, but its matching source was not located. M71
firmware work remains **DEFERRED**; an M73 image must never be used for M71.

Repositories:

- P10: `C:/Users/m_nag/Desktop/Arduino/Projects/Masjed-P104-Esp-v2`
- M73: `C:/Users/m_nag/Desktop/Arduino/Projects/Masjed_TA_All_v2`
- Android: `C:/Users/m_nag/Desktop/Android_Projects/MasjedClockWiFi`
- Historical firmware: sibling projects under `C:/Users/m_nag/Desktop/Arduino/Projects/`

Raw evidence is in
`C:/Users/m_nag/.codex/visualizations/2026/10/17/01a14900-8f6f-7a02-ac4f-427e59861ad9/release-audit/`:
build logs, `verification-summary.json`, `model-asset-inventory.json/.csv`,
`native-test-status.json`, and two extracted-source fault reproducers.

Source references below are repository-relative to the named repository. Host
fault reproducers extract the actual production branches/functions and replace
filesystem collaborators. They prove software failure paths under modeled I/O
faults; they do not establish physical flash failure frequency or simulate all
LittleFS/power-loss behavior.

## What passed now

| Check | Fresh result | Limit |
|---|---|---|
| P10 release builds | 4/4: `p10_1x1_release` through `p10_4x1_release` | Compile/link only |
| Seven-segment builds | 2/2: `ta73`, `ta73s` | `BASIT_PUBLISH_ASSETS=0`; no asset copying |
| Android unit tests | 34 passed, zero failures/errors/skips | Local JVM tests, not phone execution |
| Android `lintRelease` | Passed, 24 warnings | Warnings remain; not a device test |
| Android `assembleRelease` | Passed: v2.9/code29, **unsigned** APK | Requires production signing before distribution |
| M73 native tests | `api_number_test`, `prayer_contract_test`, `prayer_phase_test`: 3 passed | No physical RTC/audio/OTA |
| Shared core headers | Six byte-identical P10/M73 headers | Identity does not prove all handlers equivalent |
| Bundled firmware | 12/12 present, image CRC checks passed | CRC establishes integrity, not suitability |
| Built APK assets | All 12 match the current Android source assets | Those assets are not the six fresh modern builds |
| Targeted fault checks | P10 2 cases; M73 password 3 cases passed | Confirmed undesirable paths, not successful safety tests |

The six shared headers are `CivilTimeContract.h`, `PrayerMethodContract.h`,
`PrayerPhaseContract.h`, `CompactApiContract.h`, `ClockSettingsContract.h`, and
`ClockStorageContract.h`. P10 RAM usage is 38528/38552/38644/38644 bytes for
1–4 panels; M73/M73s use 35648/36064 bytes. Existing library warnings remain.

The existing M73 `test/api_settings_contract_test.py` did **not** validate its
three cases: all error because its delegated P10
`test/firmware_fault_checks.py` is absent. Do not carry forward old passing
claims for that harness. The two small audit reproducers do not replace it.

## Confirmed findings, ranked

### 1. P1 — P10 can initialize over existing settings after lookup errors

P10 `src/Masjed-P104.cpp:2943` treats three false `LittleFS.exists()` results
as proof of a new store after `loadConfig()` fails. The installed LittleFS
implementation returns false for both missing files and stat errors. If the
only valid record is slot A and transient load/lookup errors occur, this branch
calls `resetConfig()` and can replace that record with defaults.

The extracted startup branch reproduced the call and overwrite with modeled
lookup errors. This is conditional on faults, not a claim that ordinary boot
resets settings. Require explicit `LFS_ERR_NOENT` for every relevant path, using
the existing M73 lookup pattern; otherwise preserve the files and report failure.

### 2. P1 — P10 substitutes the factory AP password after unreadable settings

P10 `src/Masjed-P104.cpp:2942` installs defaults in RAM when loading fails,
including when existing files are deliberately retained. `initAP():1557` then
starts Wi-Fi with those replacement credentials. A device with a customized
password can consequently expose settings and OTA through the known factory
password following storage read/validation failure.

Source tracing confirms the path; no physical fault was induced. Gate AP
startup on successfully loaded or explicitly initialized credentials, retaining
the physical factory-reset recovery path. M73 already keeps Wi-Fi off when its
password cannot be verified (`src/Masjed_TA.cpp:1733`).

### 3. P1 — Android accepts M71 passwords that fail secured AP startup

Android `MainActivity.java:147` accepts 4–8 digits for legacy passwords. M71
`src/masjed-71.cpp:1750` saves `atoi(password)` and reports success. Its startup
`:1459–1464` formats the integer without leading zeroes and calls `softAP()`.
For `1234` or `01234567`, the resulting password is shorter than eight
characters and the installed ESP8266 core rejects it.

The requested secured AP fails; phone access may be lost. SSID disappearance
is not guaranteed because the core enables AP mode before checking the PSK.
The physical recovery at `:1972` resets/formats settings. The smallest bounded
fix is Android-only M71 validation: exactly eight digits, first digit nonzero.
This does not authorize reopening the deferred M71 firmware work.

### 4. P1 — M71s/M722 password changes report success without changing AP access

Android `classes/DeviceIdentity.java:41` exposes password changes for both
models. Their setters save a value and return `passwordOk`
(`Masjed-71s/src/masjed-71s.cpp:1471`,
`Masjed-722/src/masjed-722.cpp:1477`), but startup always uses literal
`12881288` (`:1169`, `:1207` respectively).

The user is told a password changed while the AP continues using its factory
password. For these historical releases, hide/disable the unsupported operation
in Android until a separately reviewed firmware change actually supports it.

### 5. P2 — P10 config and M73 password saves can acknowledge unreadable data

P10 `src/Masjed-P104.cpp:299–317` and M73
`src/Masjed_TA.cpp:156–165` check the write byte count, call void
`flush()/close()`, then rename without reopening/verifying the temporary file.
The installed LittleFS wrapper discards the sync error. Both extracted functions
returned success after a modeled full-count write followed by sync corruption.

P10 advances its committed RAM state although the replacement slot is unreadable;
its previous slot survives this modeled failure. M73 publishes the new password
in RAM, but next boot rejects the invalid password file and keeps Wi-Fi off.
Reopen and verify the temporary record/password before replacement and RAM
commit, following M73's existing config readback pattern at `:761–766`.

### 6. P2 — Android turns modern device refusals into connection failures

`classes/SecureClockApi.java:88` throws on non-2xx HTTP before interpreting the
v2 error body. `MainViewModel.java:111–129` then classifies the resulting
`IOException` as a connection failure. For example, HTTP500
`v=2 / ok=0 / error=storage` loses its actual storage error; the specific UI
message in `MainActivity.java:547` is not reached through this path.

Parse/validate structured device errors on non-2xx responses too, while keeping
transport/protocol failures distinct. Add one regression test for a storage
refusal and one transport failure.

### 7. P2 — Legacy screens offer settings that their transport discards

Android shows the Makkah Isha override without a firmware capability guard
(`MainActivity.java:898–904`), but `LegacyClockApi.java:77–85` drops
`makkahIsha` for every legacy family. Day brightness is also displayed for
M71/M71s/M722/O72 (`MainActivity.java:2351–2354`) and dropped by that adapter.
P170/P170s show font/language/digit groups (`:2377–2413`), while their legacy
save branch (`:2797–2810`) sends only the supported message/name/speed/size.

The request can succeed and clear the dirty state while those selections never
reach the clock. Hide unsupported controls using the existing model identity
policy; do not silently add fields to historical firmware protocols.

### 8. P2 — Technician reset does not consistently complete its UI flow

For P170/P170s, Android exposes reset (`MainActivity.java:2308`, `:2771`), but
neither historical route list registers `/resetDefault`
(`Masjed-P170/src/masjed-P170.cpp:1329`,
`Masjed-P170-s/src/masjed-P170-s.cpp:1389`); the request receives 404.

On models that support it, Android `MainActivity.java:2102` only toasts after
`resetDefaultOk`: the disabled reset button and pre-reset form are not refreshed,
and the loaded-data guard prevents the intended reload on reconnect. Reopening
the screen recovers, but a later save can restore stale settings. Gate unsupported
reset controls and reload supported defaults after the restart grace period.

### 9. P2 — P10 font storage failures use the wrong API error

P10 `src/Masjed-P104.cpp:2232` combines parsing and `installConfig()` failure in
one branch, returning HTTP422 `arguments` for a storage failure. Other save
handlers return HTTP500 `storage`. Separate validation from installation failure.
This is a real error-contract mismatch; it does not mean normal font saves fail.

### 10. P2 — Location instrumented tests launch with an invalid model fixture

Android `app/src/androidTest/java/com/basitapps/masjedclockwifi/LocationSelectionTest.java:73`
passes model `104` and SSID `LocationRegression`. `MainActivity.java:329–335`
accepts the explicit model only when it contains `-`; SSID fallback cannot
identify this fixture, so activity initialization returns before the tested UI.
All 13 location cases use that fixture. Use `P-104` and rerun on a device or
emulator. These instrumented tests were not run in this audit.

### 11. P3 — M73 password responses do not follow shared escaping rules

M73 `src/Masjed_TA.cpp:1984/1994` interpolates an accepted printable password
directly into `/getData`. Backslashes are legal password characters. Extracted
handler checks show `abcdefgh\q` violates compact escaping and literal
`abcdefgh\n` decodes differently under the shared decoder. P10 escapes fields.

Escape this field before sending it. Current Android retains raw response values
and does not read the password field; therefore a current Android connection
failure from this mismatch was **not** established. This is wire-contract drift
affecting a decoder that follows the shared escaping contract.

## API unification verdict

Modern P10 and M73 share the same core POST routes: `/getData`, `/health`,
`/resetDefault`, `/selfTest`, `/setDateTime`, `/setEqama`, `/setLocation`,
`/setPassword`, `/setSettings`, `/testMode`, plus GET `/checkUpdate` and
multipart POST `/updateMain`. M73 is synchronous now. P10 display/card/font/
network routes are legitimate model-specific capabilities.

That is substantial shared structure, **not complete behavioral parity**:
findings 5, 9 and 11 remain. Historical models intentionally use GET adapters
(plain or old encoding). They should be supported by explicit app capability
rules, not described as all implementing the modern API. Existing legacy
response positions and the inspected parameter mappings agree with the app.

The catalog's P101 dual capability flag is not runtime version negotiation:
the app selects modern transport by model. Known historical P101 v3 is forced
through OTA before controls, so a reachable v3 control failure was not proved.
Do not add a skip-update path without verifying that old release's contract.

## Per-model release matrix

| Model | Catalog version | Bundled BIN bytes | Protocol/source status | Readiness |
|---|---:|---:|---|---|
| M-71 | 8 | 412352 | Historical GET/encoded; source located | Deferred firmware; Android password fix required |
| M-71s | 5 | 414784 | Historical GET/encoded; source located | Capability corrections; hardware coverage pending |
| M-722 | 5 | 377088 | Historical GET/plain; source located | Capability corrections; hardware coverage pending |
| M-73 | 15 | 411968 | Current POST v2; fresh build passed | Storage-password/readiness gates remain |
| M-73s | 14 | 412752 | Current POST v2; fresh build passed | Same, plus physical audio coverage |
| P-101 | 6 | 427376 | Current POST v2; fresh build passed | P10 findings and hardware gates |
| P-102 | 11 | 428736 | Current POST v2; fresh build passed | P10 findings and hardware gates |
| P-103 | 12 | 428448 | Current POST v2; fresh build passed | P10 findings and hardware gates |
| P-104 | 12 | 430416 | Current POST v2; fresh build passed | P10 findings, visible shimmer still unresolved |
| P-170 | 2 | 382640 | Historical GET/plain; source located | Capability corrections; hardware coverage pending |
| P-170s | 3 | 389152 | Historical GET/plain; source located | Capability corrections; hardware coverage pending |
| O-72 | 1 | 408752 | Bundled `update,O,72,1,good`; matching source unknown | No new firmware release without source provenance |

Five located legacy assets match their existing local `.pio` BINs; those old
builds were not rerun. O72 is not proven equivalent to nearby Masjed-TA6932:
that source advertises O73 and its BIN differs. Do not substitute it.

All six modern bundled assets differ by SHA256 from this audit's builds:
fresh P101/P102/P103/P104 sizes are 428416/428560/430240/430240 bytes;
M73/M73s are 412144/416208 bytes. This is a packaging/readiness gap, not proof
that every difference is defective. The JSON inventory records full hashes.
The P104 asset's `P104-rc4` label is a human-readable marker; its catalog OTA
version is 12. Do not interpret that marker as numeric update version 4.

## Remaining gates and smallest completion order

1. Fix the verified storage/credential paths, then Android legacy capability
   guards and structured error handling. Preserve user data and the approved
   prayer formula; do not broaden the deferred M71 firmware scope.
2. Restore the missing fault-test harness and correct the Android location
   fixture. Add only targeted regressions for the confirmed defects; rerun the
   six builds, Android tests/lint, and relevant existing tests after fixes.
3. Resolve or explicitly obtain owner acceptance of the P10 normal-use shimmer.
   The prior ACTIVE/HOLD/ACTIVE experiment reduced shimmer while updates were
   held and restored it when updates resumed. Frame-update work contributes;
   drawing cost versus buffer swapping and residual shimmer remain unisolated.
   No scan/OE/PWM fix has been implemented.
4. Freeze an identified candidate per supported model; test save/reboot/readback,
   RTC set/read/battery/power interruption, boundary prayer/Friday/DST behavior,
   audio where present, and OTA from representative installed market releases.
   Exercise both phone-connected and disconnected operation, malformed uploads,
   settings retention and recovery. Repeat a useful soak on the exact candidates.
5. Only after those gates, assign monotonic per-model firmware/catalog versions,
   create a BIN/model/version/hash manifest, replace verified Android assets,
   build/sign using the existing production signing identity, and verify upgrade
   over the installed Android app without database/license loss. Test a bounded
   pilot before wider distribution.

Replacing a BIN with the same numeric version is insufficient:
`DeviceList.java:571` and `UpdateActivity.java:153` skip already-current devices.
EEPROM/config schema versions are separate and must not be bumped merely to
advertise a firmware update. No signing key was inspected or changed.

Earlier owner OTA successes are acknowledged; old failed experiments are not
being relabelled as a currently reproduced OTA defect. The older P104 soak also
does not certify these six fresh images. No additional concrete RTC/prayer/OTA
logic defect was established in this bounded source audit.

Not checked here: physical units for every model, electrical/power faults,
phone/emulator execution, production signing, upgrade over the market APK,
O72 source provenance, exact installed market firmware, or a new candidate soak.
Successful compilation and local tests cannot guarantee those outcomes.

## Android handoff follow-up — owner handles store preparation/upload

Owner confirmed the app already exists on Google Play and will prepare/sign/
upload the final application personally. Engineering scope here is technical
readiness verification; Play Console access and signing credentials are not
needed for the current checks. No implementation plan or production fix was
started during this follow-up.

Additional fresh checks:

- `:app:bundleRelease :app:assembleDebugAndroidTest` passed, 96 actionable tasks.
  The latter compiles/packages instrumented tests; it does not execute them.
- `Masjed-v2.9-release.aab` is 6154697 bytes, SHA256
  `e2f85b6ccb1c631a82222620492e4e4e3a9cd56f4f17aa29907f492af4413559`.
  ZIP integrity passed. All12 embedded firmware assets match the reviewed source
  assets. No native `.so` libraries and no JAR signing certificate entries are
  present. It is an unsigned verification artifact, not the final store upload.
- Existing application ID remains `com.basitapps.masjedclockwifi`, target/
  compile SDK37, versionCode29/versionName2.9. Highest Play versionCode and the
  existing signing certificate were not inspected; owner must retain update
  compatibility when preparing the store release.
- The app has a privacy-policy action and URL
  `https://basitapps.com/privacy-policy/`. This browsing tool could not retrieve
  the URL; site availability/content were not verified, and no outage is claimed.
  Store Data safety declarations were not inspected.

Evidence: `release-audit/android-bundle-check.log` and
`release-audit/android-bundle-verification.json` in the task artifact directory.
**Readiness remains blocked by the findings and physical validation above.**
No code, firmware assets, versions or signing configuration were changed.

Official publishing references checked for the owner's later handoff:
[Play App Signing](https://support.google.com/googleplay/android-developer/answer/9842756),
[Data safety](https://support.google.com/googleplay/android-developer/answer/10787469),
[Android App Bundles](https://developer.android.com/guide/app-bundle).

## Release 13 packaging follow-up — 2026-10-17

At the owner's request, the modern release target was narrowed to P101/P102/
P103/P104 and M73/M73s. P10 firmware/catalog versions are **13**. Market M73
and M73s units already advertise 15 and 14, so their unified forward target is
**16**; using 13 would be a downgrade and the app would correctly skip it.
P10 startup markers are P101-13 through P104-13; seven-segment markers are
M73-16 and M73s-16. M73 `/checkUpdate` literals now advertise16. Historical
models were left unchanged.

The publisher's stale P10 output-name mapping was corrected. It now stages all
six builds before replacing Android assets, so one failed build cannot partially
replace the app bundle. Fresh builds passed for all six, their Android assets
are byte-identical, and the expected version markers were found in each BIN.
The catalog and its version tests now expect P10=13 and M73/M73s=16. `testDebugUnitTest`,
`lintRelease` and `bundleRelease` pass after the copy. Verification hashes are
in `release-audit/firmware-modern-verification.json`.

The regenerated AAB is
`C:/Users/m_nag/Desktop/Android_Projects/MasjedClockWiFi/app/build/outputs/bundle/release/Masjed-v2.9-release.aab`,
6159880 bytes, SHA256
`4eae8b37e9a3fd93d886644062f65855f231e21967e4d54641f90ac8bbf0d538`.
It was built locally only; the existing app signing identity and Play upload
compatibility remain the owner's responsibility.

This packaging step did not fix the previously ranked storage/API findings or
the P10 shimmer. It did not flash hardware, run physical OTA, sign/upload the
AAB, or change any historical firmware asset. Treat release 13 as a staged
candidate until those gates are explicitly accepted.

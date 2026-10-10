# Android deferred work — Makkah Isha rule

Both firmware paths now expose the same optional `makkahIsha` setting without
changing the legacy CSV contract:

- `0`: use the selected calculation method (the compatibility default).
- `1`: use a fixed 90 minutes after Maghrib when `prayerMethod=1` (Makkah).

Android work is intentionally deferred; no APK or Android source is changed in
this firmware session. When resumed:

1. Read `makkahIsha` from the POST `/getData` response. If the field is absent, keep the
   selector hidden and preserve the old-device flow.
2. Show a compact localized selector only when `prayerMethod=1`: “حسب طريقة الحساب” or
   “90 دقيقة ثابتة”.
3. Send `makkahIsha=0|1` with the existing POST `/setSettings` request and verify
   the readback before showing success.
4. Keep the legacy `/getData` CSV unchanged; route the new setting through the
   profile-specific v2/GET capability path rather than appending CSV columns.
5. Test persistence across restart, switching away from Makkah, old firmware,
   and both Arabic/English resources.

The firmware default is mode `0` so existing installations keep their prior
angle-derived behavior until the user explicitly chooses fixed 90 minutes.

## Egypt locations — next bounded batch (2026-10-06)

Offline “Add my location” is implemented in the Android project; owner
phone/GPS/P10/7-segment verification is pending. See Android API.md and
ROADMAP.md for the current implementation. Do not duplicate this feature
or add a map/coordinate form; keep the main Location screen compact.

GPS implementation did not expand the data. The next bounded batch, approved
and implemented on 2026-10-06, added six reviewed towns (see below). Remaining
expansion rules:

1. Keep all bundled cities and their IDs/coordinates. Existing records win
   duplicates; additions must not disrupt stored firmware city references.
2. Use open-admin-data names/parent links as candidates, not certified mosque
   coordinates. The inspected repository has 27 governorates, 365 districts
   and 5,716 shiyakhas, three-decimal positions and missing source metadata
   for district/shiyakha records; administrative centres can differ from
   settlement centres. Completeness/accuracy is not guaranteed.
3. Verify settlement positions against independent sources and record
   provenance/license. GeoNames/OSM disagreement by name alone is a review
   flag, not proof of an error (homonyms and administrative representative
   points exist). Prioritize confirmed towns/centres; add villages only with
   verified positions, without forcing extra nested selectors on the user.
4. Compare a small verified Egyptian sample with ESA, matching dates, method,
   timezone/DST and rounding. Leave the owner's approved prayer formula alone.
   The 2026-10-06 Cairo/Alexandria/Aswan reference comparison is recorded in
   `docs/LOCATION_VALIDATION.md`: maximum displayed difference one minute;
   P10/7-segment native outputs match. Handset GPS is still unverified.
5. Keep this enhancement optional outside Egypt; preserve country-specific
   existing hierarchy. Native GPS remains independent of administrative data.

Research links: https://github.com/open-admin-data/egypt-administrative-divisions
and https://www.esa.gov.eg/praytimes.aspx . No automatic bulk village import.

### First reviewed additions — completed 2026-10-06

- Owner accepted the observed one-minute ESA discrepancy; no prayer correction.
- Android IDs 290–295 add six towns: المنزلة، ميت سلسيل، القناطر الخيرية،
  الكردي، جمصة، أبو زنيمة. Existing IDs/coordinates and sold-device tables
  are preserved. Current total: 263 places / 261 Egypt.
- GeoNames WGS84 coordinates, Egyptian identity/governorate references and
  independent OSM settlement comparison recorded in Android LOCATION_DATA.md.
  Neither administrative-centroid coordinates nor bulk villages were imported.
- New bundled cities appear alongside installed records without app-data clearing;
  installed records take priority. Public localized source/license dialog in Settings.
- INSERT-only append tool is idempotent; DB integrity and original rows checked.
  Android Java/resource compile passed, no APK or firmware/version changes.
- Owner phone checks still pending. Continue only reviewed additions, preserving
  IDs, not the historical destructive tools/rebuild_db.py.

### Second additions / authorized classification corrections — 2026-10-06

- Added Android IDs 296–310 (15 reference places), total 278 places / 276 Egypt.
- Governorate-only corrections: 39→17, 115→20, 116→20, 118→10, 121→20,
  131→19, 132→19. Owner confirmed IDs 115/116 near/in Qillin and expressly
  requested no coordinate correction for Nashart. All old names/points/IDs kept.
- Installed databases synchronize only bundled governorate membership; no DB
  replacement/version bump. Shared restoreBoardLocation resolves canonical
  governorate by stable city ID for both display models without auto-saving.
- Append tool transaction checks permitted changes and protected tables; second
  run changes nothing. GeoNames/Wikidata sources/licensing documented; no APK,
  firmware/API/prayer/version changes. Java/resource compile passed.
- Remaining modern cities and ID 128 "64 الخاشعه" are listed in Android
  LOCATION_DATA.md; do not bulk import, guess a point or declare full coverage.
- Owner installed-app checks pending, including GPS and non-Egypt hierarchy.

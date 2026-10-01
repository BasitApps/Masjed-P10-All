# Android deferred work — Makkah Isha rule

Both firmware paths now expose the same optional `makkahIsha` setting without
changing the legacy CSV contract:

- `0`: use the selected calculation method (the compatibility default).
- `1`: use a fixed 90 minutes after Maghrib when `cm=1` (Makkah).

Android work is intentionally deferred; no APK or Android source is changed in
this firmware session. When resumed:

1. Read `makkahIsha` from `/v2/getData`. If the field is absent, keep the
   selector hidden and preserve the old-device flow.
2. Show a compact localized selector only when `cm=1`: “حسب طريقة الحساب” or
   “90 دقيقة ثابتة”.
3. Send `makkahIsha=0|1` with the existing `/v2/setSettings` request and verify
   the readback before showing success.
4. Keep the legacy `/getData` CSV unchanged; route the new setting through the
   profile-specific v2/GET capability path rather than appending CSV columns.
5. Test persistence across restart, switching away from Makkah, old firmware,
   and both Arabic/English resources.

The firmware default is mode `0` so existing installations keep their prior
angle-derived behavior until the user explicitly chooses fixed 90 minutes.

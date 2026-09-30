# Commercial OTA Status

Date: 2026-09-28

## Stable customer policy

`/updateMain` is the only customer-facing update path. It is already used by
the Android application and remains supported for every P10 generation,
including devices sold with older firmware. No device-version selection or
second OTA file is required from the customer.

The existing `/v2/update` implementation is retained in the source as an
optional laboratory/engineering path, but it is not part of the customer
workflow and must not replace `/updateMain` without a separately approved
migration plan.

## Release rule

Use the existing raw firmware asset for `/updateMain`. Do not append a
signature trailer to that asset: the legacy updater writes the complete upload
as firmware. `tools/release_ota.ps1` now builds the selected release profile
and copies the raw image to Android `assets/updateP104.bin`. Any future signed
OTA experiment must use a separate endpoint and asset and must not change the
stable Android flow.
## 2026-09-30 - Optional progress metadata

Android adds `size=<raw file bytes>` to `/updateMain`. It remains the same
legacy customer endpoint; old firmware ignores the extra query argument. New
firmware validates it and displays upload progress locally. Old applications
without the argument continue to work, displaying an indeterminate local
indicator. The raw asset is still unsigned; the existing stale signed manifest
does not match its size and is ignored by Android's current manifest loader.

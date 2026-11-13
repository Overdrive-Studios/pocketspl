# PocketSPL

Software for the PocketSPL sound level meter.

- `firmware/PocketSPL/PocketSPL.ino` — the meter's software
- `firmware/NOTES.txt` — what's new (shown on the phone when an update is offered)

## Releasing an update

1. In `PocketSPL.ino`, raise `FW_VERSION` (e.g. `"1.1.1"` → `"1.1.2"`).
2. Write what changed in `NOTES.txt`.
3. Commit and push to `main`.

GitHub builds it for both Feather boards and publishes it to
<https://overdrive-studios.github.io/pocketspl/latest.json> (takes about 5 minutes,
see the **Actions** tab). Phones then offer the update under **Settings → Software**.

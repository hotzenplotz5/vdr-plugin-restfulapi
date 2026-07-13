# Deleted recordings API

This extension exposes VDR's native deleted-recordings list for external clients without scanning `.del` directories directly.

It requires VDR 2.7.8 or newer.

## List deleted recordings

```http
GET /recordings/deleted.json
```

Optional query parameters:

- `start`: zero-based pagination offset
- `limit`: maximum number of returned recordings
- `marks=true`: include recording marks

The response uses the existing recordings JSON schema:

```json
{
  "recordings": [],
  "count": 0,
  "total": 0
}
```

The endpoint reads VDR's `DeletedRecordings` list while holding `LOCK_DELETEDRECORDINGS_READ`.

## Restore a deleted recording

```http
POST /recordings/deleted/restore
Content-Type: application/json

{
  "file": "/srv/vdr/video/Folder/Recording/2026-07-13.20.15.1-0.del"
}
```

The `file` value must:

- be the complete VDR recording path,
- end with `.del`,
- exactly identify an entry in VDR's `DeletedRecordings` list.

The endpoint does not accept a numeric recording index. It calls VDR's native `cRecording::Undelete()` implementation and only updates `DeletedRecordings` and `Recordings` after that operation succeeds.

Possible responses:

- `200`: recording restored
- `400`: missing or invalid `.del` path
- `404`: deleted recording not found
- `409`: VDR could not restore the recording, for example because the target `.rec` path already exists
- `501`: VDR is older than 2.7.8

## Deliberate non-goal

This first extension does not expose permanent deletion. A future purge endpoint must be reviewed and tested separately because it would call VDR's irreversible `cRecording::Remove()` operation.

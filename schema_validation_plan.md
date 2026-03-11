# Schema Validation — Implementation Plan

## Overview

Add per-collection schema validation to the Firebase panel. When a schema is defined for a collection, every document push (create or update) is validated client-side before the HTTP request is sent. Non-conforming documents are rejected with clear error messages in the activity log.

---

## 1. Schema Data Model

### Schema Definition (C++ struct in FirebasePanel.cpp)

```cpp
struct SchemaField {
    std::string name;       // field name, e.g. "player_name"
    int         type;       // 0 = String, 1 = Integer, 2 = Number, 3 = Boolean
    bool        required;   // if true, field must be present
};

struct CollectionSchema {
    std::vector<SchemaField> fields;
    bool strict;            // if true, extra fields not in schema are rejected
};

// Keyed by collection name
static std::unordered_map<std::string, CollectionSchema> schemas;
```

### Type Enum

| Value | Label     | JSON type        | Validation rule                          |
|-------|-----------|------------------|------------------------------------------|
| 0     | String    | `string`         | Must be a JSON string                    |
| 1     | Integer   | `number` (int)   | Must be a JSON number with no decimal    |
| 2     | Number    | `number` (float) | Must be any JSON number                  |
| 3     | Boolean   | `bool`           | Must be `true` or `false`                |

---

## 2. Schema Storage

Schemas are stored **in Firebase** under a reserved `_schemas` path so they sync across engine instances via Live Update.

```
RTDB root
├── _schemas/
│   ├── leaderboard.json    →  { "fields": {...}, "strict": true }
│   └── inventory.json      →  { "fields": {...}, "strict": false }
├── leaderboard/
│   ├── player1  { "player_name": "Alice", "score": 100, ... }
│   └── ...
```

### Firebase JSON format for a schema

```json
{
  "fields": {
    "player_name": { "type": "string",  "required": true },
    "score":       { "type": "integer", "required": true },
    "timestamp":   { "type": "string",  "required": true }
  },
  "strict": true
}
```

### Why Firebase (not local file)?

- Schemas are shared across all engine instances automatically via the existing Live Update (SSE root watch)
- No additional file I/O or local config paths needed
- Schema changes propagate in real-time to other connected instances
- `_schemas` is a reserved collection name — hidden from the Collections list in the UI

---

## 3. Schema Persistence — Load / Save

### Loading schemas

| When              | How                                                                |
|-------------------|--------------------------------------------------------------------|
| Panel first open  | `fetch_all_schemas()` — GET `_schemas.json`, parse into `schemas` map |
| Live Update event | SSE callback already refreshes collections; add a check for path starting with `_schemas` and re-fetch schemas |

```
static void fetch_all_schemas()
{
    Engine::Firebase::query_collection("_schemas",
        [](bool ok, const std::string& data) {
            if (!ok) return;
            // Parse data → populate schemas map
            // Each key = collection name, value = { fields: {...}, strict: bool }
        });
}
```

### Saving a schema

When user edits a schema in the UI and clicks "Save Schema":

```
Engine::Firebase::set_document("_schemas", collection_name, schema_json, callback);
```

### Deleting a schema

```
Engine::Firebase::delete_document("_schemas", collection_name, callback);
```

---

## 4. Validation Logic

### Core validation function

```cpp
// Returns empty string on success, or error message on failure.
static std::string validate_document(const std::string& collection,
                                     const std::string& json_data)
{
    auto it = schemas.find(collection);
    if (it == schemas.end())
        return "";  // No schema defined → always passes

    const CollectionSchema& schema = it->second;
    rapidjson::Document doc;
    doc.Parse(json_data.c_str());
    if (doc.HasParseError() || !doc.IsObject())
        return "Document is not a valid JSON object";

    // 1. Check required fields are present and types match
    for (const auto& sf : schema.fields) {
        auto member = doc.FindMember(sf.name.c_str());
        if (member == doc.MemberEnd()) {
            if (sf.required)
                return sf.name + " expected " + type_label(sf.type) + " but received null";
            continue;  // optional and absent → OK
        }
        std::string type_err = check_type(member->value, sf.type);
        if (!type_err.empty())
            return sf.name + " " + type_err;
    }

    // 2. If strict mode, reject any field not in the schema
    if (schema.strict) {
        for (auto m = doc.MemberBegin(); m != doc.MemberEnd(); ++m) {
            std::string key = m->name.GetString();
            bool found = false;
            for (const auto& sf : schema.fields)
                if (sf.name == key) { found = true; break; }
            if (!found)
                return key + " is an unknown field";
        }
    }

    return "";  // valid
}
```

### Type checking helper

```cpp
static std::string check_type(const rapidjson::Value& val, int expected_type)
{
    switch (expected_type) {
    case 0: // String
        if (!val.IsString()) return "expected string but received " + describe(val);
        break;
    case 1: // Integer
        if (!val.IsInt() && !val.IsInt64())
            return "expected integer but received " + describe(val);
        break;
    case 2: // Number
        if (!val.IsNumber()) return "expected number but received " + describe(val);
        break;
    case 3: // Boolean
        if (!val.IsBool()) return "expected boolean but received " + describe(val);
        break;
    }
    return "";
}
```

---

## 5. Integration Points (4 call sites)

Every `set_document` call in FirebasePanel.cpp must be guarded by `validate_document()`. There are **4** call sites:

| # | Location                   | Line (approx) | Context                            |
|---|----------------------------|---------------|------------------------------------|
| 1 | New Collection popup       | ~692          | Creating first doc in new collection |
| 2 | New Document popup         | ~884          | Creating a new doc in existing collection |
| 3 | Delete Field (re-save)     | ~978          | Saving doc after removing a field    |
| 4 | Update button              | ~1011         | Saving edited fields                 |

### Integration pattern (same for all 4 sites)

```cpp
// Before: immediately calls set_document
// After:  validate first, reject if invalid

std::string body = json_edit_buf;  // (already exists at each site)

std::string err = validate_document(col, body);
if (!err.empty()) {
    set_status("Schema violation: " + err, false);
    push_log("VALIDATE " + col + "/" + id, false, err);
    return;  // or skip the set_document call
}

// ... existing set_document call ...
```

---

## 6. UI — Schema Editor

### Location

New **collapsible section** between "Collections" and "Document Browser":

```
[Connection Status]
[Collections]
[Schema Editor]       ← NEW
[Document Browser]
[Activity Log]
```

### Layout

```
┌─ Schema Editor ──────────────────────────────────────────┐
│                                                          │
│  Collection: [leaderboard ▼]     [Delete Schema]         │
│                                                          │
│  ☑ Strict mode (reject unknown fields)                   │
│                                                          │
│  ┌─────────────────┬───────────┬──────────┬────────┐     │
│  │ Field Name      │ Type      │ Required │        │     │
│  ├─────────────────┼───────────┼──────────┼────────┤     │
│  │ player_name     │ String  ▼ │   ☑      │   x    │     │
│  │ score           │ Integer ▼ │   ☑      │   x    │     │
│  │ timestamp       │ String  ▼ │   ☑      │   x    │     │
│  │ [___________]   │ String  ▼ │   ☐      │        │ ← empty row for adding
│  └─────────────────┴───────────┴──────────┴────────┘     │
│                                                          │
│  [Save Schema]                                           │
│                                                          │
└──────────────────────────────────────────────────────────┘
```

### Schema Editor UI detail

1. **Collection dropdown** — lists all collections from `cached_collections` (excluding `_schemas`)
2. **Strict checkbox** — toggles `schema.strict`
3. **Field table** — editable rows:
   - Column 1: `InputText` for field name
   - Column 2: `Combo` dropdown (String / Integer / Number / Boolean)
   - Column 3: `Checkbox` for required
   - Column 4: `SmallButton("x")` to delete row
   - Last row is always an empty row for adding new fields (auto-appended)
4. **Save Schema** button — serializes `CollectionSchema` to JSON → `set_document("_schemas", col, json, cb)`
5. **Delete Schema** button — `delete_document("_schemas", col, cb)` → removes from `schemas` map
6. **Visual indicator** — In the Collections list, show a small "S" badge or icon next to collections that have a schema

### When collection is selected in the Collections list

- If a schema exists for it, the Schema Editor auto-loads those fields
- If no schema exists, the Schema Editor shows empty state with "No schema defined for this collection. Add fields below to create one."

---

## 7. Live Update Integration

The existing SSE root watch already fires on any DB change, including writes to `_schemas/`.

In the SSE callback (inside `start_live_watch`), add:

```cpp
// After refreshing collection list, also refresh schemas
fetch_all_schemas();
```

This ensures schema changes from one engine instance propagate to others in real-time.

---

## 8. _schemas Collection Hiding

The `_schemas` collection must be hidden from the user's collection list since it's an internal/system path.

### Changes

1. In `fetch_all_collections()` — filter out `"_schemas"` from `cached_collections`
2. In the Schema Editor — use a separate fetch for `_schemas` only

---

## 9. Error Messages

Match the rubric's expected error format:

| Scenario                  | Error message                                          |
|---------------------------|-------------------------------------------------------|
| Missing required field    | `score expected integer but received null`             |
| Wrong type                | `score expected integer but received string`           |
| Extra field (strict mode) | `cheats is an unknown field`                           |
| Invalid JSON              | `Document is not a valid JSON object`                  |

Error messages appear in:
- **Activity log** — red entry with operation `VALIDATE {collection}/{doc_id}`
- **Status bar** — red text "Schema violation: {message}"

---

## 10. File Changes Summary

| File                    | Changes                                                    |
|-------------------------|------------------------------------------------------------|
| `FirebasePanel.cpp`     | Add `SchemaField`, `CollectionSchema` structs              |
|                         | Add `schemas` map, `fetch_all_schemas()`, schema editor UI |
|                         | Add `validate_document()` + `check_type()` helpers         |
|                         | Guard all 4 `set_document` call sites with validation      |
|                         | Filter `_schemas` from collection list                     |
|                         | Add `fetch_all_schemas()` call to SSE callback             |
|                         | New ImGui collapsible section "Schema Editor"              |

No changes needed to `FirebaseManager`, `FirebaseHTTP`, `FirebaseListener`, or `FirebaseTypes` — validation is purely client-side in the panel, using the existing CRUD API.

---

## 11. Implementation Order

| Step | Task                                                    | Depends on |
|------|---------------------------------------------------------|------------|
| 1    | Add `SchemaField`, `CollectionSchema` structs + `schemas` map | —          |
| 2    | Add `validate_document()`, `check_type()`, `describe()` helpers | 1          |
| 3    | Add `fetch_all_schemas()` + `save_schema()` functions   | 1          |
| 4    | Filter `_schemas` from `cached_collections`             | 3          |
| 5    | Build Schema Editor UI section (ImGui)                  | 1, 3       |
| 6    | Guard all 4 `set_document` sites with validation        | 2          |
| 7    | Add `fetch_all_schemas()` to SSE callback + panel init  | 3          |
| 8    | Add schema indicator badge to collection list           | 1          |
| 9    | Test against rubric scenarios                           | All        |

---

## 12. Testing — Rubric Validation

### Test 1: Define schema
1. Open Firebase panel → Schema Editor
2. Select collection "leaderboard"
3. Add fields: `player_name` (String, required), `score` (Integer, required), `timestamp` (String, required)
4. Enable strict mode
5. Click "Save Schema"
6. **Verify**: Schema saved to Firebase under `_schemas/leaderboard`

### Test 2: Conforming document (rubric step 3-4)
Push `{"player_name": "Alice", "score": 100, "timestamp": "2026-03-10"}`
→ **Expected**: Accepted, activity log shows success

### Test 3: Missing required field (rubric step 5-6)
Push `{"player_name": "Bob", "timestamp": "2026-03-10"}`
→ **Expected**: Rejected, error: `score expected integer but received null`

### Test 4: Extra field in strict mode (rubric step 7-8)
Push `{"player_name": "Dave", "score": 50, "timestamp": "2026-03-10", "cheats": true}`
→ **Expected**: Rejected, error: `cheats is an unknown field`

### Test 5: Multi-instance sync
1. Define schema in Instance 1
2. Instance 2 receives schema via Live Update (SSE)
3. Instance 2 rejects invalid documents using the synced schema

---

## 13. Non-Goals (Out of Scope)

- Nested object / array schema validation (only flat fields)
- Server-side validation rules (Firebase Security Rules) 
- Schema versioning or migration
- Schema import/export from file

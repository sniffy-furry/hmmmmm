# Drop your MW field-name dictionaries here

This folder is the **ingest path for the vault naming wall**.

## The problem this solves

NFS:MW's ~747 vault schema-field names (and many collection keys) are `EA::Reflection`
strings **stripped at compile time**. In the retail build they exist only as
`lookup2`/`0xABCDEF00` hashes, and appear as a string in **no shipped file**. They can't be
brute-forced (>1.2M English combinations resolved ~2 names). The one file-side way through
is an **external dictionary** of candidate names.

## What to drop here

Any list of candidate attribute/field/collection **names**, in essentially any format:

- **VltEd / Binary / community field lists** (the NFS modding scene has published large ones)
- EA symbol dumps, header extracts, decompiler string dumps
- `hashlist.txt` style files (`0xHASH = Name`)
- CSV/TSV, XML, or just one name per line
- your own hand-written guesses

The resolver reads **every `A-Za-z_` token** on every line, so messy files are fine. It also
tries each token in `lower`, `UPPER`, `snake_case`, `UPPER_SNAKE`, and `CamelCase`.

If a file uses the `0xHASH = Name` form, the tool **cross-checks the source** — it re-hashes
the name and warns if the source's hash disagrees with `lookup2/0xABCDEF00` (catches wrong-seed
or wrong-hash-function lists so you don't trust bad data).

## Run it

From `MWTools/tools/`:

```bash
# dry run - see what a list would resolve, land nothing
python vlt_dict_resolve.py vlt_dictionaries/yourlist.txt

# ingest everything in this folder (dry run)
python vlt_dict_resolve.py --drop

# ingest and WRITE confirmed names into the encyclopedia data files
python vlt_dict_resolve.py --drop --apply
```

`--apply` writes matches into `RE-Data-And-Discoveries/data/vault_field_names.json` and flips the
`null` names in `vault_schema_census.json`. Matches are exact 32-bit hash equality, so a hit is a
proof, not a guess.

## After a successful ingest

Re-running the blocked Frontier items (F14 pursuit fields, F11 damage fields, the `aivehicle`
schema siblings, etc.) will now show named fields wherever the dictionary covered them — every
`data/*.json` that referenced a bare hash can be re-labelled in one pass.

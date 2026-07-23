#pragma once

namespace nfsmw {

// ─────────────────────────────────────────────────────────────────────────────
// KnownNames — seeds HashResolver with confirmed asset/event/class name
// strings collected from GLOBAL/attributes.bin (the master VPAK attribute
// vault's 'ErtS' string table). These are real, retail-confirmed strings —
// not guesses — covering surface/material classes, particle/VFX names,
// AI/gameplay system node names, pursuit-AI dispatch event names, smackable
// physics flags, and destructible scenery object classes.
//
// Without this, any hash the toolkit encounters that happens to match one of
// these known strings would still show up as an unresolved "0x0743CFB1"-style
// placeholder in HexViewPanel and the various browser panels, even though the
// name is already known. Call RegisterKnownNames() once at startup (see
// AppShell::Init, right after Logger::Init) so HashResolver::Resolve()
// returns readable names for these out of the box.
//
// This is a seed list, not a schema decoder — it only helps hash→name
// display. It does not by itself decode attributes.bin's class-record
// payloads (see VaultFile.h / VaultPackParser for that).
// ─────────────────────────────────────────────────────────────────────────────

/// Registers every known name below with HashResolver::Instance(). Safe to
/// call more than once (HashResolver::Register de-duplicates by hash).
/// Returns the number of names registered.
size_t RegisterKnownNames();

} // namespace nfsmw

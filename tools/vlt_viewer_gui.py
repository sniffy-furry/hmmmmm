#!/usr/bin/env python3
"""vlt_viewer_gui.py - Tkinter desktop browser for EA AttribSys VLT/VPAK
attributes.bin / *.bun files. All parsing is delegated to vlt_viewer.py;
this module only builds the UI and renders parsed results.

Launch:  python3 vlt_viewer_gui.py
"""

import io
import os
import struct
import sys
import traceback

try:
    import tkinter as tk
    from tkinter import filedialog, messagebox, ttk, font as tkfont
except Exception as e:  # pragma: no cover - depends on the host Python build
    print("This application requires tkinter, which is part of the Python "
          "standard library but was not available in this interpreter.\n"
          f"Import error: {e}\n"
          "On Debian/Ubuntu install it with: sudo apt-get install python3-tk")
    sys.exit(1)

import vlt_viewer as V


MONO = ("Courier New", 10)


def _mono_font(root):
    """Return a monospaced tkinter font, falling back gracefully."""
    for fam in ("Courier New", "DejaVu Sans Mono", "Menlo", "Consolas",
                "Liberation Mono", "Monospace", "Courier"):
        try:
            return tkfont.Font(root=root, family=fam, size=10)
        except Exception:
            continue
    return tkfont.Font(root=root, size=10)


def hexdump(data: bytes, base: int = 0) -> str:
    """Full hex dump (offset, 16 hex bytes, ASCII gutter)."""
    out = []
    for i in range(0, len(data), 16):
        chunk = data[i:i + 16]
        hexs = " ".join(f"{b:02X}" for b in chunk)
        asc = "".join(chr(b) if 32 <= b < 127 else "." for b in chunk)
        out.append(f"{base + i:08X}  {hexs:<47}  {asc}")
    if not out:
        out.append("(empty)")
    return "\n".join(out)


def tag4(cid: int) -> str:
    """Render a 4-byte chunk id as its ASCII tag (big-endian print order)."""
    b = struct.pack("<I", cid)
    return "".join(chr(c) if 32 <= c < 127 else "." for c in b)


class VltViewerApp:
    def __init__(self, root):
        self.root = root
        self.mono = _mono_font(root)
        self.file_path = None
        self.entries = []       # List[VaultPackEntry]
        self.vaults = []        # List[Optional[Vault]] aligned to entries
        # maps a tree item id -> ("kind", vault_index, payload)
        self.node_data = {}

        root.title("VLT Vault Viewer")
        root.geometry("1100x700")
        root.minsize(700, 400)

        self._build_menu()
        self._build_body()
        self._build_status()

    # ---- UI construction ------------------------------------------------
    def _build_menu(self):
        menubar = tk.Menu(self.root)
        filem = tk.Menu(menubar, tearoff=0)
        filem.add_command(label="Open...", accelerator="Ctrl+O",
                          command=self.open_file)
        filem.add_command(label="Save dump...", command=self.save_dump)
        filem.add_separator()
        filem.add_command(label="Exit", command=self.root.quit)
        menubar.add_cascade(label="File", menu=filem)

        helpm = tk.Menu(menubar, tearoff=0)
        helpm.add_command(label="About", command=self.show_about)
        menubar.add_cascade(label="Help", menu=helpm)

        self.root.config(menu=menubar)
        self.root.bind_all("<Control-o>", lambda e: self.open_file())

    def _build_body(self):
        toolbar = ttk.Frame(self.root)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=4, pady=4)
        ttk.Button(toolbar, text="Open File...",
                   command=self.open_file).pack(side=tk.LEFT)
        ttk.Button(toolbar, text="Save dump...",
                   command=self.save_dump).pack(side=tk.LEFT, padx=(4, 0))

        paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=4, pady=(0, 4))

        # Left: tree
        left = ttk.Frame(paned)
        self.tree = ttk.Treeview(left, show="tree", selectmode="browse")
        tsb = ttk.Scrollbar(left, orient=tk.VERTICAL, command=self.tree.yview)
        self.tree.configure(yscrollcommand=tsb.set)
        self.tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        tsb.pack(side=tk.RIGHT, fill=tk.Y)
        self.tree.bind("<<TreeviewSelect>>", self.on_select)
        paned.add(left, weight=1)

        # Right: detail area (container we repopulate per selection)
        self.detail = ttk.Frame(paned)
        paned.add(self.detail, weight=3)
        self._show_message("Open an attributes.bin / *.bun file to begin.")

    def _build_status(self):
        self.status = tk.StringVar(value="No file loaded.")
        bar = ttk.Label(self.root, textvariable=self.status,
                        relief=tk.SUNKEN, anchor=tk.W)
        bar.pack(side=tk.BOTTOM, fill=tk.X)

    # ---- detail-pane helpers -------------------------------------------
    def _clear_detail(self):
        for w in self.detail.winfo_children():
            w.destroy()

    def _show_message(self, text, monospace=False):
        self._clear_detail()
        txt = tk.Text(self.detail, wrap=tk.NONE,
                      font=self.mono if monospace else None)
        ysb = ttk.Scrollbar(self.detail, orient=tk.VERTICAL, command=txt.yview)
        xsb = ttk.Scrollbar(self.detail, orient=tk.HORIZONTAL, command=txt.xview)
        txt.configure(yscrollcommand=ysb.set, xscrollcommand=xsb.set)
        ysb.pack(side=tk.RIGHT, fill=tk.Y)
        xsb.pack(side=tk.BOTTOM, fill=tk.X)
        txt.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        txt.insert("1.0", text)
        txt.configure(state=tk.DISABLED)

    def _make_header(self, lines):
        """Top-of-detail multi-line label; returns the container frame."""
        frame = ttk.Frame(self.detail)
        frame.pack(side=tk.TOP, fill=tk.X, padx=6, pady=6)
        lbl = tk.Label(frame, justify=tk.LEFT, anchor=tk.W,
                       font=self.mono, text="\n".join(lines))
        lbl.pack(side=tk.LEFT, fill=tk.X)
        return frame

    def _make_table(self, columns, rows, widths=None):
        frame = ttk.Frame(self.detail)
        frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=6, pady=(0, 6))
        tv = ttk.Treeview(frame, columns=columns, show="headings")
        for i, col in enumerate(columns):
            tv.heading(col, text=col)
            w = (widths[i] if widths and i < len(widths) else 120)
            tv.column(col, width=w, anchor=tk.W, stretch=True)
        for row in rows:
            tv.insert("", tk.END, values=row)
        ysb = ttk.Scrollbar(frame, orient=tk.VERTICAL, command=tv.yview)
        xsb = ttk.Scrollbar(frame, orient=tk.HORIZONTAL, command=tv.xview)
        tv.configure(yscrollcommand=ysb.set, xscrollcommand=xsb.set)
        ysb.pack(side=tk.RIGHT, fill=tk.Y)
        xsb.pack(side=tk.BOTTOM, fill=tk.X)
        tv.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        return tv

    # ---- file open ------------------------------------------------------
    def open_file(self):
        path = filedialog.askopenfilename(
            title="Open attributes.bin / *.bun",
            filetypes=[("VPAK attribute files", "*.bin *.bun"),
                       ("BIN files", "*.bin"),
                       ("BUN files", "*.bun"),
                       ("All files", "*.*")])
        if not path:
            return
        self.load_path(path)

    def load_path(self, path):
        try:
            with open(path, "rb") as f:
                data = f.read()
        except OSError as e:
            messagebox.showerror("Open failed", f"Could not read file:\n{e}")
            return

        try:
            entries = V.parse_vpak(data)
        except Exception:
            self._report_error("Failed to parse VPAK container", path)
            return

        vaults = []
        errors = []
        for i, e in enumerate(entries):
            try:
                vaults.append(V.load_vault(e))
            except Exception:
                vaults.append(None)
                errors.append(f"[{i}] {e.name}:\n"
                              + traceback.format_exc())

        self.file_path = path
        self.entries = entries
        self.vaults = vaults
        self._populate_tree()
        self.status.set(f"{path}    |    {len(entries)} vault(s)")

        if errors:
            self._report_error(
                "Some vaults failed to load (others loaded OK)", path,
                extra="\n\n".join(errors))
        else:
            self._show_message(
                f"Loaded {len(entries)} vault(s) from:\n{path}\n\n"
                "Select a node in the tree on the left to inspect it.")

    def _report_error(self, title, path, extra=None):
        tb = extra if extra is not None else traceback.format_exc()
        body = f"{title}\n\nFile: {path}\n\n{tb}"
        # Show the full, copyable traceback in the detail pane (persistent).
        self._show_message(body, monospace=True)
        self.status.set(f"{title}  --  {path}")
        # Also pop up a brief, non-blocking notification so the error is
        # noticed even if the detail pane is off-screen.
        messagebox.showerror(title, f"{title}\n\nFile: {path}\n\n"
                                  "Details are shown in the right-hand pane.")

    # ---- tree population -----------------------------------------------
    def _populate_tree(self):
        self.tree.delete(*self.tree.get_children())
        self.node_data.clear()

        for vi, (entry, v) in enumerate(zip(self.entries, self.vaults)):
            label = f"[{vi}] {entry.name}"
            if v is None:
                label += "  (load failed)"
            vault_node = self.tree.insert("", tk.END, text=label, open=(vi == 0))
            self.node_data[vault_node] = ("vault", vi, None)
            if v is None:
                continue

            def add_group(gtext, kind, items, child_kind):
                gnode = self.tree.insert(vault_node, tk.END,
                                         text=f"{gtext} ({len(items)})")
                self.node_data[gnode] = (kind, vi, None)
                for idx, it in enumerate(items):
                    cnode = self.tree.insert(gnode, tk.END,
                                             text=self._child_label(child_kind, v, idx, it))
                    self.node_data[cnode] = (child_kind, vi, idx)
                return gnode

            add_group("Classes", "classes", v.classes, "class")
            add_group("Collections", "collections", v.collections, "collection")
            add_group("Databases", "databases", v.databases, "database")

            snode = self.tree.insert(vault_node, tk.END,
                                     text=f"Strings ({len(v.strings)})")
            self.node_data[snode] = ("strings", vi, None)

            add_group("Chunks (VLT)", "vlt_chunks", v.chunks, "vlt_chunk")
            add_group("Chunks (BIN)", "bin_chunks", v.bin_chunks, "bin_chunk")

    def _child_label(self, kind, v, idx, item):
        if kind == "class":
            return V.name(v, item.name_hash)
        if kind == "collection":
            return V.name(v, item.name_hash)
        if kind == "database":
            return f"database[{idx}] count={item.count}"
        if kind in ("vlt_chunk", "bin_chunk"):
            nm = V.CHUNK_NAMES.get(item.cid, "Unknown")
            return f"{nm} '{tag4(item.cid)}' @0x{item.offset:X}"
        return str(idx)

    # ---- selection dispatch --------------------------------------------
    def on_select(self, _event):
        sel = self.tree.selection()
        if not sel:
            return
        info = self.node_data.get(sel[0])
        if not info:
            return
        kind, vi, idx = info
        v = self.vaults[vi] if 0 <= vi < len(self.vaults) else None

        if kind == "vault":
            self._show_vault(vi, v)
        elif kind in ("classes", "collections", "databases",
                      "vlt_chunks", "bin_chunks"):
            self._show_group_overview(kind, v)
        elif kind == "class":
            self._show_class(v, v.classes[idx])
        elif kind == "collection":
            self._show_collection(v, v.collections[idx])
        elif kind == "database":
            self._show_database(v, v.databases[idx], idx)
        elif kind == "strings":
            self._show_strings(v)
        elif kind == "vlt_chunk":
            self._show_chunk(v, v.chunks[idx], "VLT")
        elif kind == "bin_chunk":
            self._show_chunk(v, v.bin_chunks[idx], "BIN")

    # ---- detail renderers ----------------------------------------------
    def _show_vault(self, vi, v):
        if v is None:
            self._show_message(f"Vault [{vi}] failed to load. See error dialog.")
            return
        e = v.entry
        lines = [
            f"Vault [{vi}] {e.name}",
            f"VLT version : 0x{v.version:016X}",
            f"VLT block   : offset 0x{e.vlt_offset:08X}  size 0x{e.vlt_size:X} ({e.vlt_size} bytes)",
            f"BIN block   : offset 0x{e.bin_offset:08X}  size 0x{e.bin_size:X} ({e.bin_size} bytes)",
            f"Dependencies: {', '.join(d for d in v.dependencies if d) or '(none)'}",
            "",
            f"Strings={len(v.strings)}  Exports={len(v.exports)}  "
            f"Classes={len(v.classes)}  Collections={len(v.collections)}  "
            f"Databases={len(v.databases)}",
            f"Pointers: vlt-fixups={len(v.vlt_fixups)}  raw-fixups={len(v.raw_fixups)}",
        ]
        self._show_message("\n".join(lines), monospace=True)

    def _show_group_overview(self, kind, v):
        if v is None:
            self._show_message("Vault failed to load.")
            return
        labels = {
            "classes": ("Classes", v.classes),
            "collections": ("Collections", v.collections),
            "databases": ("Databases", v.databases),
            "vlt_chunks": ("VLT Chunks", v.chunks),
            "bin_chunks": ("BIN Chunks", v.bin_chunks),
        }
        title, items = labels[kind]
        self._show_message(f"{title}: {len(items)}\n\n"
                           "Expand this node in the tree to inspect items.")

    def _show_class(self, v, cd):
        self._clear_detail()
        self._make_header([
            f"Class: {V.name(v, cd.name_hash)}   (hash 0x{cd.name_hash:08X})",
            f"baseSize=0x{cd.base_size:X} ({cd.base_size})   "
            f"collectionReserve={cd.collection_reserve}   "
            f"numDefinitions={cd.num_definitions}   required={cd.required_count}",
        ])
        cols = ("Name", "Type", "Flags", "Size", "Offset", "MaxCount", "Alignment")
        rows = []
        for f in cd.fields:
            rows.append((
                V.name(v, f.key), V.name(v, f.type_key), V.fmt_flags(f.flags),
                f.size, f.offset, f.max_count, f.alignment))
        self._make_table(cols, rows,
                         widths=(220, 220, 150, 60, 70, 80, 90))

    def _show_collection(self, v, coll):
        self._clear_detail()
        cls = v.class_by_hash.get(coll.class_hash)
        self._make_header([
            f"Collection: {V.name(v, coll.name_hash)}   (hash 0x{coll.name_hash:08X})",
            f"class={V.name(v, coll.class_hash)}   parent={V.name(v, coll.parent_hash)}",
            f"tableReserve={coll.table_reserve}   entries={coll.num_entries}   "
            f"types={coll.num_types}",
            f"types: {', '.join(V.name(v, t) for t in coll.types) or '(none)'}",
        ])
        fmap = {f.key: f for f in cls.fields} if cls else {}
        cols = ("Key", "TypeIndex", "NodeFlags", "InlineData")
        rows = []
        for ent in coll.entries:
            rows.append((
                V.name(v, ent.key),
                ent.type_index,
                V.fmt_nodeflags(ent.node_flags),
                ent.data4.hex(" ")))
        self._make_table(cols, rows, widths=(260, 90, 140, 160))

    def _show_database(self, v, db, idx):
        lines = [
            f"Database record [{idx}]",
            f"num1  = {db.num1}",
            f"num2  = {db.num2}",
            f"count = {db.count}",
            "",
            "sizes:",
        ]
        for i, s in enumerate(db.sizes):
            lines.append(f"  [{i:3d}] {s}  (0x{s:X})")
        self._show_message("\n".join(lines), monospace=True)

    def _show_strings(self, v):
        self._clear_detail()
        self._make_header([f"BIN strings: {len(v.strings)}"])
        cols = ("Hash", "String")
        rows = [(f"0x{V.vlt32(s):08X}", s) for s in v.strings]
        self._make_table(cols, rows, widths=(120, 600))

    def _show_chunk(self, v, c, block):
        lines = [
            f"{block} Chunk: {V.CHUNK_NAMES.get(c.cid, 'Unknown')}",
            f"id     = '{tag4(c.cid)}'  (0x{c.cid:08X})",
            f"size   = 0x{c.size:X} ({c.size} bytes)",
            f"offset = 0x{c.offset:X} (within {block} block)",
            "",
            hexdump(c.data),
        ]
        self._show_message("\n".join(lines), monospace=True)

    # ---- save dump ------------------------------------------------------
    def save_dump(self):
        sel = self.tree.selection()
        vi = None
        if sel:
            info = self.node_data.get(sel[0])
            if info:
                vi = info[1]
        if vi is None:
            if len(self.vaults) == 1:
                vi = 0
            else:
                messagebox.showinfo(
                    "Save dump",
                    "Select a vault (or a node within it) in the tree first.")
                return
        v = self.vaults[vi]
        if v is None:
            messagebox.showerror("Save dump", "That vault failed to load.")
            return

        default = f"{self.entries[vi].name or 'vault'}_dump.txt"
        path = filedialog.asksaveasfilename(
            title="Save vault dump",
            defaultextension=".txt",
            initialfile=default,
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")])
        if not path:
            return
        try:
            with open(path, "w", encoding="utf-8") as f:
                f.write(self._build_dump(vi, v))
        except OSError as e:
            messagebox.showerror("Save failed", str(e))
            return
        self.status.set(f"Dump saved: {path}")
        messagebox.showinfo("Save dump", f"Saved vault dump to:\n{path}")

    def _build_dump(self, vi, v):
        buf = io.StringIO()
        old = sys.stdout
        try:
            sys.stdout = buf
            e = v.entry
            print(f"=== Vault [{vi}] {e.name} ===")
            print(f"VLT version: 0x{v.version:016X}")
            if v.dependencies:
                print(f"Dependencies: {', '.join(d for d in v.dependencies if d)}")
            print(f"Strings: {len(v.strings)}   Exports: {len(v.exports)}   "
                  f"Classes: {len(v.classes)}   Collections: {len(v.collections)}   "
                  f"Databases: {len(v.databases)}")
            print()
            V.print_chunks(v)
            print()
            V.print_classes(v)
            V.print_collections(v)
            V.print_databases(v)
            print()
            print("Strings (BIN block):")
            for s in v.strings:
                print(f"  0x{V.vlt32(s):08X}  {s}")
        finally:
            sys.stdout = old
        return buf.getvalue()

    # ---- about ----------------------------------------------------------
    def show_about(self):
        messagebox.showinfo(
            "About VLT Vault Viewer",
            "VLT Vault Viewer\n\n"
            "A tkinter front-end for inspecting EA AttribSys VPAK-packed\n"
            "attributes.bin / *.bun files (Black-Box era Need for Speed).\n\n"
            "All parsing is provided by vlt_viewer.py.")


def main():
    root = tk.Tk()
    app = VltViewerApp(root)
    if len(sys.argv) > 1 and os.path.isfile(sys.argv[1]):
        app.load_path(sys.argv[1])
    root.mainloop()


if __name__ == "__main__":
    main()

#!/usr/bin/env python3
"""Generates code/savelayout_v1.inc: how to convert save game structures written by 64-bit Linux
builds before the save format fix (commit 8c51099, where these structures had 'long' members,
8 bytes on LP64) to the fixed layout (int32_t members, identical on all platforms).

It does not hand-copy struct definitions. Instead:
  1. the field paths of each structure are taken from clang's record layout dump of the new code,
  2. a small program printing offsetof()/sizeof() of every path is compiled against the old code
     (a git worktree at the old commit) and against the new code,
  3. fields are paired by path: same size -> copy, 8 -> 4 bytes -> narrow (int64 to int32).
The old program also writes sample structures (deterministic patterns per field) and the new
program the expected converted result; tests/test_save_conversion.cpp checks the converter
against them.

usage: gen_save_layout.py <new source dir> <old source dir> <compile_commands.json of a build>
                          <output .inc> <fixture dir>
"""
import json, os, re, shlex, subprocess, sys, tempfile

STRUCTS = ["MoverData", "MechData", "GroundVehicleData", "WeaponBoltData", "TeamData",
           "TeamStaticData", "StaticCommanderData", "CommanderData", "GateData"]
HEADERS = ["mclib.h", "objmgr.h", "weaponbolt.h", "mover.h", "mech.h", "gvehicl.h", "team.h",
           "comndr.h", "gate.h"]
# fields renamed by the format fix: new name -> old name
RENAMED = {"closestObject_unused": "closestObject"}


def compile_flags(compile_commands, src_dir):
    cc = json.load(open(compile_commands))
    entry = [e for e in cc if e["file"].endswith("code/objmgr.cpp")][0]
    args = shlex.split(entry["command"])
    flags = []
    for a in args:
        if a.startswith(("-D", "-isystem", "-std")) or a == "/usr/include/SDL2":
            flags.append(a)
        elif a.startswith("-I"):
            # point include dirs at the requested source tree
            path = a[2:]
            new_root = os.path.abspath(sys.argv[1])
            if path.startswith(new_root):
                path = src_dir + path[len(new_root):]
            flags.append("-I" + path)
    return flags


def dump_layouts(src_dir, flags):
    with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False) as f:
        for h in HEADERS:
            f.write('#include "%s"\n' % h)
        for i, s in enumerate(STRUCTS):
            f.write("%s g_layout_%d;\n" % (s, i))
        name = f.name
    out = subprocess.run(["clang++"] + flags + ["-w", "-fsyntax-only", "-Xclang", "-fdump-record-layouts", name],
                         capture_output=True, text=True, check=True).stdout
    os.unlink(name)
    records = {}
    for block in out.split("*** Dumping AST Record Layout")[1:]:
        lines = [l for l in block.strip("\n").split("\n") if l.strip()]
        rec_name = lines[0].split("|", 1)[1].strip()
        fields = []
        for l in lines[1:]:
            if "|" not in l or "sizeof=" in l or "nvsize=" in l:
                continue
            off, rest = l.split("|", 1)
            depth = (len(rest) - len(rest.lstrip(" "))) // 2
            text = rest.strip()
            fields.append((int(off.split(":")[0]), depth, text))
        records[rec_name.split()[-1]] = fields
    return records


def record_for(records, type_name):
    for key in (type_name, "_" + type_name):
        if key in records:
            return records[key]
    return None


def leaf_paths(records, rec, depth0=1):
    """Field paths below a record (expanding nested records and arrays of records)."""
    paths = []
    fields = records[rec] if isinstance(rec, str) else rec
    i = 0
    stack = []  # (depth, path prefix)
    for idx, (off, depth, text) in enumerate(fields):
        if "vtable pointer" in text:
            continue
        while stack and stack[-1][0] >= depth:
            stack.pop()
        prefix = stack[-1][1] if stack else ""
        if text.endswith("(base)"):
            stack.append((depth, prefix))  # base members are accessed directly
            continue
        m = re.match(r"(.*?)\s+(\w+)$", text)
        if not m:
            continue
        ftype, fname = m.group(1).strip(), m.group(2)
        path = prefix + fname
        next_depth = fields[idx + 1][1] if idx + 1 < len(fields) else -1
        if next_depth > depth:
            stack.append((depth, path + "."))  # record expanded inline
            continue
        am = re.match(r"(.*?)\[(\d+)\]$", ftype)
        if am:
            elem, count = am.group(1).strip(), int(am.group(2))
            elem_name = elem.replace("struct ", "").replace("class ", "").strip()
            sub = record_for(records, elem_name)
            if sub is not None:
                for k in range(count):
                    for p, t in leaf_paths(records, sub):
                        paths.append(("%s[%d].%s" % (path, k, p), t))
                continue
            if elem in ("long", "unsigned long", "int32_t", "uint32_t", "int", "unsigned int"):
                # integer arrays element by element, their element size may have changed
                paths.extend(("%s[%d]" % (path, k), elem) for k in range(count))
                continue
        paths.append((path, ftype))
    return paths


def measure(src_dir, flags, paths_by_struct, old, fixture_path):
    """offset/size of every path, and a sample of each struct filled with per-field patterns."""
    lines = ['#include "%s"' % h for h in HEADERS]
    lines += ["#include <cstdio>", "#include <cstring>", "#include <cstdint>", "#include <cstddef>",
              "template<class T> static void fill(T& v, unsigned seed) {",
              "  unsigned char* p = (unsigned char*)&v;",
              "  for (size_t i = 0; i < sizeof(T); ++i) p[i] = (unsigned char)(seed * 31u + i * 7u + 1u); }",
              "template<class T> static void fill_int(T& v, unsigned seed) { v = (T)(int32_t)(seed * 2654435761u % 2000000000u) - 1000000000; }",
              "int main() {", "  FILE* bin = fopen(\"%s\", \"wb\");" % fixture_path]
    for s, paths in paths_by_struct.items():
        # raw storage: constructing the structs would need game code (vtables of members)
        lines.append("  { alignas(%s) static unsigned char buf[sizeof(%s)]; memset(buf, 0, sizeof(buf)); %s& v = *reinterpret_cast<%s*>(buf);" % (s, s, s, s))
        for i, (p, is_int) in enumerate(paths):
            q = p
            if old:
                for new_name, old_name in RENAMED.items():
                    q = q.replace(new_name, old_name)
            lines.append('    printf("%s %s %%zu %%zu\\n", (size_t)offsetof(%s, %s), sizeof(v.%s));' % (s, p, s, q, q))
            fn = "fill_int" if is_int else "fill"
            lines.append("    %s(v.%s, %du);" % (fn, q, i + 1))
        lines.append("    fwrite(&v, sizeof(v), 1, bin); }")
    lines += ["  fclose(bin); return 0; }"]
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "measure.cpp")
        open(src, "w").write("\n".join(lines))
        exe = os.path.join(d, "measure")
        subprocess.run(["g++"] + flags + ["-w", "-Wno-invalid-offsetof", src, "-o", exe], check=True)
        out = subprocess.run([exe], capture_output=True, text=True, check=True).stdout
    result = {}
    for l in out.split("\n"):
        if l.strip():
            s, p, off, size = l.split()
            result[(s, p)] = (int(off), int(size))
    return result


def main():
    new_dir, old_dir, ccj, out_inc, fixture_dir = sys.argv[1:6]
    new_flags = compile_flags(ccj, os.path.abspath(new_dir))
    old_flags = compile_flags(ccj, os.path.abspath(old_dir))
    old_records = dump_layouts(old_dir, old_flags)
    new_records = dump_layouts(new_dir, new_flags)

    paths_by_struct = {}
    for s in STRUCTS:
        new_paths = [p for p, t in leaf_paths(new_records, record_for(new_records, s))]
        old_types = {}
        for p, t in leaf_paths(old_records, record_for(old_records, s)):
            for new_name, old_name in RENAMED.items():
                p = p.replace(old_name, new_name)
            old_types[p] = t
        missing = [p for p in new_paths if p not in old_types]
        if missing:
            raise SystemExit("%s: fields not in the old layout: %s" % (s, missing[:5]))
        # fields that were 'long' get integer test patterns that fit in 32 bits (they are narrowed)
        paths_by_struct[s] = [(p, old_types[p] in ("long", "unsigned long")) for p in new_paths]

    os.makedirs(fixture_dir, exist_ok=True)
    old_m = measure(old_dir, old_flags, paths_by_struct, True, os.path.join(fixture_dir, "save_v0_linux64.bin"))
    new_m = measure(new_dir, new_flags, paths_by_struct, False, os.path.join(fixture_dir, "save_v1_expected.bin"))

    out = ["// Generated by tools/save_layout/gen_save_layout.py, do not edit.",
           "// Conversion of save game structures from the old 64-bit Linux layout ('long' members, before",
           "// the save format fix) to the current layout. Ops: copy 'size' bytes, or narrow an 8 byte",
           "// integer to 4 bytes.", ""]
    for s in STRUCTS:
        ops = []
        for (p, is_int) in paths_by_struct[s]:
            o_off, o_size = old_m[(s, p)]
            n_off, n_size = new_m[(s, p)]
            if o_size == n_size:
                kind = 0
            elif o_size == 8 and n_size == 4:
                kind = 1
            else:
                raise SystemExit("%s.%s: size %d -> %d not supported" % (s, p, o_size, n_size))
            ops.append([o_off, n_off, n_size, kind, p])
        # merge adjacent copies; drop fully overlapping duplicates (unions)
        ops.sort(key=lambda o: (o[0], -o[2]))
        merged = []
        for o in ops:
            if merged:
                m = merged[-1]
                if o[3] == 0 and m[3] == 0 and m[0] - m[1] == o[0] - o[1] and o[0] <= m[0] + m[2]:
                    m[2] = max(m[2], o[0] + o[2] - m[0])
                    continue
            merged.append(o)
        old_size = max(v[0] + v[1] for k, v in old_m.items() if k[0] == s)
        out.append("// %s" % s)
        out.append("static const SaveLayoutOp k%sOps[] = {" % s)
        for o in merged:
            out.append("    { %d, %d, %d, %d }, // %s" % (o[0], o[1], o[2], o[3], o[4]))
        out.append("};")
        out.append("// new offsets, checked on every platform")
        for o in merged:
            path = o[4]
            out.append("static_assert(offsetof(%s, %s) == %d, \"%s.%s moved\");" % (s, path, o[1], s, path))
        out.append("")
    out.append("static const SaveLayout kSaveLayoutsV0[] = {")
    for s in STRUCTS:
        old_sz = [l for l in old_records.get("_" + s, [])]
        out.append("    { \"%s\", SAVE_OLD_SIZE_%s, sizeof(%s), k%sOps, sizeof(k%sOps) / sizeof(k%sOps[0]) }," % (s, s, s, s, s, s))
    out.append("};")
    # old sizes from the old tree
    sizes = []
    with tempfile.TemporaryDirectory() as d:
        src = os.path.join(d, "sizes.cpp")
        open(src, "w").write("\n".join(['#include "%s"' % h for h in HEADERS] + ["#include <cstdio>", "int main() {"] +
                                       ['printf("%s %%zu\\n", sizeof(%s));' % (s, s) for s in STRUCTS] + ["}"]))
        subprocess.run(["g++"] + old_flags + ["-w", src, "-o", os.path.join(d, "sizes")], check=True)
        for l in subprocess.run([os.path.join(d, "sizes")], capture_output=True, text=True, check=True).stdout.split("\n"):
            if l.strip():
                s, n = l.split()
                sizes.append("#define SAVE_OLD_SIZE_%s %s" % (s, n))
    open(out_inc, "w").write("\n".join(out[:5] + sizes + [""] + out[5:]) + "\n")


if __name__ == "__main__":
    main()

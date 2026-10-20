# Save game layout conversion

Save games store some structures as raw bytes. Until the save format fix they contained `long`
members, 8 bytes on 64-bit Linux and 4 on Windows, so saves were not portable. They now use
`int32_t` (identical everywhere), and `code/savegame.cpp` converts packets written by older 64-bit
Linux builds when loading (recognised by their size). Windows saves did not change: `long` was
already 4 bytes there.

`code/savelayout_v1.inc` (the conversion table) and the test fixtures in `tests/data/` are generated
by `gen_save_layout.py` from the old code (commit 8c51099) and the current code:

```
git worktree add /tmp/mc2-old 8c51099
cmake --preset relwithdebinfo
python3 tools/save_layout/gen_save_layout.py . /tmp/mc2-old build/relwithdebinfo/compile_commands.json \
    code/savelayout_v1.inc tests/data
```

It needs clang (for `-fdump-record-layouts`) and g++. The field paths come from clang's layout
dump; offsets and sizes come from compiling `offsetof`/`sizeof` against both trees, so nothing is
copied by hand. The generated `static_assert`s pin the new offsets on every platform CI builds.

If one of these structures changes again, the save format changes: add a new conversion step
rather than editing the generated table.

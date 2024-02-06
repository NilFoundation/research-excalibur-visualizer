A simple circuit visualizer for Placeholder proof system.

# Building and running
0. `git clone --recurse-submodules https://github.com/NilFoundation/research-excalibur-visualizer.git`
1. `cd research-excalibur-visualizer/libs/crypto3`
2. Add blueprint to crypto3: `git submodule add https://github.com/NilFoundation/zkllvm-blueprint.git libs/blueprint`
3. Install [nix](https://nixos.org/download.html).
4. Run `nix-shell` inside excalibur repository directory.
5. `mkdir build && cd build`
6. `cmake ../. -DCMAKE_CXX_STANDARD=17 -DBUILD_SHARED_LIBS=TRUE -DBUILD_TESTS=TRUE -DBUILD_EXAMPLES=TRUE -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER="clang++" -DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_FLAGS="-Wall" -DLLDB_EXPORT_ALL_SYMBOLS=ON`
7. `make excalibur`
8. Additionally, you need to compile the GTK schema used in the dialog, and link the environment variable. In order to do that, find you gtk installation in `/nix/store/`.

The best way of doing this is via `nix-locate org.gtk.gtk4.Settings.FileChooser.gschema.xml` if you have [nix-index](https://github.com/nix-community/nix-index) installed.

Alternatively, run `schema-locator.sh`. If you have multiple versions of gtk4 in the store, use the latest one.

After finding the file, run `glib-compile-schemas --targetdir=. /path/to/schema/directory`.
Note that you need to pass *directory*, and not the file path.
This should create `gschemas.compiled` file in current (build) directory.

9. Export the `gschemas.compiled` directory via ` export GSETTINGS_SCHEMA_DIR=/path/to/compiled/schema/dir`.
10. Run `./src/excalibur --vesta` (or `--pallas`).
11. (Optional) If you are using VSCode for development, you might be interested in installing the [Nix Environment Selector](https://marketplace.visualstudio.com/items?itemName=arrterian.nix-env-selector) extension.

# FAQ
I get the following error while running the tool:
```
(excalibur:24987): GLib-GIO-ERROR **: 18:39:52.016: Settings schema 'org.gtk.gtk4.Settings.FileChooser' is not installed
Trace/breakpoint trap
```
Check that you've done steps 8-9 above. Export has to be redone each shell session, unless you modify `.bashrc` or do something similar.

How do I export my circuit/assignment table?

Use `export_table` (defined in `include/nil/blueprint/blueprint/plonk/assignment.hpp`) for exporting the assignment table, and `export_circuit` (defined in in `include/nil/blueprint/blueprint/plonk/circuit.hpp`) for circuit export.
A good place to call the export functions might be `test_plonk_component.hpp`.

There currently is no compiler integration.

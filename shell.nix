let
  sources = import ./nix/sources.nix;
  pkgs = import sources.nixpkgs {};
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    niv
    cmake
    boost
    pkg-config
    gtk4
    gtkmm4
    glibmm
    pcre2
    glib
    pango
    pangomm
    util-linux
    xorg.libXdmcp
    libselinux
    libsepol
    pcre
    fribidi
    libthai
    libdatrie
    libdeflate
    wrapGAppsHook4
    gdb
    valgrind
    libsForQt5.kcachegrind
    linuxKernel.packages.linux_zen.perf
    clang-tools_16
  ] ++ (with pkgs.llvmPackages_16; [
    clang
  ]);

  shellHook = ''
    export NO_AT_BRIDGE="1"
  '';
}
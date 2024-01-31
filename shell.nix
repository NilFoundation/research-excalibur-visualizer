let
  sources = import ./nix/sources.nix;
  pkgs = import sources.nixpkgs {};
in
pkgs.mkShell {
  buildInputs = with pkgs; [
    niv
    git
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
    pcre
    fribidi
    libthai
    libdatrie
    libdeflate
    wrapGAppsHook4
    lldb
    #valgrind
    #libsForQt5.kcachegrind
    #linuxKernel.packages.linux_zen.perf
    clang-tools
    clang
  ];

  shellHook = ''
    export NO_AT_BRIDGE="1"
    function nil_test_runner() {
      clear
      filename=$(cat Makefile | grep "$2" | awk 'NR==1{print $NF}')
      make -j$(nproc) "$filename" && ./libs/crypto3/libs/$1/test/$filename
    }
    function ctcmp() {
      nil_test_runner blueprint $1
    }
    function ztcmp() {
      nil_test_runner zk $1
    }
    function mtcmp() {
      nil_test_runner math $1
    }
  '';
}
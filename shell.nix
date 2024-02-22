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
    clang-tools_17
    clang_17
    gcc13
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
    function zmtcmp() {
      nil_test_runner marshalling/zk $1
    }
    function atcmp() {
      nil_test_runner algebra $1
    }
    function htcmp() {
      nil_test_runner hash $1
    }
    function cotcmp() {
      nil_test_runner container $1
    }
    function rtcmp() {
      nil_test_runner random $1
    }
    function mctcmp() {
      nil_test_runner marshalling/core $1
    }
  '';
}
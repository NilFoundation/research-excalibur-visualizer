let
  sources = import ./nix/sources.nix;
  pkgs = import sources.nixpkgs {};
in
pkgs.mkShell {
  buildInputs = [
    pkgs.niv
    pkgs.clang
    pkgs.cmake
    pkgs.boost
    pkgs.pkg-config
    pkgs.gtk4
    pkgs.gtkmm4
    pkgs.glibmm
    pkgs.pcre2
    pkgs.glib
    pkgs.pango
    pkgs.pangomm
    pkgs.util-linux
    pkgs.xorg.libXdmcp
    pkgs.libselinux
    pkgs.libsepol
    pkgs.pcre
    pkgs.fribidi
    pkgs.libthai
    pkgs.libdatrie
    pkgs.libdeflate
    pkgs.wrapGAppsHook4
  ];

  shellHook = ''
    export NO_AT_BRIDGE="1"
  '';
}
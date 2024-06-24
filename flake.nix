{
  description = "Nix flake for excalibur";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
    nil_crypto3 = {
      url = "github:NilFoundation/crypto3";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, nil_crypto3, ... }:
    (flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs { inherit system; };
        stdenv = pkgs.llvmPackages_17.stdenv;
        crypto3 = nil_crypto3.packages.${system}.default;
      in rec {
        devShells = {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              cmake
              pkg-config
              boost
              clang
              clang-tools
              gtk4
              gtkmm4
              glibmm
              pcre2
              glib
              pango
              pangomm
              crypto3
            ];

            shellHook = ''
              export NO_AT_BRIDGE="1"
              echo "Excalibur dev environment activated"
            '';
          };
        };
      }));
}

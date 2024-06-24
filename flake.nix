{
  description = "Nix flake for excalibur";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-24.05";
    flake-utils = {
      url = "github:numtide/flake-utils";
    };
    nix_3rdparty = {
      url = "github:NilFoundation/nix-3rdparty";
      inputs = {
        nixpkgs.follows = "nixpkgs";
        flake-utils.follows = "flake-utils";
      };
    };
    nil_crypto3 = {
      url = "github:NilFoundation/crypto3";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, flake-utils, nil_crypto3, nix_3rdparty, ... }:
    (flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = import nixpkgs {
          overlays = [ nix_3rdparty.overlays.${system}.default ];
          inherit system;
        };
        stdenv = pkgs.llvmPackages_17.stdenv;
        crypto3 = nil_crypto3.packages.${system}.default;
      in rec {
        devShells = {
          default = pkgs.mkShell {
            buildInputs = with pkgs; [
              cmake
              cmake_modules
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
              echo "excalibur dev environment activated"
            '';
          };
        };
      }));
}

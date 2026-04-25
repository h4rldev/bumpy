{
  description = "A very basic flake";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = {
    self,
    nixpkgs,
    flake-utils,
  }:
    flake-utils.lib.eachDefaultSystem (system: let
      pkgs = nixpkgs.legacyPackages.${system};
    in {
      packages.default = pkgs.stdenv.mkDerivation {
        pname = "bumpy";
        version = "0.1.0";

        src = ./.;

        nativeBuildInputs = with pkgs; [just gcc mold];

        buildPhase = ''
          runHook preBuild
          sed -i 's|#!/usr/bin/env bash|#!${pkgs.bash}/bin/bash|' justfile
          just build release
          runHook postBuild
        '';

        installPhase = ''
          runHook preInstall
          mkdir -p $out/lib
          mkdir -p $out/include

          cp lib/libbumpy.so $out/lib
          cp include/bumpy.h $out/include
          runHook postInstall
        '';
      };
      devShells.default = import ./nix/shell.nix {inherit pkgs;};
    });
}

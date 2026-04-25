{pkgs ? import <nixpkgs> {}}:
with pkgs;
  mkShell {
    name = "bumpy";
    description = "A simple and extendable Windows and Linux compatible bump-allocator / arena allocator.";

    buildInputs = [
      # Utilities
      pkg-config # For finding libraries
      mold # For linking
    ];

    packages = [
      # Language servers, and formatters
      clang-tools
      doctoc
      prettierd

      # Utilities
      bear # For generating a compile-commands.json
      just # Fod building.
    ];
  }

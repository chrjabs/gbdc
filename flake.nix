{
  description = "Development environment for GBDC";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
  };

  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} (_: {
      systems = [
        "x86_64-linux"
        "aarch64-linux"
        "x86_64-darwin"
        "aarch64-darwin"
      ];
      perSystem = {pkgs, ...}: {
        devShells.default = let
          lib = pkgs.lib;
          libs = with pkgs; [
            libarchive
          ];
          python = pkgs.python3.withPackages (python-pkgs: with python-pkgs; [pip pybind11]);
        in
          pkgs.mkShell.override {stdenv = pkgs.clangStdenv;} rec {
            nativeBuildInputs = with pkgs; [
              cmake
              ninja
              python
            ];
            buildInputs = libs;
            LD_LIBRARY_PATH = lib.makeLibraryPath libs;
          };
      };
    });
}

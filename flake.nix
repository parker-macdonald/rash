{
    description = "rash, the rat ass shell, is a rudimentary shell written in C";

    inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    inputs.flake-utils.url = "github:numtide/flake-utils";

    outputs = { self, nixpkgs, flake-utils, ... }:
        flake-utils.lib.eachDefaultSystem (system: let pkgs = nixpkgs.legacyPackages.${system}; in {
            devShells.default = pkgs.mkShell {
                packages = with pkgs; [
                    clang
                    clang-tools
                    gdb
                    lldb
                    gnumake
                    valgrind
                    bear
                ];

                hardeningDisable = [ "all" ];
            };

            packages = {
                # Expose .#rash as the default
                default = self.packages.${system}.rash;

                # Compile rash with a clang-based stdenv
                rash = pkgs.callPackage ./package.nix { stdenv = pkgs.clangStdenv; };
                rash-musl = pkgs.pkgsMusl.callPackage ./package.nix { };

                # Allow compiling rash statically for armv7 linux (for a kindle paperwhite 6 specifically)
                rash-armv7-static = let
                    pkgsCross = pkgs.pkgsCross.armv7l-hf-multiplatform;
                in pkgsCross.callPackage ./package.nix {
                    stdenv = pkgsCross.clangStdenv;
                    compileStatically = true;
                };
            };
        });
}

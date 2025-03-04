{
    description = "Rash POSIX-compatible shell";

    inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    inputs.flake-utils.url = "github:numtide/flake-utils";

    outputs = { nixpkgs, flake-utils, ... }:
        flake-utils.lib.eachDefaultSystem (system: let pkgs = nixpkgs.legacyPackages.${system}; in {
            devShells.default = pkgs.mkShell {
                packages = with pkgs; [
                    clang
                    clang-tools
                ];
            };

            packages.default = pkgs.stdenv.mkDerivation rec {
                pname = "rash";
                version = "1";

                src = nixpkgs.lib.fileset.toSource {
                    root = ./.;
                    fileset = nixpkgs.lib.fileset.unions [ ./src ./Makefile ];
                };

                outputs = [ "out" "dev" ];

                nativeBuildInputs = with pkgs; [
                    clang
                ];

                dontConfigure = true;

                makeFlags = [
                    "DEBUG=0"
                ];

                installPhase = ''
                    runHook preInstall

                    mkdir -p $out/bin
                    cp ./build/${pname} $out/bin/

                    runHook postInstall
                '';

                meta = let lib = nixpkgs.lib; in {
                    mainProgram = "rash";

                    description = "rash, a rudimentary shell written in C";
                    homepage = "https://git.myriation.xyz/parker_macdonald/rash";
                    license = lib.licenses.mit;
                    platforms = lib.platforms.linux;
                };
            };
        });
}

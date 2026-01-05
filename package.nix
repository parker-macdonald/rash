{
    stdenv,
    lib,
    glibc,

    compileStatically ? false,
    ...
}:
stdenv.mkDerivation rec {
    pname = "rash";
    version = "0.6.8";

    src = lib.fileset.toSource {
        root = ./.;
        fileset = lib.fileset.unions [ ./src ./Makefile ];
    };

    outputs = [ "out" "dev" ];

    buildInputs = [] ++ lib.optional compileStatically [ glibc.static ];

    dontConfigure = true;

    hardeningDisable = [ "all" ];

    makeFlags =
        [
            "DEBUG=0"
            "CC=${stdenv.cc.targetPrefix}cc"
        ]
        ++ lib.optional compileStatically "STATIC=1";

    installPhase = ''
        runHook preInstall

        mkdir -p $out/bin
        cp ./build/${pname} $out/bin/

        runHook postInstall
    '';

    meta = {
        mainProgram = "rash";

        description = "rash, a rudimentary shell written in C";
        homepage = "https://git.myriation.xyz/parker_macdonald/rash";
        license = lib.licenses.mit;
        platforms = lib.platforms.linux;
    };
}

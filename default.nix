{ }:

let pkgs = import <nixpkgs> {};
    stdenv = pkgs.stdenv;
    fetchFromGitHub = pkgs.fetchFromGitHub;
    groff = pkgs.groff;
    cmake = pkgs.cmake;
    python2 = pkgs.python2;
    perl = pkgs.perl;
    libffi = pkgs.libffi;
    libbfd = pkgs.libbfd;
    libxml2 = pkgs.libxml2;
    valgrind = pkgs.valgrind;
    ncurses = pkgs.ncurses;
    lib = pkgs.lib;
    zlib = pkgs.zlib;

in stdenv.mkDerivation {
  pname = "llvm";
  version = "3.6-mono-2017-02-15";

  src = ./.;

  nativeBuildInputs = [ cmake ];
  buildInputs = [ perl groff libxml2 python2 libffi ] ++ lib.optional stdenv.isLinux valgrind;

  propagatedBuildInputs = [ ncurses zlib ];

  # hacky fix: created binaries need to be run before installation
  preBuild = ''
    mkdir -p $out/
    ln -sv $PWD/lib $out
  '';
  postBuild = "rm -fR $out";

  cmakeFlags = with stdenv; [
    "-DLLVM_USE_LINKER=lld"
    "-DCMAKE_EXPORT_COMPILE_COMMANDS=1"
    "-DLLVM_ENABLE_PROJECTS=\"clang;lld\""
    "-DLLVM_EXPERIMENTAL_TARGETS_TO_BUILD=TinyRAM"
    "-DLLVM_USE_SPLIT_DWARF=1"
    "-DLLVM_ENABLE_FFI=ON"
    "-DLLVM_BINUTILS_INCDIR=${libbfd.dev}/include"
  ] ++ lib.optional (!isDarwin) "-DBUILD_SHARED_LIBS=ON";

  meta = {
    description = "Collection of modular and reusable compiler and toolchain technologies - Mono build";
    homepage    = "http://llvm.org/";
    license     = lib.licenses.bsd3;
    maintainers = with lib.maintainers; [ thoughtpolice ];
    platforms   = lib.platforms.all;
  };
}

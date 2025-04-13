{
  description = "JASM Dev Env";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs?ref=nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils, ... }@inputs:
  flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs { inherit system; };
      llvmPkgs = pkgs.llvmPackages;
    in
    {
      devShells.default = pkgs.mkShell {
        packages = with pkgs; [
          cmake
          llvmPkgs.clang
          llvmPkgs.clang-tools
          stdenv.cc.libc.dev
        ];

        shellHook = '''';
      };
    });
}

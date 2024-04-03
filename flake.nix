{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";
    systems.url = "github:nix-systems/default";

    # Dev tools
    treefmt-nix.url = "github:numtide/treefmt-nix";
  };

  outputs = inputs:
    inputs.flake-parts.lib.mkFlake { inherit inputs; } {
      systems = import inputs.systems;
      imports = [
        inputs.treefmt-nix.flakeModule
      ];
      perSystem =
        { config
        , self'
        , pkgs
        , lib
        , system
        , ...
        }:
        {
          packages.default = pkgs.stdenv.mkDerivation {
            name = "minishell";
            version = "1.0";

            src = ./.;
            nativeBuildInputs = with pkgs; [

            ];

            buildInputs = with pkgs; [
            ];


            buildPhase = ''
              make
            '';
            installPhase = ''
mkdir -p $out/bin
              cp minishell $out/bin
            '';
          };




          devShells.default = pkgs.mkShell {
            inputsFrom = [
              config.treefmt.build.devShell
            ];

            packages = with pkgs; [
              gdb
              valgrind
            ];
          };


          treefmt.config = {
            projectRootFile = "flake.nix";
            programs = {
              nixpkgs-fmt.enable = true;
              clang-format.enable = true;
            };
          };
        };
    };
}

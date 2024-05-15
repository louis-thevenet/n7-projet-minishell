{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-parts.url = "github:hercules-ci/flake-parts";

    # Dev tools
    treefmt-nix.url = "github:numtide/treefmt-nix";
  };
  outputs = inputs @ {flake-parts, ...}:
    flake-parts.lib.mkFlake {inherit inputs;} {
      systems = ["x86_64-linux"];

      imports = [
        inputs.treefmt-nix.flakeModule
      ];

      perSystem = {
        config,
        self',
        pkgs,
        lib,
        ...
      }: {
        packages.projet = pkgs.stdenv.mkDerivation {
          name = "minishell";
          version = "1.0";

          src = ./projet;

          nativeBuildInputs = [];
          buildInputs = [];

          buildPhase = ''
            make
          '';
          installPhase = ''
            mkdir -p $out/bin
            cp minishell $out/bin
          '';
        };
        packages.tp = pkgs.stdenv.mkDerivation {
          name = "minishell";
          version = "1.0";

          src = ./tp/minishell;

          nativeBuildInputs = [];
          buildInputs = [];

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
            # C
            gdb
            valgrind

            # Nix
            nil
            alejandra

            # Typst
            typst
            typst-lsp
            typst-fmt

            # Utils
            zip
            unzip
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

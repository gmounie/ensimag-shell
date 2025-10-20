let
  pkgs = import <nixpkgs> {};
in
  pkgs.mkShell {
    buildInputs = with pkgs; [
      ruby
      rubyPackages.minitest
      guile
      readline
    ];
    nativeBuildInputs = with pkgs; [
      pkg-config
    ];
  }

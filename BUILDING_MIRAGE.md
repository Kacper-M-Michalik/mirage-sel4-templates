This library only supports unikernels built for aarch64 with the hvt target. Follow these steps to build your unikernels:

# Building on aarch64
Building on aarch64 follows the standard steps, with hvt as the target:

```sh
mirage configure -t hvt
make depends
make build
```

# Building on x86_64 for aarch64:
Cross-compiling on x86_64 requires a few extra steps, running them in this order is important; any other commands that would cause reinstallation of the dependencies will drop the cross-compiler: 

```sh
# We have to first build for x86_64 unfortunately 
mirage configure -t hvt
make depends
make build
# These steps specifically allow cross compiling
opam install ocaml-solo5-cross-aarch64
dune clean
make build
```
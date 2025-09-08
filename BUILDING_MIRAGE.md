# Building MirageOS for sel4
MirageOS can be run inside sel4 as a Virtual Machine. Microkit is used to create VMM's that control MirageOS guests, and solo5 is used as a shim by MirageOS to perform environment setup before running OCaml based code.
<br><br>
Currently Microkit and the solo5 VMM library only support virtualisation of aarch64 guests, as such your solo5 based unikernels must be built for aarch64 with the hvt target.

## Building on aarch64
Building on aarch64 follows the standard steps, with hvt as the target:

```sh
mirage configure -t hvt
make depends
make build
```

## Building on x86_64 for aarch64:
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

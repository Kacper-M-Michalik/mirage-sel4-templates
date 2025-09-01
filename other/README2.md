This system a single instance of the example hello-world MirageOS unikernel running on a virtual machine in sel4.

You can build the system, generating the 'loader.image' file with these commands:
NOTE: You must build the system in debug mode, otherwise the kernel will not bundle in a debug serial driver, meaning you will see no output.

//ADD BUILD

You can run the resulting system image in qemu like this:

qemu-system-aarch64 -machine virt,virtualization=on \
		-cpu cortex-a53 \
		-serial mon:stdio \
		-device loader,file=loader.image,addr=0x70000000,cpu-num=0 \
		-m size=2G \
		-nographic \

You can run the resulting system image on a Raspberry Pi 4B 1GB/2GB/4GB/8GB like this:

//ADD INSTRUCTIONS



//Change so you can bundle different guests? turn it into an arg with different folders

You can reproduce the hello.hvt file with these commands:

git clone https://github.com/mirage/mirage-skeleton.git
cd mirage-skeleton/tutorial/hello/

On aarch64:
mirage configure -t hvt
make depends
make build

on x86_64:
# We have to first build for x86_64 unfortunately 
mirage configure -t hvt
make depends
make build
# These steps specifically allow cross compiling
opam install ocaml-solo5-cross-aarch64
dune clean
make build

You should see hello.hvt in the dist/ folder in your current build directory.
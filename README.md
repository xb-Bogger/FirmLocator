

# FirmRCA

Embedded Firmware Root Cause Analysis.

This repo contains the source code of the paper "The Last Mile of Fuzzing: An Efficient Fault Localization Framework for ARM Embedded Firmware"

## NOTE

*During the development of FirmLocator, footprint collection and root cause analysis were carried out sequentially on two separate servers. However, the server responsible for footprint collection suffered a hard drive failure. As a result, the version of fuzzware used by the current repository’s fuzzware-emulator is uncertain, which may introduce potential instability in the experimental results.*

## How to Install

Step 1. Clone the repo.

```shell
git clone https://github.com/NESA-Lab/FirmLocator
cd ./FirmLocator
```

Step 2. Install the dependencies.

Install the capstone.

```shell
git clone https://github.com/capstone-engine/capstone.git
cd ./capstone
git reset --hard 622059530f172b1570a424e3f7ef5fda8c00dab0 # not sure if new features in the latest commit affect our code
```

Then you should compile and install capstone as system library, following the instructions in capstone.
For example, on *nix:

```shell
sudo ./make.sh
sudo ./make.sh install
```

Some python packages:

```shell
pip3 install matplotlib pandas pyyaml openpyxl 
```

Step 3. Compile the capnproto library.

(Option) Configure c-capnproto, if you want to modify tracing data.

```shell
curl -O https://capnproto.org/capnproto-c++-1.0.1.tar.gz
tar zxf capnproto-c++-1.0.1.tar.gz
cd ./capnproto-c++-1.0.1
./configure
make -j4 check
sudo make install
```

```shell
git clone https://gitlab.com/dkml/ext/c-capnproto.git
cd ./c-capnproto
sudo apt install ninja-build
cmake --preset=ci-linux_x86_64
cmake --build --preset=ci-tests
```

Compile the library.

```shell
cd ./test_c_capnproto
# before capnp compile, you can modify bintrace.capnp if need
capnp compile -o ./c-capnproto/build/capnpc-c bintrace.capnp 
gcc *.c -I./ -shared -fPIC -o libcapnproto.so
cp ./libcapnproto.so ../src/lib
```

Step 4. Compile the project binary

Note that you should comment/uncomment the settings in `Makefile.am`.

```shell
cd ./src
./autogen.sh
./configure
cd src
make
```

If something wrong occurs when running `./configure`, please make sure these compilation files use LF instead of CRLF. You can also check [POMP](https://github.com/junxzm1990/pomp) for installation reference.

## Dataset 

We prepare 3 testsuites as a demo in the `testsuites-demo.zip` file. You can download full dataset from [10.5281/zenodo.15623399](https://doi.org/10.5281/zenodo.15623399). 

If you want to generate more testcases, you can prepare your files like this:

```
.
├── testsuites
│   ├── <something-your-bin-name1>
│   │   ├── firmware.bin
│   ├── <something-your-bin-name2>
│   │   ├── firmware.bin
│   ├── <something-your-bin-name3>
│   │   ├── firmware.bin

```

`<something-your-bin-name1>` should be the value of the `name` key in `config.yml`. You should also specify `bin_load_addr` that loads the binary.

Then please refer to [fuzzware-fuzzer](https://github.com/fuzzware-fuzzer/fuzzware-emulator) to setup the environment. Please do not clone the their repository in that the unicorn version may be different. Use the fuzzware-emulator in this repository, instead.

Then, run `python dataset.py` to generate your own dataset.
## About

English | [中文](https://github.com/mustime/Adrill/blob/main/README.zh-CN.md)

Adrill is an Android native libraries injection tool written in C++1X, supports arch arm/arm64/x86/x86_64.

You may notice there are already tons of similar inject tools, but few of them targets on all archs(not that I know of).

Furthermore, Adrill make it more easier when it comes to zygote[64] injection(see for workaround at [ptrace_wrapper.cc](https://github.com/mustime/Adrill/blob/main/source/ptrace_wrapper.cc#L184)). And there will be detail info printed when any error occured.

I've test on multiple arch platforms from Android 4.x to 7.0. Fire an issue if there's something I could help with.

> Notice: running on root privilege is a must.

## Build from source

First clone this repository:

```bash
git clone git@github.com:mustime/Adrill.git
```

Then you need to update the submodle: 

```bash
cd Adrill/
git submodule update --init
```

Say you want to use Adrill in Android emulators, i.e., normally a x86 executable:

```
cmake -S . -B build -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=x86 -DCMAKE_ANDROID_NDK=$ANDROID_NDK_ROOT
cmake --build build --parallel 4 --target adrill
```

> Notice: define ${ANDROID_NDK_ROOT} in your env or change the command at will.

## Usage:

```
adrill [--pid <number>] | [--pname <string>] --libpath <path>
   -h,--help      print this message.
      --pid       target process id. e.g., grep from 'ps' command
      --pname     target process name. used to match with content in /proc/<pid>/cmdline.
      --libpath   absolute path to inject. only supports ELF file.
```

## Liscense

See [LISCENSE](https://github.com/mustime/Adrill/blob/main/LICENSE) file for more details
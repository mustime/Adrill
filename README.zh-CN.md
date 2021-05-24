## Adrill

中文 | [English](https://github.com/mustime/Adrill/blob/main/README.md)

Adrill是一款基于C++1X编写的Android ELF库文件注入工具，全面支持arm/arm64/x86/x86_64架构。

你可能会留意到已经有不少类似的注入工具，但是并没有一次性支持所有常见CPU架构（至少据我所知未有发现），基本上都仅支持arm和x86。

另外针对zygote[64]进程的注入，Adrill做了一些额外的工作大大提高了成功率（参见[ptrace_wrapper.cc](https://github.com/mustime/Adrill/blob/main/source/ptrace_wrapper.cc#L184)处的注释说明）；还有针对每一步注入环节的错误信息导出更加完善，方便排查。

目前此工具已经在Android 4.x至7.0测试过（更高版本的Android系统比较少有root权限），碰到任何问题欢迎提交issue讨论。

> 注意：运行此工具需要root权限。

## 从源码编译

首先拉取源码：

```bash
git clone git@github.com:mustime/Adrill.git
```

然后把git submodle都拉取下来：

```bash
cd Adrill/
git submodule update --init
```

假设你在模拟器上使用Adrill（一般是i386即x86架构）：

```bash
cmake -S . -B build -DCMAKE_SYSTEM_NAME=Android -DCMAKE_SYSTEM_VERSION=21 -DCMAKE_ANDROID_ARCH_ABI=x86 -DCMAKE_ANDROID_NDK=$ANDROID_NDK_ROOT
cmake --build build --parallel 8 --target adrill
```

> 注意: 需要你在命令行环境中定义 ${ANDROID_NDK_ROOT}，或者修改上面对应的命令。

## 命令行运行:

```bash
adrill [--pid <number>] | [--pname <string>] --libpath <path>
   -h,--help      打印本说明
      --pid       目标进程id，可以通过`ps`命令行查找得到
      --pname     目标进程名，与`/proc/<pid>/cmdline`中内容一致，对于zygote这类具名进程的注入比较方便
      --libpath   注入目标的完整路径，只能是ELF库文件
```

## Liscense

具体请查看 [LISCENSE](https://github.com/mustime/Adrill/blob/main/LICENSE) 文件的说明。
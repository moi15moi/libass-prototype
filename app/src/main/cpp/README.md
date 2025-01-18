## Build instructions (Linux, macOS)
### Required dependencies
Before running the build script for libass, you will need the following dependencies installed on your system. You may use your whatever package manager to install these.

* build-essential
* pkg-config
* autoconf
* automake
* libtool
* wget
* meson and it's dependencies

### Build Options

```
NDK_PATH="<path to Android NDK>"
```

* Set the host platform (use "darwin-x86_64" for Mac OS X):

```
HOST_PLATFORM="linux-x86_64"
```

* Set the ABI version for native code (typically it's equal to minSdk and must
  not exceed it):

```
ANDROID_ABI_VERSION=21
```

* Execute `build_libass.sh` to build libass for `armeabi-v7a`, `arm64-v8a`,
  `x86` and `x86_64`. The script can be edited if you need to build for
  different architectures:

```
./build_libass.sh "${NDK_PATH}" "${HOST_PLATFORM}" "${ANDROID_ABI_VERSION}"
```

## Build instructions (Windows)

We do not provide support for building this module on Windows, however it should
be possible to follow the Linux instructions in [Windows PowerShell][].

[Windows PowerShell]: https://docs.microsoft.com/en-us/powershell/scripting/getting-started/getting-started-with-windows-powershell

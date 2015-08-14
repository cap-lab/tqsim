# TQSIM (Timed QEMU-based Simulator)
We present TQSIM, an open source, fast, and cycle-approximate simulation tool built on QEMU to support simulation of generic modern superscalar out-of-order processors. TQSIM is developed by Codesign and Parallel Processing Laboratory(CAPLab), Seoul National University, and sponsored by Samsung Advanced Institute of Technology (SAIT).

You can find more details about TQSIM in future literatures from CAPLab. (will be available at  http://iris.snu.ac.kr/xe/papers)

# Licence
TQSIM is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

# Installation

To install TQSim on a linux system, do the following steps:

  1. Satisfy external dependencies: *build-essential, gcc, pkg-config, glib-2.0, libglib2.0-dev, libsdl1.2-dev, zlib1g-dev, libxml2-dev, libpthread-stubs0-dev*. In case you can use Advanced Packaging Tool (APT), do the following: 
  
   ```
  sudo apt-get install build-essential gcc pkg-config glib-2.0 libglib2.0-dev libsdl1.2-dev zlib1g-dev libxml2-dev libpthread-stubs0-dev
  ```
  2. Change the current directory to the main directory of the source tree (We will just call it *TQSIM_SRC* for short), then move in (TQSIM_SRC)/src.
  3. Modify *tqsim_configure* to be suitable for your environment. In case you want to install the package in another place than the specified user directory ($HOME/tqsim), change it to the desired place (*TQSIM_BIN* for short).
  4. Configure the package:
   
  ```
  ./tqsim_configure
  ```
  5. Compile the package: 
  
  ```
  make
  ```
  6. Install the package: 
  
  ```
  make install
  ```

In order to execute TQSim, it is required to provide the detailed specification of the target core architecture. You can find a sample configuration file *armv7.cfg* at *TQSIM_SRC*. Set the *ARCH_CONFIG_FILE* environment variable with an export command: 

```
export ARCH_CONFIG_FILE=(CUSTOM_DIRECTORY)/armv7.cfg
```

To launch a test run, do the followings.

1. Edit .bashrc in your home directory and add the following line:

```
export PATH=(TQSIM_BIN)/bin:$PATH
```
2. change the current directory to (TQSIM_SRC)/example:
```
cd (TQSIM_SRC)/example
```
3. type the command "qemu-arm hello_world" 


# Note

Current we only support simulation of arm binaries compiled through arm-linux-gnueabi-gcc compiler with "-static" option. Various guest binaries, including x86, x86-64, MIPS, will be supported in the near future. 

# Contact

servant.shin@gmail.com

```
Shin-haeng Kang, Ph.D. Candidate 
Codesign and Parallel Processing Lab
Department of EECS/CSE
Seoul National University
```

# Appendix: Libraries We Use
The following sets forth attribution notices for third party software that may be contained in portions of TQSIM product. We thank the open source community for all of their contributions.

## QEMU
The following points clarify the QEMU license:

1) QEMU as a whole is released under the GNU General Public License, version 2.

2) Parts of QEMU have specific licenses which are compatible with the GNU General Public License, version 2. Hence each source file contains its own licensing information.  Source files with no licensing information are released under the GNU General Public License, version 2 or (at your option) any later version.

As of July 2013, contributions under version 2 of the GNU General Public License (and no later version) are only accepted for the following files or directories: bsd-user/, linux-user/, hw/vfio/, hw/xen/xen_pt*.

3) The Tiny Code Generator (TCG) is released under the BSD license  (see license headers in files).

4) QEMU is a trademark of Fabrice Bellard. 

Fabrice Bellard and the QEMU team

## DARM

Copyright (c) 2013, Jurriaan Bremer All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of the darm developer(s) nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

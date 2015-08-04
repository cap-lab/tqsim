# TQSIM (Timed QEMU-based Simulator)
Timed QEMU (TQEMU) is a fast and generic cycle-approximate simualtor supporting modern superscalar out-of-order processors.

TQEMU is developed by CAPLab, SNU, sponsored by Samsung SAIT.

You can find more details about Timed QEMU in future literatures from CAPLab, SNU.

# Licence
Timed QEMU (TQEMU) is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 3.

# Setup
External dependencies: buildessential, gcc, pkg-config, glib-2.0, libglib2.0-dev, libsdl1.2-dev, zlib1g-dev, libxml2-dev, libpthreadstubs0-dev

Modify tqemu_configure for your environment.
make
make install

생성된 qemu의 실행파일은 설치 디렉토리의 bin 이하에서 찾을 수 있는 qemu-arm이다. 이를 path에 걸어주어 다른 directory에서도 실행할 수 있게 해야 한다. 

이어 TQEMU를 실행하기 위해서는 아키텍처 명세가 필요하다. ARCH\_CONFIG\_FILE. Default 코어 명세 파일들은 qemu의 소스를 푼 디렉토리에 포함되어 있으니 편한 곳으로 옮기고 환경 변수로 그들의 위치를 설정한다. 이러한 설정들을 마치면  .bash\_rc에 다음과 같은 라인들이 추가하는 것을 추천한다.

export QEMU_BIN=$HOME/qemu/bin/
export ARCH_CONFIG_FILE=~/armv7.cfg
export PATH=$PATH:$QEMU_BIN

TQEMU 자체가 실행이 잘되었는지 확인하기 위해서는 tqemu\_configure로 configure했을 때 기준으로 qemu 소스 디렉토리/example로 가서 qemu-arm hello_world를 실행해보면 된다.

# Execution


# Appendix: Libraries We Use
The following sets forth attribution notices for third party software that may be contained in portions of the timed QEMU product. We thank the open source community for all of their contributions.

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

#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

	# START MODS for Kernel Build ---
	
    #'deep clean' - Ref slide 12 of lecture notes
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
	
	# Note found some missing dependencies causing following steps to fail... 
	# sudo apt-get install flex
	# sudo apt-get install bison
	# sudo apt-get install libssl-dev
	
	#configure - Ref slide 13 of lecture notes
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
	
	#build vmlinux - Ref slide 14 of lecture notes
	make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all

	#build modules - Ref slide 15 of lecture notes
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
	
	#build devicetree - Ref slide 15 of lecture notes
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
		
	# ---- END MODS

fi

echo "Adding the Image in outdir"
# Added this line manually.. not sure if it was supposed to be there. 
# qemu complains that image file is missing otherwise
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# START MODS for base directories ---

# We cd'd into the OUTDIR above. Create rootfs in current wd
mkdir -p rootfs
cd rootfs

#reference slide 11 of lecture slides
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin /usr/lib usr/sbin
mkdir -p var/log

#Line below cd's back to OUTDIR. No need to clean up wd. 

# ---- END MODS 


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    
    # START MODS for config busybox ---
    # Reference slide 14 of lecture ntoes
    make distclean
    make defconfig
	# ---- END MODS     
else
    cd busybox
fi

# START MODS for build/install  busybox ---
# Reference slide 14 of lecture notes. Need to quote dirname?

make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd ${OUTDIR}/rootfs
# ---- END MODS

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
# START MODS for library dependencies ---
# Reference pg. 209 of Mastering Embedded Linux Programming (2e)

# Output of lines above: 
# [Requesting program interpreter: /lib/ld-linux-aarch64.so.1]
# 0x0000000000000001 (NEEDED)             Shared library: [libm.so.6]
# 0x0000000000000001 (NEEDED)             Shared library: [libresolv.so.2]
# 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]

# Set Environment Variable to save typing as siggested in MELP2e... 
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)

# Note these are symlinks! Need to copy both source and destination

# $ ls -l $SYSROOT/lib/ld-linux-aarch64.so.1
# lrwxrwxrwx 1 matt matt 19 Jul  1  2021 /home/matt/gcc-arm/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc/lib/ld-linux-aarch64.so.1 -> ../lib64/ld-2.33.so

cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 ./lib/
cp -a ${SYSROOT}/lib64/* ./lib64/
# cp -a ${SYSROOT}/lib64/ld-2.33.so ./lib/
# cp -a ${SYSROOT}/lib64/ld-2.33.so ./lib64/

# ~/gcc-arm/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/aarch64-none-linux-gnu/libc/lib64$ ls -l libm.so.6
# lrwxrwxrwx 1 matt matt 12 Jul  1  2021 libm.so.6 -> libm-2.33.so

# cp -a ${SYSROOT}/lib64/libm.so.6 ./lib64/
# cp -a ${SYSROOT}/lib64/libm-2.33.so ./lib64/

# ls -l $SYSROOT/lib64/libresolv.so.2
# lrwxrwxrwx 1 matt matt 17 Jul  1  2021 /home/matt/gcc-arm/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc/lib64/libresolv.so.2 -> libresolv-2.33.so

# cp -a ${SYSROOT}/lib64/libresolv.so.2 ./lib64/
# cp -a ${SYSROOT}/lib64/libresolv-2.33.so ./lib64/

# ls -l ${SYSROOT}/lib64/libc.so.6
# lrwxrwxrwx 1 matt matt 12 Jul  1  2021 /home/matt/gcc-arm/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc/lib64/libc.so.6 -> libc-2.33.so

# cp -a ${SYSROOT}/lib64/libc.so.6 ./lib64/
# cp -a ${SYSROOT}/lib64/libc-2.33.so ./lib64/

# ---- END MODS


# TODO: Make device nodes
# START MODS for dev nodes ---
# Reference MELP2e pg 213
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1
# ---- END MODS

# TODO: Clean and build the writer utility
# START MODS to clean and cross-compile the writer from hw2 ---
cd ${FINDER_APP_DIR}
make CROSS_COMPILE=${CROSS_COMPILE} clean
make CROSS_COMPILE=${CROSS_COMPILE}
# ---- END MODS


# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
# START MODS to copy finder files to rootfs ---

# Files listed in assignemnt description
cd ${FINDER_APP_DIR}
cp ./writer ${OUTDIR}/rootfs/home/
cp ./finder.sh ${OUTDIR}/rootfs/home/
cp ./finder-test.sh ${OUTDIR}/rootfs/home/

mkdir -p ${OUTDIR}/rootfs/home/conf
cp conf/username.txt ${OUTDIR}/rootfs/home/conf/

cp ./autorun-qemu.sh ${OUTDIR}/rootfs/home/
# ---- END MODS

# TODO: Chown the root directory
# START MODS to chown the root directory---
# Reference Slide 18 of lecture slides
cd ${OUTDIR}/rootfs
sudo chown -R root:root *
# ---- END MODS

# TODO: Create initramfs.cpio.gz
# START MODS to create initramfs---
# Reference Slide 19 of lecture slides
cd ${OUTDIR}/rootfs
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
gzip -f ${OUTDIR}/initramfs.cpio

# ---- END MODS



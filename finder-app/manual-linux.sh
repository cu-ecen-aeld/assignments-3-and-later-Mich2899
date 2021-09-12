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
export SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

#SYSROOT=/home/michelle/Downloads/install-lnx/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/../aarch64-none-linux-gnu/libc


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

    # TODO: Add your kernel build steps here
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- mrproper		
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- defconfig		
    make -j4 ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- all			
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- modules		
    make ARCH=arm64 CROSS_COMPILE=aarch64-none-linux-gnu- dtbs			

fi 

echo "Adding the Image in outdir"
cd "$OUTDIR"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
    mkdir rootfs
    cd rootfs

#folder tree
	mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
	mkdir -p usr/bin usr/lib usr/sbin home/conf
	mkdir -p var/log 


cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
        make distclean
        make defconfig
else
    cd busybox
fi

# TODO: Make and insatll busybox
	make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} CONFIG_PREFIX=$OUTDIR/rootfs install

echo "Library dependencies"
cd "${OUTDIR}/rootfs"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
cd "${OUTDIR}/rootfs"
cp -L $SYSROOT/lib/ld-linux-aarch64.* lib
cp -L $SYSROOT/lib64/libm.so.* lib64
cp -L $SYSROOT/lib64/libresolv.so.* lib64
cp -L $SYSROOT/lib64/libc.so.* lib64


# TODO: Make device nodes
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 600 dev/console c 5 1

# TODO: Clean and build the writer utility
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=${CROSS_COMPILE}

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cd $FINDER_APP_DIR
cp  finder.sh finder-test.sh writer autorun-qemu.sh "${OUTDIR}/rootfs/home"
cp  "conf/username.txt" "${OUTDIR}/rootfs/home/conf/"

# TODO: Chown the root directory
cd "${OUTDIR}/rootfs"
sudo chown -R root:root *

# TODO: Create initramfs.cpio.gz
cd "${OUTDIR}/rootfs"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "${OUTDIR}"
gzip -f initramfs.cpio
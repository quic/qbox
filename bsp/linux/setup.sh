#!/bin/bash

#
# A script to pull EVERYTHING needed to build a bootable Linux Android
# kernel
#

getrepo() {
  if [ -d "android-tools" ]
  then
      return
  fi
  mkdir -p android-tools
  curl https://storage.googleapis.com/git-repo-downloads/repo > android-tools/repo
  chmod a+x android-tools/repo
}


#
# getmanifest:  This may take a very long time to complete and could
#               pull 28G (more or less) of data to your host.
# 
#               May want to use: "tar c Google | pigz -p 32 > Google-orig.tgz"
#               to create a backup of the original in the event of some user
#               error.
#
getmanifest() {
  if [ -d "build" ]
  then
      return
  fi
  android-tools/repo init -u  https://android.googlesource.com/kernel/manifest -b common-android-mainline
  android-tools/repo sync -j16
}

getbuildroot() {
    if [ -d "buildroot" ]
    then
        return
    fi
    git clone https://github.com/buildroot/buildroot.git
    pushd buildroot
    git checkout -b dev -t origin/2022.02.x
    popd
}
buildroot() {
    pushd buildroot
    if [ -z $1 ]
    then
	runlevel=0
    elif [ $1 -le 6 ]
    then
	runlevel=$1
    else
	runlevel=0
    fi
    echo $runlevel

    git checkout package/sysvinit/inittab

    if [ $runlevel -le 6 ]
    then
	sed -i -e 's/id:3:initdefault:/id:'"$runlevel"':initdefault:/' package/sysvinit/inittab
	git diff package/sysvinit/inittab
    fi

#
# NOTE:
# Users of this script may want to alter this to a stable filepath and
# preserve these downloads as it saves time and makes for a more robust
# build
#
    if [ -d /prj/qct/llvm/target/vp_qemu_llvm/buildroot/arm/dl ]
    then
	export BR2_DL_DIR=/prj/qct/llvm/target/vp_qemu_llvm/buildroot/arm/dl
    fi

    cp ../extras/br_aarch64_virt_defconfig configs
    mkdir -p board/qualcomm
    cp ../extras/post-build.sh  board/qualcomm
    make clean
    make br_aarch64_virt_defconfig
    make V=1 2>&1 | tee build.log
    popd
}
patchsrc() {
    git checkout extras/vp_defconfig
    sed -i -e 's,CONFIG_INITRAMFS_SOURCE.*,CONFIG_INITRAMFS_SOURCE=\"'"$PWD"'/buildroot/output/images/rootfs.cpio.gz\",' extras/vp_defconfig

}

#
# buildsrc: This builds the initial kernel verifies your setup is probably
#            OK.
#            Note: LTO="thin" is optional but save about 15m of link time.
#
buildsrc() {
    cp $PWD/extras/vp_defconfig $PWD/common/arch/arm64/configs/vp_defconfig
    cp $PWD/extras/build.config.gki.aarch64.qemu $PWD/common/build.config.gki.aarch64.qemu

    LTO="none" \
    BUILD_CONFIG=common/build.config.gki.aarch64.qemu \
    build/build.sh -j24 V=1
}

mkdtb() {
    cd extras/dts && make
    cd -
}

mksamplefs() {
    cd extras/fs && make
    cd -
}
#
# Once the source is built it can be rebuilt like this:
#
# make O=/prj/qct/llvm/common/users/users/sidneym/QEMU/Google/out/common LLVM=1 LLVM_IAS=1 DEPMOD=depmod DTC=dtc ARCH=arm64 Image -j 32 V=1
#
# Make sure the clang path matches the one used in the original build.
# running "strings vmlinux" | fgrep clang will help identify the version.
#
# Example would be: 
#   /prj/qct/llvm/common/users/users/sidneym/QEMU/Google/prebuilts/clang/host/linux-x86/clang-r433403/bin/clang


# /local/mnt/workspace/qemu/obj/qemu-system-aarch64 -dtb  /prj/qct/llvm/common/users/users/sidneym/QEMU/rumi.dtb -machine virt,virtualization=on,gic-version=3,highmem=off -cpu max -m size=4G -smp cpus=8 -nographic -kernel out/dist/Image

help() {
	printf "Usage: %s: [-r runlevel (0-6)]\n", $0
}
#
# Process options:
#
runlevel=3

while getopts ":hr:" option
do
	case $option in
		h) # Help
			help
			exit;;
		r) # Runlevel
			runlevel="$OPTARG";;
	esac
done

#
# validate params here
#
if [[ $runlevel -gt 6 ]] || [[ $runlevel -lt 0 ]]
then
	echo "Error runlevel out of range, 0-6"
	exit;
fi


set -e
getrepo
getmanifest
getbuildroot
buildroot $runlevel
patchsrc
buildsrc 2>&1 | tee kbuild.log
mkdtb
mksamplefs

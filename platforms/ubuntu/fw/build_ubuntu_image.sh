#!/usr/bin/bash

 # Copyright (c) 2023 Qualcomm Innovation Center, Inc. All Rights Reserved.
 #
 # This program is free software; you can redistribute it and/or
 # modify it under the terms of the GNU General Public License
 # as published by the Free Software Foundation; either version 2
 # of the License, or (at your option) any later version, or under the
 # Apache License, Version 2.0 (the "License”) at your discretion.
 #
 # SPDX-License-Identifier: Apache-2.0 OR GPL-2.0-or-later
 #
 # This program is distributed in the hope that it will be useful,
 # but WITHOUT ANY WARRANTY; without even the implied warranty of
 # MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 # GNU General Public License for more details.
 #
 # You should have received a copy of the GNU General Public License
 # along with this program; if not, write to the Free Software
 # Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
 # USA. You may obtain a copy of the Apache License at
 # http://www.apache.org/licenses/LICENSE-2.0


EXIT_SUCCESS=0
EXIT_ERROR=1
INITIAL_WD=$(pwd)

usage() {
    cat << EOF
***************************************************************************************
Build a customized Ubuntu OS (jammy) rootfs image and populate a QQVP_IMAGE_DIR.
(Tested on Ubuntu 20.04.5 LTS host machine)

The output build artifacts are:
- Image (uncompressed AARch64 Linux kernel image with efi stub)
- image.ext4.vmlinuz (gzip compressed AARch64 Linux kernel image)
- image.ext4 (ext4 root file system image)   
- image.ext4.initrd (initramfs to bring up the root file system)  
- image.ext4.manifest (list of all packages included in the image.ext4)
- ubuntu.dts (device tree source file to build ubuntu.dtb)
- ubuntu.dtb (device tree blob passed to the kernel Image by boot loader)

(please notice that this script needs sudo privilege)

Options:
-h  print usage and exit
-p  comma separated list of deb packages to be added to image.ext4
-t  systemd boot target [multi-user.target | graphical.target | <any other target>]
-s  Rootfs Image size
Example:
    build_ubuntu_image.sh -p <deb packages> -t  <systemd target>

please use these login info:
username: root
password: root
***************************************************************************************
EOF
}

# print failure msg and exit with $EXIT_ERROR.
# args:
# $1: failure msg 
fail() {
    echo "exit 1, $1"
    cd "$INITIAL_WD"
    exit $EXIT_ERROR
}

DEB_PACKAGES_TO_ADD=""
SD_TARGET="multi-user.target"
ROOTFS_IMAGE_SIZE="3G"

if [[ $# -eq 6 ]]; then
# Don't handle the correctness of the passed parameter in this case,
# they will be checked later in "case $1 ..." 
    echo "Use $1:"
    echo $2
    echo "Use $3:"
    echo $4
	echo "Use $5:"
    echo $6
elif [[ $# -eq 1  &&  "$1" = "-h" ]]; then
	usage
	exit $EXIT_SUCCESS
elif [[ $# -eq 1  &&  "$1" != "-h" ]]; then
	usage
	fail "please use the correct commands line arguments as described above"
elif [[ $# -eq 0 ]]; then
    set -- "-t" "multi-user.target" "-p" "" "-s" "3G"
    echo "Note: use default systemd boot target: multi-user.target"
    echo "Note: No new deb packages are added"
	echo "Note: use default Rootfs image size: 3G"
else
    echo "processing arguments ..."
fi

while :; do
    case $1 in
        -p)
            DEB_PACKAGES_TO_ADD=$2
            shift 2
			if [[ $# -eq 0 ]]; then break; fi
            ;;
        -t)
            SD_TARGET=$2
            shift 2
			if [[ $# -eq 0 ]]; then break; fi
            ;;
		-s)
            ROOTFS_IMAGE_SIZE=$2
            shift 2
			if [[ $# -eq 0 ]]; then break; fi
            ;;
        *)
            usage
            fail "unkown option used: $1"
            ;;
    esac
done

echo "Note: use systemd boot target: ${SD_TARGET}"
echo "Note: use extra packages: ${DEB_PACKAGES_TO_ADD}"
echo "Note: use Rootfs image size: ${ROOTFS_IMAGE_SIZE}"

MKOSI_GIT_REPO="https://github.com/systemd/mkosi"
MKOSI_REPO_DIR="mkosi"
MKOSI_TAG="v13"
OUTPUT_ARTIFACTS_DIR="Artifacts"
COMPRESSED_OUTPUT_KERNEL_IMAGE="image.ext4.vmlinuz"
INITRD_IMAGE="image.ext4.initrd"
DEVICE_TREE_SOURCE="ubuntu.dts"
DEVICE_TREE_BLOB="ubuntu.dtb"
DEVICE_TREE_TEMPLATE="ubuntu-dts.template"
INITRD_START_ADDR=2323644416 #0x8A800000

# clone git repo to a certain directory.
# args:
# $1: git repo to clone
# $2: directory to store the cloned repo 
clone_git_repo() {
    if [ -d "$2" ]; then
        cd ${INITIAL_WD}
        return
    fi
    echo "clone : $1"
    git clone "$1" "$2" || fail "can't clone $1 to $2"
    cd "$2" || fail "can't cd to $2"
    git fetch --all --tags || fail "can't git fetch --all --tags"
    cd ${INITIAL_WD}
}


# checkout a certain TAG.
# args:
# $1: TAG name
# $2: repo dir
checkout_tag() {
    echo "checkout TAG: $1"
    cd "$2" || fail "can't cd to $2"
    git checkout tags/$1 || fail "can't checout tag $1"
    cd ${INITIAL_WD}
}

# check if package is installed.
# args:
# $1: deb package name 
check_installed() {
    dpkg-query -s "$1" > /dev/null 2>&1
}

# install deb package.
# args:
# $1: deb package name
install_package() {
    sudo apt-get install $1 || fail "$1 can't be installed"
}

# check if all needed packages are installed.
# args:None 
check_needed_packages_installed() {
    echo "check if host machine needs packages to install..."
    check_installed "python3" || install_package "python3"
    check_installed "qemu-user-static" || install_package "qemu-user-static"
    check_installed "gzip" || install_package "gzip"
    check_installed "e2fsprogs" || install_package "e2fsprogs" # for mke2fs
    check_installed "debootstrap" || install_package "debootstrap"
    check_installed "dosfstools" || install_package "dosfstools"
    check_installed "squashfs-tools" || install_package "squashfs-tools"
    check_installed "gnupg" || install_package "gnupg"
    check_installed "tar" || install_package "tar"
    check_installed "cryptsetup-bin" || install_package "cryptsetup-bin"
    check_installed "xfsprogs" || install_package "xfsprogs"
    check_installed "xz-utils" || install_package "xz-utils"
    check_installed "zypper" || install_package "zypper"
    check_installed "sbsigntool" || install_package "sbsigntool"
    check_installed "btrfs-progs" || install_package "btrfs-progs"
    check_installed "arch-install-scripts" || install_package "arch-install-scripts"
    check_installed "device-tree-compiler" || install_package "device-tree-compiler"
    check_installed "systemd-container" || install_package "systemd-container"
    echo "all needed packages are installed."
}

# get git patch string.
# args:None
get_patch_str() {
    cat << EOF
diff --git a/mkosi/__init__.py b/mkosi/__init__.py
index 3aab454..c92737e 100644
--- a/mkosi/__init__.py
+++ b/mkosi/__init__.py
@@ -2916,14 +2916,14 @@ def install_debian_or_ubuntu(args: MkosiArgs, root: Path, *, do_run_build_script
 
     if args.release not in ("testing", "unstable"):
         if args.distribution == Distribution.ubuntu:
-            updates = f"deb http://archive.ubuntu.com/ubuntu {args.release}-updates {' '.join(repos)}"
+            updates = f"deb http://ports.ubuntu.com/ubuntu-ports {args.release}-updates {' '.join(repos)}"
         else:
             updates = f"deb http://deb.debian.org/debian {args.release}-updates {' '.join(repos)}"
 
         root.joinpath(f"etc/apt/sources.list.d/{args.release}-updates.list").write_text(f"{updates}\n")
 
         if args.distribution == Distribution.ubuntu:
-            security = f"deb http://archive.ubuntu.com/ubuntu {args.release}-security {' '.join(repos)}"
+            security = f"deb http://ports.ubuntu.com/ubuntu-ports {args.release}-security {' '.join(repos)}"
         elif args.release in ("stretch", "buster"):
             security = f"deb http://security.debian.org/debian-security/ {args.release}/updates main"
         else:
@@ -6726,7 +6726,7 @@ def load_args(args: argparse.Namespace) -> MkosiArgs:
         elif args.distribution == Distribution.ubuntu:
             args.mirror = "http://archive.ubuntu.com/ubuntu"
             if platform.machine() == "aarch64":
-                args.mirror = "http://ports.ubuntu.com/"
+                args.mirror = "http://ports.ubuntu.com/ubuntu-ports"
         elif args.distribution == Distribution.arch and platform.machine() == "aarch64":
             args.mirror = "http://mirror.archlinuxarm.org"
         elif args.distribution == Distribution.opensuse:
EOF
}

# patch mkosi tool to use http://ports.ubuntu.com/ubuntu-ports
# instead of http://archive.ubuntu.com/ubuntu for AArch64 packages.
# args:
# $1: patch string 
patch_mkosi() {
    if [ ! -d ${MKOSI_REPO_DIR} ]; then
        fail "can't find ${INITIAL_WD} repo"
    fi
    local patch_str="$1"
    cd "${MKOSI_REPO_DIR}" || fail "can't cd to ${MKOSI_REPO_DIR}"
    echo "${patch_str}" | git apply --whitespace=fix --check > /dev/null 2>&1 || { cd ${INITIAL_WD}; return; }
    echo "patch will be applied to mkosi/__init__.py"
    echo "${patch_str}" | git apply || fail "can't apply patch to mkosi"
    cd ${INITIAL_WD}
}

# use mkosi tool to build ubuntu image.
# args:
# $1: extra deb packages
# $2: systemd boot target
# $3: rootfs image size
do_mkosi_build() {
    local base_deb_packages="udev,dmsetup,networkd-dispatcher,systemd-timesyncd,libnss-systemd,systemd-hwe-hwdb,linux-image-generic,iproute2,iputils-ping,network-manager,gpg,vim,wget,openssh-server,ssh-client,net-tools,dhcpcd5"
    local extra_deb_packages=$( [ ! -z "$1" ] && printf ",$1" || printf "" )
    local graphical_packages=$( [[ "$2" == "graphical.target" ]] && printf ",ubuntu-desktop" || printf "" )
    local deb_packages="--package=${base_deb_packages}${extra_deb_packages}${graphical_packages}"
    echo "packages add to the rootfs image: ${deb_packages}"
    local image_size="$3"
	local re='^[0-9]+G$'
	if ! [[ "${image_size}" =~ $re ]]; then
		fail "the Rootfs image size should follow this pattern '^[0-9]+G$' ex: 5G"
	fi
	local input_image_size_num="${image_size:0:-1}"
	if [[ "$2" == "graphical.target" && "${input_image_size_num}" -lt 5 ]]; then 
		image_size="5G"
    fi
	echo "Rootfs image size = ${image_size}"

    if [ ! -d "${OUTPUT_ARTIFACTS_DIR}" ]; then
        mkdir -p Artifacts
    fi

    sudo ./mkosi/bin/mkosi -f \
    --bootable \
    --boot-protocols=linux \
    --format=gpt_ext4 \
    --distribution=ubuntu \
    --root-size=${image_size} \
    --output=${OUTPUT_ARTIFACTS_DIR}/image.ext4 \
    --architecture=aarch64 \
    --mirror=http://ports.ubuntu.com/ubuntu-ports \
    --package= ${deb_packages} \
    --password=root || fail "mkosi failed to build ubutnu"
}

# uncompress mkosi generate AArch64 compressed kernel image.
# args:
# $1: mkosi generated output artifacts dir
# $2: compressed kernel image name
uncompress_kernel_image() {
    if [ ! -d "$1" ]; then
        fail "can't find images dir: $1"
    fi  
    cd "$1" || fail "can't cd to $1"
    if [ -e Image ]; then
        cd ${INITIAL_WD}
        return
    fi
    gzip -dc "$2" | dd of=Image || fail "can't uncompress linux kernel image: $2"
    cd ${INITIAL_WD}
    echo "uncompressed AArch64 linux kernel image: $2"
}

# generate device tree source without chosen node.
# args:
# $1: template dts to generate ubuntu.dts from
generate_device_tree_src() {
    local template_dts="$1"
	if [ ! -e "${template_dts}" ]; then
		fail "ubuntu dts template is missing!"
	fi
	cat "${template_dts}" || fail "can't cat ${template_dts}"
}

# generate the chosen node of the device tree src.
# args:
# $1: mkosi artifacts dir
# $2: systemd boot target
# $3: initrd image 
generate_chosen_node() {
    local artifacts_dir="$1"
    local sd_boot_target="$2"
    local initrd_image="$3"
    local initrd_start_address=$(printf "<0x%X>" ${INITRD_START_ADDR})
    local size_of_initrd=$(stat --format="%s" "${artifacts_dir}/${initrd_image}")
    # initrd end address is the first address after the initrd image
    # https://www.kernel.org/doc/html/latest/devicetree/usage-model.html
    local initrd_end_address=$(printf "<0x%X>" $((${INITRD_START_ADDR} + ${size_of_initrd} + 1)))
    cat << EOF


        chosen {
		bootargs = "systemd.unit=${sd_boot_target} loglevel=8 root=LABEL=root rootfstype=ext4 rw";
		linux,initrd-start = ${initrd_start_address};
		linux,initrd-end = ${initrd_end_address};
		stdout-path = "/pl011@10000000";
		kaslr-seed = <0xdec2698b 0x996c3b6b>;
	};
};
EOF
}

# grant rw permission to artifacts folder
# args:
# $1: mkosi artifacts dir 
grant_rw_permission_to_artifacts() {
    echo "Granted rw permission to artifacts folder"
    local artifacts_dir="$1"
    sudo chmod -R +rw "${artifacts_dir}" || fail "can't chmod +rw ${artifacts_dir}"
}

# generate device tree source file.
# args:
# $1: mkosi artifacts dir 
# $2: systemd boot target
# $3: initrd image name
# $4: device tree source name
# $5: dts template
generate_dts() {
    local artifacts_dir="$1"
    local sd_boot_target="$2"
    local initrd_name="$3"
    local dts_name="$4"
	local dts_template="$5"
    local dts_without_chosen=$(generate_device_tree_src "${dts_template}")
    local chosen_entry=$(generate_chosen_node "${artifacts_dir}" "${sd_boot_target}" "${initrd_name}")
    local dts="${dts_without_chosen}${chosen_entry}"
    grant_rw_permission_to_artifacts "${artifacts_dir}"
    echo "${dts}" > ${artifacts_dir}/${dts_name} || fail "can't generate ${dts_name}"
    echo "Finished generating ${dts_name}"
}

# generate device tree blob file.
# args:
# $1: mkosi artifacts dir 
# $2: device tree source name
# $3: device tree blob name
generate_dtb() {
    local artifacts_dir="$1"
    local dts_name="$2"
    local dtb_name="$3"
    dtc -I dts -O dtb -o "${artifacts_dir}/${dtb_name}" "${artifacts_dir}/${dts_name}" > /dev/null 2>&1 \
    || fail "can't generate ${dtb_name}"
    echo "Finished generating ${dtb_name}"
}

# rename the large generated images to be tracked by git lfs
# args: 
# $1: mkosi artifacts dir 
rename_artifacts() {
	local artifacts_dir="$1"
	cd "${artifacts_dir}" || fail "can't cd to ${artifacts_dir}"
	mv Image Image.bin || fail "can't rename Image"
	mv image.ext4 image_ext4.img || fail "can't rename image.ext4"
	mv image.ext4.initrd image_ext4_initrd.img || fail "can't rename image.ext4.initrd"
	mv image.ext4.vmlinuz image_ext4_vmlinuz.bin || fail "can't rename image.ext4.vmlinuz" 
	cd ${INITIAL_WD}
	echo "Finished renaming large images"
}

# main.
# args: None
main() {
    check_needed_packages_installed
    clone_git_repo  "${MKOSI_GIT_REPO}" "${MKOSI_REPO_DIR}" 
    checkout_tag "${MKOSI_TAG}" "${MKOSI_REPO_DIR}" 
    patch_mkosi "$(get_patch_str)"
    do_mkosi_build "${DEB_PACKAGES_TO_ADD}" "${SD_TARGET}" "${ROOTFS_IMAGE_SIZE}"
    uncompress_kernel_image  "${OUTPUT_ARTIFACTS_DIR}" "${COMPRESSED_OUTPUT_KERNEL_IMAGE}"
    generate_dts "${OUTPUT_ARTIFACTS_DIR}" "${SD_TARGET}" "${INITRD_IMAGE}" "${DEVICE_TREE_SOURCE}" "${DEVICE_TREE_TEMPLATE}"
    generate_dtb "${OUTPUT_ARTIFACTS_DIR}" "${DEVICE_TREE_SOURCE}" "${DEVICE_TREE_BLOB}"
    rename_artifacts "${OUTPUT_ARTIFACTS_DIR}"
}

# run the script
main

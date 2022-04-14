
export `cat common/build.config.constants`
echo clang version: $CLANG_VERSION
set -e
CLANG=`find prebuilts -name clang-$CLANG_VERSION`

if [ -z $CLANG ]
then
  echo "clang not found"
  exit 1
fi

echo $CLANG
P=$PWD/$CLANG/bin
echo $P

output=$PWD/out/android-mainline/common
cd common
# Always touch main.c so generate a new uname timestamp.
touch init/main.c
PATH=$P:$PATH && make O=$output LLVM=1 LLVM_IAS=1 DEPMOD=depmod ARCH=arm64 Image -j 32 V=1 

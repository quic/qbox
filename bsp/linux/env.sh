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
export PATH=$P:$PATH

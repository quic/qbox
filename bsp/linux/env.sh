export `cat common/build.config.constants`
echo clang version: $CLANG_VERSION
set -e
CLANG=`find prebuilts -name clang-$CLANG_VERSION`

if [ -z $CLANG ]
then
  echo "clang not found"
fi

echo $CLANG
export PATH=$PWD/$CLANG/bin:$PATH

# electron-quick-start
svn checkout https://github.com/humphery755/DevTools/trunk/app/electron-quick-start

cnpm install -g electron-log
cnpm install -g node-gyp
cnpm install -g --production windows-build-tools
cnpm install -g iconv-lite
cnpm config set python python2.7
cnpm config set msvs_version 2017

# Go into the repository
cd electron-quick-start
# Install dependencies
cnpm install

cd node_modules/ffi
node-gyp rebuild -target="1.8.4" -arch=x64 -dist-url="https://atom.io/download/atom-shell"

cd node_modules/ref
node-gyp rebuild -target="1.8.4" -arch=x64 -dist-url="https://atom.io/download/atom-shell"

# cd native && set vcvars64.bat
vi compile.bat
# make dll
# run compile.bat
nmake

# Run the app
cnpm start



####################################################
#application-packaging
####################################################
cnpm install -g yarn
yarn add electron-builder --dev
cnpm install -g electron-builder
cnpm install electron-updater --save
cnpm install -g fix-path

yarn packg
yarn dist


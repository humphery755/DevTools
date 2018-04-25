# electron-quick-start

cnpm install -g node-gyp
cnpm install --g --production windows-build-tools
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

# set vcvars64.bat
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


cnpm run pack

如何进行更新？

改变package.json中的version属性，例如：改为 version: "1.0.1" (之前为1.0.0)；
再次执行electron-builder打包，将新版本latest.yml文件和exe文件放到package.json中build -> publish中的url对应的地址下；
在应用中触发更新检查，electron-updater自动会通过对应url下的yml文件检查更新；

const electron = require('electron')
// Module to control application life.
const app = electron.app
// Module to create native browser window.
const BrowserWindow = electron.BrowserWindow
const dialog = electron.dialog
const ipc = electron.ipcMain
// 注意这个autoUpdater不是electron中的autoUpdater
const autoUpdater = require('electron-updater').autoUpdater
const uploadUrl = "https://github.com/humphery755/DevTools/releases/download/electron-quick-start/"//require("../renderer/config/config").uploadUrl;

const path = require('path')
const url = require('url')


// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let mainWindow=null

const shouldQuit = app.makeSingleInstance(
      (commandLine, workingDirectory) => {
          if (mainWindow) {
              if (mainWindow.isMinimized()){
                   mainWindow.restore();
              };
          mainWindow.focus();
       };
 });
 if (shouldQuit) {
    app.quit();
    return;
 };

function createWindow () {
  // Create the browser window.
  mainWindow = new BrowserWindow({width: 800, height: 600})

  // and load the index.html of the app.
  mainWindow.loadURL(url.format({
    pathname: path.join(__dirname, 'renderer/index.html'),
    protocol: 'file:',
    slashes: true
  }))

  // Open the DevTools.
  mainWindow.webContents.openDevTools()

  // Emitted when the window is closed.
  mainWindow.on('closed', function () {
    // Dereference the window object, usually you would store windows
    // in an array if your app supports multi windows, this is the time
    // when you should delete the corresponding element.
    mainWindow = null
  })
  
  updateHandle()
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow)

// Quit when all windows are closed.
app.on('window-all-closed', function () {
  // On OS X it is common for applications and their menu bar
  // to stay active until the user quits explicitly with Cmd + Q
  if (process.platform !== 'darwin') {
    app.quit()
  }
})

app.on('activate', function () {
  // On OS X it's common to re-create a window in the app when the
  // dock icon is clicked and there are no other windows open.
  if (mainWindow === null) {
    createWindow()
  }
})

// In this file you can include the rest of your app's specific main process
// code. You can also put them in separate files and require them here.

// 检测更新，在你想要检查更新的时候执行，renderer事件触发后的操作自行编写
//Checking for update

function updateHandle(){
    let message={
      error:'检查更新出错',
      checking:'正在检查更新……',
      updateAva:'检测到新版本，正在下载……',
      updateNotAva:'现在使用的就是最新版本，不用更新',
    };
    const os = require('os');
    autoUpdater.logger = require("electron-log")
    autoUpdater.logger.transports.file.level = "info"
    autoUpdater.autoDownload = false
    autoUpdater.setFeedURL(uploadUrl);//'放最新版本文件的文件夹的服务器地址');
    autoUpdater.on('error', function(error){
      console.log(error);
      sendUpdateMessage(message.error)
    });
    autoUpdater.on('checking-for-update', function(info) {
      sendUpdateMessage(message.checking)
    });
    autoUpdater.on('update-available', function(info) {
      const options = {
        type: "info",
        title: "更新提示",
        message: "有新版本需要更新",
        buttons: ["现在更新","稍后"]
      }
      dialog.showMessageBox(options, function(index) {
        if (index == 0){
          sendUpdateMessage(message.updateAva)
          autoUpdater.downloadUpdate();
        }
      })
        
    });
    autoUpdater.on('update-not-available', function(info) {
        sendUpdateMessage(message.updateNotAva)
    });
    
    // 更新下载进度事件
    autoUpdater.on('download-progress', function(progressObj) {
        //console.log(progressObj);
        mainWindow.webContents.send('downloadProgress', progressObj)
    })
    autoUpdater.on('update-downloaded',  function (event, releaseNotes, releaseName, releaseDate, updateUrl, quitAndUpdate) {
        ipc.on('isUpdateNow', (e, arg) => {
            console.log(arg);
            console.log("开始更新");
            //some code here to handle event
            autoUpdater.quitAndInstall();
        })
        mainWindow.webContents.send('isUpdateNow')
    });
    ipc.on("checkForUpdate",()=>{
      //执行自动更新检查
      autoUpdater.checkForUpdates();
    })
}

// 通过main进程发送事件给renderer进程，提示更新信息
// mainWindow = new BrowserWindow()
function sendUpdateMessage(text){
    //console.log(text);
    mainWindow.webContents.send('message', text)
}



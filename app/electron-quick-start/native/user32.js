const ffi = require("ffi");
const path = require('path')
const child_process = require('child_process');
const iconv = require('iconv-lite');
const fixPath = require('fix-path');
/*
var ref = require('ref');
var Struct = require('ref-struct');
var winapi = {};
winapi.void = ref.types.void;
winapi.PVOID = ref.refType(winapi.void);
winapi.HANDLE = winapi.PVOID;
winapi.HWND = winapi.HANDLE;
winapi.WCHAR = ref.types.char;
winapi.LPCWSTR = ref.types.int;
winapi.UINT = ref.types.uint;
*/
exports.User32 = ffi.Library('user32', {
'GetWindowLongPtrW': ['int', ['int', 'int']],
'SetWindowLongPtrW': ['int', ['int', 'int', 'long']],
'GetSystemMenu': ['int', ['int', 'bool']],
'DestroyWindow': ['bool', ['int']]
});
var testdll = "native/libtestdll.dll";//path.join(__dirname, '../native/libtestdll.dll');
console.log(testdll);
exports.TestDLL = ffi.Library(testdll, {
'XAdd' : ['int',['int','int']]
});


fixPath();
exports.aria2c = function(url,target){
  //console.log(process.env.PATH);
  const cmd = 'native/aria2c.exe'
  //const cmd = 'cmd.exe'
  //const args = ['/c','start','native/aria2c.exe','-o',target,url]
  //const args = ["/c","dir"]
  const args = ['-o',target,"\""+url+"\""]
  var ls = child_process.spawn(cmd, args, {
      // stdio: 'inherit',
      //cwd: __dirname+"/../native"
    })
  //const ls = child_process.spawn('ls', ['-l', '/']);
  ls.stdout.on('data', (data) => {
    console.log(iconv.decode(new Buffer(data), 'GBK'))
  });

  ls.stderr.on('data', (data) => {
    //var str=`stderr: ${data}`
    console.log(iconv.decode(new Buffer(data), 'GBK'))
  });

  ls.on('close', (code) => {
    console.log(`child process exited with code ${code}`);
  });
};

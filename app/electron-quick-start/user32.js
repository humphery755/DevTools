const ffi = require("ffi");
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

exports.TestDLL = ffi.Library('./testdll.dll', {
'XAdd' : ['int',['int','int']]
});

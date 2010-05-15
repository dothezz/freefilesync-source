set argIn = WScript.Arguments
num = argIn.Count

if num = 0 then
    WScript.Echo "Call a batch file silently" & VbCrLf & VbCrLf &_
                 "Usage: WScript Invisible.vbs MyBatchfile.cmd <command line arguments>"
    WScript.Quit 1
end if

argOut = ""
for i = 0 to num - 1
    argOut = argOut & """" & argIn.Item(i) & """ "
next

set WshShell = WScript.CreateObject("WScript.Shell")

WshShell.Run argOut, 0, True
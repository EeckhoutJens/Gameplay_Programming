SetWorkingDir %A_ScriptDir%
Run, GPP_TEST_RELEASE.exe 30 30
WinActivate

Run, GPP_TEST_RELEASE.exe 1000 30
WinActivate

Run, GPP_TEST_RELEASE.exe 30 540
WinActivate

Run, GPP_TEST_RELEASE.exe 1000 540
WinActivate

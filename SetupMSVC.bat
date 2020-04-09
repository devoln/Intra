@echo off

REM Install debugger settings for Visual Studio

@FOR /F "tokens=3* delims= " %%a in ('reg query "HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" /v "Personal"') do @(Set UserDocuments=%%a)

if exist "%UserDocuments%\Visual Studio 2017\" (
	xcopy src\Intra\Intra.natstepfilter "%UserDocuments%\Visual Studio 2017\Visualizers\" /I /Y
)
pause

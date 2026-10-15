@echo off
mingw32-make %1
if errorlevel 1 goto err
echo Uploading...
type _DRX_synth.hex > COM1:
echo --- Done ---
goto end
:err
echo --- Error ---
:end

echo off
cls
echo +-----------------MDL BUILD BATCH ------------------------------+
cd
echo MDLVS.bat
date /t 
time /t
rem SET MS=C:\win32app\ustation
SET MS=C:\Bentley\Program\MicroStation
SET MS

call "C:\Program Files\Microsoft Visual Studio\VC98\Bin\vcvars32.bat"
SET INCLUDE=%MS%\mdl\include;%INCLUDE%
SET LIB=%MS%\mdl\library;%MS%\jmdl\lib;%LIB%
set BMAKE_OPT=-I%MS%\mdl\include -I%MS%\jmdl\include
set PATH=;%MS%;%MS%\mdl\bin;%MS%\jmdl\bin;%PATH%
set MLINK_STDLIB=%MS%\mdl\library\mdllib.dlo %MS%\mdl\library\builtin.dlo %MS%\mdl\library\toolsubs.dlo %MS%\mdl\library\mspsolid.dlo
echo +---------------------------------------------------------------+

rem  %MS%\mdl\bin\bmakewin -a %1
%MS%\mdl\bin\bmake -a %1

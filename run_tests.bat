@echo off
setlocal enabledelayedexpansion
set ROOT=%~dp0
pushd "%ROOT%"

if not exist "pic_test" (
  echo missing pic_test folder next to this script
  popd
  exit /b 1
)

echo [build] running make clean && make
make clean && make
if errorlevel 1 (
  echo build failed
  popd
  exit /b 1
)

set LIST_EXE=list_information.exe
if not exist "%LIST_EXE%" set LIST_EXE=list_information
set REC_EXE=recover_files.exe
if not exist "%REC_EXE%" set REC_EXE=recover_files
set MKFS_EXE=mkfs_qfs.exe
if not exist "%MKFS_EXE%" set MKFS_EXE=mkfs_qfs
set READ_EXE=read_file.exe
if not exist "%READ_EXE%" set READ_EXE=read_file
set WRITE_EXE=write_file.exe
if not exist "%WRITE_EXE%" set WRITE_EXE=write_file
set DEL_EXE=delete_file.exe
if not exist "%DEL_EXE%" set DEL_EXE=delete_file
set FAIL=0

if not exist "tests_output" mkdir "tests_output"

call :process_image qu_pics.img
call :process_image qu_pics_reformatted1.img
call :process_image qu_pics_reformatted2.img
call :round_trip

if %FAIL%==0 (
  echo ALL TESTS PASSED
) else (
  echo SOME TESTS FAILED
)

echo done
popd
endlocal
goto :eof

:process_image
set IMG=%1
if not exist "pic_test\%IMG%" (
  echo [warn] missing pic_test\%IMG%
  goto :eof
)
echo [info] listing %IMG%
"%LIST_EXE%" "pic_test\%IMG%" > "tests_output\%~n1_list.txt"

echo [recover] extracting jpgs from %IMG%
pushd tests_output
del /q "%~n1_*.jpg" 2>nul
del /q recovered_file_*.jpg 2>nul
"..\%REC_EXE%" "..\pic_test\%IMG%"
set count=1
for %%F in (recovered_file_*.jpg) do (
  ren "%%F" "%~n1_!count!.jpg"
  set /a count+=1
)
if exist "%~n1_1.jpg" (
  echo [pass] recovered at least one file from %IMG%
) else (
  echo [warn] no recovered files from %IMG% (may be expected)
)
popd
goto :eof

:round_trip
echo [mkfs] creating blank 4MB image
if exist "tests_output\mkfs.img" del /q "tests_output\mkfs.img"
powershell -command "$size=4MB; [IO.File]::WriteAllBytes('tests_output\\mkfs.img',(New-Object byte[] $size))"
"%MKFS_EXE%" "tests_output\mkfs.img"

echo [mkfs] validating empty superblock stats
"%LIST_EXE%" "tests_output\mkfs.img" > "tests_output\mkfs_list.txt"
findstr /c:"Free directory entries: 255" "tests_output\mkfs_list.txt" >nul 2>&1
if errorlevel 1 (
  echo [fail] mkfs list check
  set FAIL=1
) else (
  echo [pass] mkfs list check
)

echo [write] adding pic1.jpg
"%WRITE_EXE%" "tests_output\mkfs.img" "pic_test\pic1.jpg"

echo [read] extracting pic1.jpg to pic1_copy.jpg
"%READ_EXE%" "tests_output\mkfs.img" "pic1.jpg" "tests_output\pic1_copy.jpg"
fc /b "pic_test\pic1.jpg" "tests_output\pic1_copy.jpg" >nul 2>&1
if errorlevel 1 (
  echo [fail] read/write mismatch
  set FAIL=1
) else (
  echo [pass] read/write match
)

echo [delete] removing pic1.jpg
"%DEL_EXE%" "tests_output\mkfs.img" "pic1.jpg"

echo [verify] post-delete directory free count
"%LIST_EXE%" "tests_output\mkfs.img" > "tests_output\mkfs_post_delete.txt"
findstr /c:"Free directory entries: 255" "tests_output\mkfs_post_delete.txt" >nul 2>&1
if errorlevel 1 (
  echo [fail] delete did not free dir entry
  set FAIL=1
) else (
  echo [pass] delete freed dir entry
)
goto :eof


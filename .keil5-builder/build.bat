@echo off
START /WAIT "KeilBuild" "C:\Keil_v5\UV4\UV4.exe" -sg -j0 -r "Project\GD32F405RG.uvprojx" -o ".keil5-builder\build_log.txt"
exit %ERRORLEVEL%

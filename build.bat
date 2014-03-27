@echo ****************
@echo *** Building ***
@echo ****************
@arm-linux-gnueabihf-g++.exe *.c -o gpio-poll

@IF %ERRORLEVEL% NEQ 0 EXIT %ERRORLEVEL%

@echo *****************
@echo *** Deploying ***
@echo *****************
@scp -o UserKnownHostsFile=/dev/null -o StrictHostKeyChecking=no ./gpio-poll pi@192.168.2.111:~/
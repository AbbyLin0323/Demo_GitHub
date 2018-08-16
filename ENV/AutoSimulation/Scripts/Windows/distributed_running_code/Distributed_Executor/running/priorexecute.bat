@set projectpath=%1
@set NetStoredPath=%2

if exist %projectpath%\firmware (
  rd /s /q %projectpath%\firmware
)
if exist %projectpath%\model (
  rd /s /q %projectpath%\model
)
if exist %projectpath%\unit_test (
  rd /s /q %projectpath%\unit_test
)
if exist %projectpath%\ENV\checklist (
  rd /s /q %projectpath%\ENV\checklist
)
md %projectpath%\firmware
md %projectpath%\model
md %projectpath%\unit_test
md %projectpath%\ENV\checklist
xcopy %NetStoredPath%\firmware %projectpath%\firmware /y /s /q
xcopy %NetStoredPath%\model %projectpath%\model /y /s /q
xcopy %NetStoredPath%\unit_test %projectpath%\unit_test /y /s /q
xcopy %NetStoredPath%\ENV\checklist %projectpath%\ENV\checklist /y /s /q
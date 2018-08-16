@if exist %NetDiskPath%\firmware (
rd /s /q %NetDiskPath%\firmware
)
@if exist %NetDiskPath%\model (
rd /s /q %NetDiskPath%\model
)
@if exist %NetDiskPath%\unit_test (
rd /s /q %NetDiskPath%\unit_test
)
@if exist %NetDiskPath%\ENV\checklist (
rd /s /q %NetDiskPath%\ENV\checklist
)

@md %NetDiskPath%\firmware
@md %NetDiskPath%\model
@md %NetDiskPath%\unit_test
@md %NetDiskPath%\ENV\checklist
@xcopy %WorkSpace%\firmware %NetDiskPath%\firmware /s /e /y /q
@xcopy %WorkSpace%\model %NetDiskPath%\model /s /e /y /q
@xcopy %WorkSpace%\unit_test %NetDiskPath%\unit_test /s /e /y /q
@xcopy %WorkSpace%\ENV\checklist %NetDiskPath%\ENV\checklist /s /e /y /q
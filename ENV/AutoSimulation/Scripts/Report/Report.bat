@if %RunningorNot% equ 1 (
  goto notsend
)
::::::::::::::::::::::::::
::    Checkin log    
::::::::::::::::::::::::::
@echo Checkin log
@call %ReportScripts%\CheckinLog\Checkin.bat

:::::::::::::::::::::::::
::      Send email  
:::::::::::::::::::::::::
@echo Send email
@call %ReportScripts%\SendMail\SendMail.bat

:notsend
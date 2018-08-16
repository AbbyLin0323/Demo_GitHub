@set TraceLogDir=%cd:~0,-8%
@set FormatFile=D:\error\log_format_file12.ini
@set DataFile=D:\error\MCU1TL
@set TraceLogHeaderFile=D:\error\HAL_TraceLog.h

@if not exist %TraceLogDir%\Report (
  md %TraceLogDir%\Report
)
@Decoder.exe %FormatFile% %DataFile% %TraceLogHeaderFile% %TraceLogDir%

@pause
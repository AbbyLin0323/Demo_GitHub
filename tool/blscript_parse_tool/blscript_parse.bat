::exe have three parameters.
:: the parameter_1: 0 -- transfer *.cfg file to *.bin file;
::                  1 -- transfer *.bin file to *.cfg file.

:: for *.cfg --> *.lst:
:: the parameter_2: the root path.
::                  if NULL, will set the root path to be the diretory of the EXE.
::                  if NULL, the following parameters move forward, that's parameter_2 and parameter_3.
:: the parameter_3: 4K parameter binary file name which will be generated.
:: the parameter_4: the relative or absolute path of the "main.cfg".
::                  if relative path, it's relative to the root path. 
::                  the relative path can be NULL, it means that the \"main.cfg\" is located in the same
::                  diretory with the root path.

:: for *.bin --> *.cfg:
:: the parameter_2: the root path.
::                  if NULL, will set the root path to be the diretory of the EXE.
::                  if NULL, the following parameters move forward, that's parameter_2 and parameter_3.
:: the parameter_3: 4K parameter binary file name which will be parsed to the config file;
                    the binary file must in the "root path\\ouput\" diretory..
:: the parameter_4: the parse config file name which will be generated.
::                  the config file will be in "root path\ouput\" diretory.

bl_parameter_compile.exe 0 BootLoader_Parameter.bin \FPGA

::bl_parameter_compile.exe 1 BootLoader_Parameter.bin parse.cfg


exit
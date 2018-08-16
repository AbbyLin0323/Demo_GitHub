@echo Xtmp Make Start
:::::::::::::::::::::::::
::    Defination
:::::::::::::::::::::::::
@if %RunningorNot% equ 1 (
  goto notxtmake
)
@set XtmpSimLog=%AutoSimLog%\Xtmp

@echo Create XtmpSimLog
@if not exist %XtmpSimLog% (
  md %XtmpSimLog%
)

@echo Prepare for Make
@c:
@call %LicenceBat%
@call %BatPath% %CorePath% %SwTools% %XtensaSystem% %CoreName%

@set XtmpWorkSpace=%WorkSpace%

@%PSDriver%
@cd %XtmpWorkSpace%

xt-make dep >%XtmpSimLog%\dep.txt 2>&1
xt-make sata_l85 >%XtmpSimLog%\sata_l85_compilelog.txt 2>&1
xt-make sata_l95 >%XtmpSimLog%\sata_l95_compilelog.txt 2>&1
xt-make sata_tsb >%XtmpSimLog%\sata_tsb_compilelog.txt 2>&1
xt-make sata_tsb_fourpln >%XtmpSimLog%\sata_tsb_fourpln_compilelog.txt 2>&1
xt-make ahci_l85 >%XtmpSimLog%\ahci_l85_compilelog.txt 2>&1
xt-make ahci_l95 >%XtmpSimLog%\ahci_l95_compilelog.txt 2>&1
xt-make ahci_tsb >%XtmpSimLog%\ahci_tsb_compilelog.txt 2>&1
xt-make ahci_tsb_fourpln >%XtmpSimLog%\ahci_tsb_fourpln_compilelog.txt 2>&1
xt-make nvme_l85 >%XtmpSimLog%\nvme_l85_compilelog.txt 2>&1
xt-make nvme_l95 >%XtmpSimLog%\nvme_l95_compilelog.txt 2>&1
xt-make nvme_tsb >%XtmpSimLog%\nvme_tsb_compilelog.txt 2>&1
xt-make nvme_tsb_fourpln >%XtmpSimLog%\nvme_tsb_fourpln_compilelog.txt 2>&1
xt-make bl FLASH_IN=L85 >%XtmpSimLog%\BootLoader_L85_compilelog.txt 2>&1
xt-make bl FLASH_IN=L95 >%XtmpSimLog%\BootLoader_L95_compilelog.txt 2>&1
xt-make bl FLASH_IN=TSB >%XtmpSimLog%\BootLoader_TSB_compilelog.txt 2>&1
xt-make bl FLASH_IN=TSB_FOURPLN >%XtmpSimLog%\BootLoader_TSB_FOURPLN_compilelog.txt 2>&1

:notxtmake
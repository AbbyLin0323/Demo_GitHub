$autosim = $args[0]
$logtime = $args[1]
$informationofproj = $args[2]
$priorityflag = 0
$compileheadaddflag = 0
$Bootloaderorfirmware = 0
$otherparafolder = $autosim + "\Scripts\Parameters"
$gitlogpath = $autosim + "\AutoSimLog\gitlog.txt"
$informationofprojcontent = get-content -path $informationofproj
#commonpara中获取参数
foreach($line in $informationofprojcontent)
{
    if($line.Contains("set MailTheme="))
    {
        $mailtheme = $line.Replace("set MailTheme=","")
    }
    elseif($line.Contains("set DestinationBox="))
    {
        $destinationbox = $line.Replace("set DestinationBox=","")
    }
    elseif($line.Contains("set CcBox="))
    {
        $ccbox = $line.Replace("set CcBox=","")
    }
    elseif($line.Contains("set ProjectNameofTotal="))
    {
        $ProjectNameofTotal = $line.Replace("set ProjectNameofTotal=","")
    }
    elseif($line.Contains("set GitPath="))
    {
        $gitdirectory = $line.Replace("set GitPath=","")
    }
    elseif($line.Contains("set GitBranchName="))
    {
        $gitbranchname = $line.Replace("set GitBranchName=","")
    }
    elseif($line.Contains("set PclintSwitch="))
    {
        $PclintSwitch = $line.Replace("set PclintSwitch=","")
    }
    elseif($line.Contains("set SimianSwitch="))
    {
        $SimianSwitch = $line.Replace("set SimianSwitch=","")
    }
    elseif($line.Contains("set DestinationPath="))
    {
        $destinationpath = $line.Replace("set DestinationPath=","")
    }
    elseif($line.Contains("set SourceBox="))
    {
        $sourcebox = $line.Replace("set SourceBox=","")
    }
    elseif($line.Contains("set SourceBoxKey="))
    {
        $key = $line.Replace("set SourceBoxKey=","")
    }
}
$body = ""
$body += $ProjectNameofTotal + " Simulation Result:`n"
#log information
if(Test-Path -Path $gitlogpath)
{
    $gitcontent = get-content -Path $gitlogpath
    $body += "`ngit latest log:`n"
    $body += "`t" + $gitcontent[0] + "`n"
    $body += "`t" + $gitcontent[1] + "`n"
    $body += "`t" + $gitcontent[2] + "`n"
    $body += "`t" + $gitcontent[3] + "`n"
    $body += "`ngit directory:  " + $gitdirectory
    $body += "`ngit branch:`t" + $gitbranchname + "`n`n"
}
#compile result
$winlogfilespath = $autosim + "\AutoSimLog\Windows"
if (Test-Path -Path $winlogfilespath)
{
    $winlogfiles = get-childitem -recurse $winlogfilespath "*compilelog.txt"
}
if($winlogfiles)
{
    #start of this cycle
    $body += "compile result:`n"
    $body += "`tMode`t`t`tENV`t`t`t`t`tResult`t`tError(s)`tWarning(s)`n"
    $compileheadaddflag = 1
    
    foreach($winlogfile in $winlogfiles)
    {
        if ($winlogfile.fullname.Contains("AHCI_WholeChip_l1_fake"))
        {
            $body += "`tAHCI`t  WholeChip_Win_L1_FAKE`t`t"
        }
        elseif ($winlogfile.fullname.Contains("AHCI_WholeChip_l2_fake"))
        {
            $body += "`tAHCI`t  WholeChip_Win_L2_FAKE`t`t"
        }
        elseif ($winlogfile.fullname.Contains("AHCI_WholeChip"))
        {
            $body += "`tAHCI`t`tWholeChip_Win`t`t`t"
        }
        elseif ($winlogfile.fullname.Contains("SATA_WholeChip_l1_fake"))
        {
            $body += "`tSATA`t  WholeChip_Win_L1_FAKE`t`t"
        }
        elseif ($winlogfile.fullname.Contains("SATA_WholeChip_l2_fake"))
        {
            $body += "`tSATA`t  WholeChip_Win_L2_FAKE`t`t"
        }
        elseif ($winlogfile.fullname.Contains("SATA_WholeChip"))
        {
            $body += "`tSATA`t`tWholeChip_Win`t`t`t"
        }
        elseif ($winlogfile.fullname.Contains("NVMe_WholeChip_l1_fake"))
        {
            $body += "`tNVMe`t  WholeChip_Win_L1_FAKE`t`t"
        }
        elseif ($winlogfile.fullname.Contains("NVMe_WholeChip_l2_fake"))
        {
            $body += "`tNVMe`t  WholeChip_Win_L2_FAKE`t`t"
        }
        elseif ($winlogfile.fullname.Contains("NVMe_WholeChip"))
        {
            $body += "`tNVMe`t`tWholeChip_Win`t`t`t"
        }
        $winlogfilecontent = get-content -Path $winlogfile.fullname
        foreach($line in $winlogfilecontent)
        {
            if($line.Contains("Build succeeded.") -or $line.Contains("已成功生成"))
            {
                $body += "succeeded" + "`t"
                break
            }
            elseif($line.Contains("Build FAILED.") -or $line.Contains("生成失败"))
            {
                 $body += "Failed!" + "`t`t"
                 $priorityflag = 1
                 break
            }
        }
        $body += $winlogfilecontent[-3].Replace("Error(s)","").Replace(" ","").Replace("个错误","") + "`t`t"
        $body += $winlogfilecontent[-4].Replace("Warning(s)","").Replace(" ","").Replace("个警告","") + "`n"
    }
}
$xtlogfilespath = $autosim + "\AutoSimLog\Xtmp"
$xtmpworkspacepath = $autosim + "\WorkSpace\Bin\bin"
#获取log信息
if(Test-Path -Path $xtlogfilespath)
{
    $xtlogfiles = get-childitem -path $xtlogfilespath "*compilelog.txt"
}
if($xtlogfiles)
{
    if ($compileheadaddflag -eq 0)
    {
        $body += "compile result:`n"
        $body += "`tMode`t`t`tENV`t`t`t`t`tResult`t`tError(s)`tWarning(s)`n"
    }
    foreach($xtlogfile in $xtlogfiles)
    {
        if ($xtlogfile.fullname.Contains("sata_l85"))
        {
            $body += "`tSATA`t`t  xtensa L85`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("sata_l95"))
        {
            $body += "`tSATA`t`t  xtensa L95`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("sata_tsb_fourpln"))
        {
            $body += "`tSATA`t  xtensa TSB_FOURPLN`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("sata_tsb"))
        {
            $body += "`tSATA`t`t  xtensa TSB`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("ahci_l85"))
        {
            $body += "`tAHCI`t`t  xtensa L85`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("ahci_l95"))
        {
            $body += "`tAHCI`t`t  xtensa L95`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("ahci_tsb_fourpln"))
        {
            $body += "`tAHCI`t  xtensa TSB_FOURPLN`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("ahci_tsb"))
        {
            $body += "`tAHCI`t`t  xtensa TSB`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("nvme_l85"))
        {
            $body += "`tNVMe`t`t  xtensa L85`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("nvme_l95"))
        {
            $body += "`tNVMe`t`t  xtensa L95`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("nvme_tsb_fourpln"))
        {
            $body += "`tNVMe`t  xtensa TSB_FOURPLN`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("nvme_tsb"))
        {
            $body += "`tNVMe`t`t  xtensa TSB`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("BootLoader_L85"))
        {
            $body += "  BootLoader`t`t`tL85`t`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("BootLoader_L95"))
        {
            $body += "  BootLoader`t`t`tL95`t`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("BootLoader_TSB_FOURPLN"))
        {
            $body += "  BootLoader`t`tTSB_FOURPLN`t`t`t`t"
        }
        elseif ($xtlogfile.fullname.Contains("BootLoader_TSB"))
        {
            $body += "  BootLoader`t`t`tTSB`t`t`t`t`t"
        }
        $xtmpcompilelogcontent = get-content -path $xtlogfile.fullname
        $xtmpcompileerrors = 0
        $xtmpcompilewarnings = 0
        foreach($line in $xtmpcompilelogcontent)
        {
            if($line.Contains("warning:"))
            {
                $xtmpcompilewarnings++
            }
            elseif($line.Contains("error:"))
            {
                $xtmpcompileerrors++
            }
            elseif($line.Contains("WARNING:"))
            {
                $xtmpcompilewarnings++
            }
            elseif($line.Contains("ERROR:"))
            {
                $xtmpcompileerrors++
            }
            elseif($line.Contains("***"))
            {
                $xtmpcompileerrors++
            }
        }
        if ($xtmpcompileerrors -eq 0)
        {
            $body += "Succeeded"
        }
        else
        {
            $body += "Failed!`t"
            $priorityflag = 1
        }
        $body += "`t" + $xtmpcompileerrors + "`t`t" + $xtmpcompilewarnings + "`n"
    }
}
#end of xplorer
if ($PclintSwitch -eq "ON")
{
    $pclintpath = $autosim + "\AutoSimLog\Pclint"
    if(Test-Path -Path $pclintpath)
    {
        $pclintfiles = get-childitem -path $pclintpath "*_pclintlog.txt"
    }
    if($pclintfiles)
    {
        $body += "`n"
        $body += "pclint result:`n"
        $body += "`tSort`t`t`t`tError(s)`t`tWarning(s)`tInfo(s)`n"
        foreach ($pclintfile in $pclintfiles)
        {
            $pclintcontent = get-content -Path $pclintfile.fullname
            $Warnings = 0
            $Infos = 0
            $Errors = 0
            foreach($line in $pclintcontent)
            {
                if($line -match "[0-9]+: Warning")
                {
                    $Warnings++
                }
                elseif($line -match "[0-9]+: Info")
                {
                    $Infos++ 
                }
                elseif($line -match "[0-9]+: Error")
                {
                    $Errors++ 
                }
            }
            if ($pclintfile.fullname.Contains("Adapter_SATA"))
            {
                $body += " Adpter_SATA`t"
            }
            elseif ($pclintfile.fullname.Contains("Adapter_AHCI"))
            {
                $body += " Adpter_AHCI`t"
            }
            if ($pclintfile.fullname.Contains("Adapter_NVMe"))
            {
                $body += " Adpter_NVMe"
            }
            elseif ($pclintfile.fullname.Contains("Algorithm_L0"))
            {
                $body += " Algorithm_L0"
            }
            elseif ($pclintfile.fullname.Contains("Algorithm_L1"))
            {
                $body += " Algorithm_L1"
            }
            elseif ($pclintfile.fullname.Contains("Algorithm_L2"))
            {
                $body += " Algorithm_L2"
            }
            if ($pclintfile.fullname.Contains("Algorithm_L3"))
            {
                $body += " Algorithm_L3"
            }
            elseif ($pclintfile.fullname.Contains("HAL"))
            {
                $body += "`tHAL`t`t"
            }
            elseif ($pclintfile.fullname.Contains("Others"))
            {
                $body += "  Others`t`t"
            }
            $body += "`t`t" + $Errors + "`t`t`t" + $Warnings + "`t`t`t" + $Infos + "`n"
        }
    }
}
if ($SimianSwitch -eq "ON")
{
    $simianpath = $autosim + "\AutoSimLog\simianlog.txt"
    if(Test-Path -Path $simianpath)
    {
        if ($simianheadaddflag -eq 0)
        {
            $body += "`n"
            $body += "simian result:`n"
            $simianheadaddflag = 1
        }
        $simiancontent = get-content -Path $simianpath.fullname
        $body += "`t" + $simiancontent[-3] + "`n"
    }
}
$logfilename = $logtime.Replace('/',"_")
$logfilename = $logfilename.Replace(':',"_")
$logfilename = $logfilename.Replace(".","_")
$logfilefullname = $destinationpath + "\" + $ProjectNameofTotal + "_" + $logfilename
$body += "`n" + "More details please refer to : " + $logfilefullname + "`n"
$body += "`n" + "This mail is sent automatically!"
$smtpServer = "mailbj.viatech.com.cn"
$smtpUser = $sourcebox
$smtpPassword = $key
$mail = New-Object System.Net.Mail.MailMessage
$MailAddress= $sourcebox
[string[]]$Addresses = [regex]::split($destinationbox,':')
$mail.From = New-Object System.Net.Mail.MailAddress($MailAddress)
foreach($Address in $Addresses)
{
    $mail.To.Add($Address)
}
[string[]]$ccs = [regex]::split($ccbox,':')
foreach($cc in $ccs)
{
    $mail.CC.Add($cc)
}
$mail.Subject = "[auto] " + $mailtheme.Replace('+'," ")
if($priorityflag -eq 1)
{
    $mail.Priority  = "High"
}
$mail.Body = $body
$smtp = New-Object System.Net.Mail.SmtpClient -argumentList $smtpServer
$smtp.Credentials = New-Object System.Net.NetworkCredential -argumentList $smtpUser,$smtpPassword
$smtp.Send($mail)
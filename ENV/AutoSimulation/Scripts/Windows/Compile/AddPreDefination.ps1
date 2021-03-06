$windowsworkspace = $args[0]
$folder = $args[1]
$flashtype = $args[2]
$firmwarefolderpath = $args[3]
$filevcxproj = Get-ChildItem $windowsworkspace -Recurse *.vcxproj
$filefilters = Get-ChildItem $windowsworkspace -Recurse *.filters
#建立备份路径和记录路径，保存vcxproj文件和filters文件的最原始形态和每次使用过后的形态
$vcxproj = $filevcxproj.fullname
$filters = $filefilters.fullname
$vcxprojpath = split-path $vcxproj
#由于此脚本应用场景为WinSim.bat，所以这里不考虑workspace建立在驱动器根目录的情况
$BackupPath = $folder + "\CompileBackup"
$RecordPath = $folder + "\CompileRecord"
$Backvcxproj = $BackupPath + "\" + $filevcxproj
$Backfilters = $BackupPath + "\" + $filefilters
$Recordvcxproj = $RecordPath + "\" + $filevcxproj
$Recordfilters = $RecordPath + "\" + $filefilters
$replace = ""
$flag = 0
if (!(Test-Path -Path $BackupPath))
{
    New-Item -path $folder -name "CompileBackup" -type "directory"
}
if (!(Test-Path -Path $RecordPath))
{
    New-Item -path $folder -name "CompileRecord" -type "directory"
}
if (!(Test-Path -Path $Backvcxproj))
{
    Copy-Item $vcxproj -destination $BackupPath
    Copy-Item $filters -destination $BackupPath
}
#每次对文件操作前都将vcxproj文件和filters文件替换为最原始形态
Remove-Item -path $vcxproj
Remove-Item -path $filters
Copy-Item $Backvcxproj -destination $vcxprojpath
Copy-Item $Backfilters -destination $vcxprojpath
if($($args[4]).ToLower() -eq "`/d")
{
    $flag = 1
    for($i = 5;$i-lt$args.Count;$i++)
    {
        $replace += $($args[$i]) + ";"
    }
}
if ($flag -eq 1)
{
    $content = get-content -Path $filevcxproj.fullname
    clear-content -Path $filevcxproj.fullname
    Foreach($line in $content)
    {
        if ($line.Contains("`<PreprocessorDefinitions`>"))
        {
            $liner = $line.Replace("`<PreprocessorDefinitions`>","`<PreprocessorDefinitions`>$replace")
        }
        else
        {
            $liner = $line
        }
        Add-content $filevcxproj.fullname -Value $liner
    }
}
#当文件名中存在空格、/、：的时候会被识别为设备名称
$Date = ((Get-Date).ToString()).Replace("`/","_")
$Date = $Date.Replace(" ","_")
$Date = $Date.Replace(":","_")
$DateName1 = $Date + ".vcxproj"
$DateName2 = $Date + ".vcxproj.filters"
#在对文件操作完成之后保存到record文件中备份，并用当前时间命名
Copy-Item $vcxproj -destination $RecordPath
Copy-Item $filters -destination $RecordPath
Rename-Item -Path $Recordvcxproj -NewName $DateName1
Rename-Item -Path $Recordfilters -NewName $DateName2
#拷贝Flash文件
$flashsourcepath = $firmwarefolderpath + "\firmware\HAL\Flash\" + $flashtype
$flashdestpath = $firmwarefolderpath + "\firmware\HAL\Flash"
Get-ChildItem $flashdestpath *.c | Remove-Item
Get-ChildItem $flashdestpath *.h | Remove-Item
Get-ChildItem $flashsourcepath *.c | %{Copy-Item $_.fullname -destination $flashdestpath}
Get-ChildItem $flashsourcepath *.h | %{Copy-Item $_.fullname -destination $flashdestpath}
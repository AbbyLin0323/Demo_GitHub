$wholeprojectlog = $args[0]
$destinationpath = $args[1]
$logtime = $args[2]
$nameoftotalprocect = $args[3]
$workspace = $args[4]
$logfiles = $wholeprojectlog + "\*"
$gitpaths = get-childitem -path $wholeprojectlog -recurse "gitlist.txt"
$pclintpaths = get-childitem -path $wholeprojectlog -recurse "pclintpath.txt"
if($gitpaths)
{
    foreach($gitpath in $gitpaths)
    {
        remove-item -path $gitpath.fullname -force
    }
}
if($pclintpaths)
{
    foreach($pclintpath in $pclintpaths)
    {
        remove-item -path $pclintpath.fullname -force
    }
}
$gitlogpath = $wholeprojectlog + "\gitlog.txt"
$gitlogcontent = get-content -path $gitlogpath
$gitversion = $gitlogcontent[0][7] + $gitlogcontent[0][8] + $gitlogcontent[0][9] + $gitlogcontent[0][10] + $gitlogcontent[0][11] + $gitlogcontent[0][12]
$logfilename = $logtime.Replace('/',"_")
$logfilename = $logfilename.Replace(':',"_")
$logfilename = $logfilename.Replace(".","_")
$logfilename = $nameoftotalprocect + "_" + $logfilename
New-Item -Path $destinationpath -name $logfilename -type "directory"
$logfilepath = $destinationpath + "\" + $logfilename
Copy-Item $logfiles -destination $logfilepath -recurse
#copy bin
#$binexistpath = $workspace + "\Bin\bin"
#if (Test-Path -Path $binexistpath)
#{
#    $binsourcepath = $workspace + "\Bin\bin\*"
#    $bindestpath = $logfilepath + "\bin_" + $gitversion
#    Copy-Item $binsourcepath -destination $bindestpath -recurse
#}
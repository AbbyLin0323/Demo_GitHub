$tasklogpath = $args[0]
$file1 = $args[1]
$content1 = Get-Content -Path $file1
Clear-Content -Path $file1
$info1 = $content1[0].Substring(8,15)
$filepath = $tasklogpath + "\gitlog.txt"
$result = 0
if (Test-Path -Path $filepath)
{
    $content2 = Get-Content -Path $filepath
    $info2 = $content2[0].Substring(8,15)
    if($info1 -eq $info2)
    {
        $result = 1
    }
    else
    {
        $result = 0
    }
}
else
{
    $result = 0
}
Add-Content $file1 -Value $result
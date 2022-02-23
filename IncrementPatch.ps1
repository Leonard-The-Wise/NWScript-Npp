(Get-Content -path $args[0] -Raw) |
    ForEach-Object {
        $defstr="#define VERSION_PATCH ";
        $regex="$defstr(?<PatchVersion>\d*)";
        if($_ -match $regex) {
            $_ = $_ -replace $regex,"$($defstr)$(([int]$matches["PatchVersion"])+1)" 
        }
        $_
    } |
    Out-File $args[0] -encoding ascii -nonewline
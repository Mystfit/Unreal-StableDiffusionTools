param(
    [Parameter(Mandatory=$true)]
    [string]$InputFolder,

    [Parameter(Mandatory=$true)]
    [string]$IgnorePathsFile
)

# Get the name of the top level directory
$rootName = Get-ChildItem -Path $InputFolder -Directory | select Name

# Read the ignore paths from the file
$ignorePaths = Get-Content -Path $IgnorePathsFile

# Get the version number from the .uplugin file
$upluginFile = Get-ChildItem -Path $InputFolder -Filter "*.uplugin" -Recurse | Select-Object -First 1
if ($upluginFile -eq $null) {
    Write-Host "No .uplugin file found in the input folder."
    Exit
}

$upluginJson = Get-Content -Path $upluginFile.FullName | ConvertFrom-Json
$versionNumber = $upluginJson.VersionName

# Create the output zip file name
$zipFileName = "{0}-{1}.zip" -f $InputFolder, $versionNumber

# Create a temporary folder for the contents to be zipped
$tempFolder = Join-Path -Path $env:TEMP -ChildPath ([System.Guid]::NewGuid().ToString())
$tempFolderRoot = Join-Path -Path $tempFolder -ChildPath $rootName

# Copy the contents of the input folder to the temporary folder, excluding the specified paths
Copy-Item -Path $InputFolder -Destination $tempFolderRoot -Recurse -Exclude $ignorePaths

# Compress the contents of the temporary folder to a zip file
Compress-Archive -Path $tempFolder\* -DestinationPath $zipFileName -CompressionLevel Optimal -Force

# Clean up the temporary folder
Remove-Item -Path $tempFolder -Recurse -Force

Write-Host "Folder '$InputFolder' compressed to '$zipFileName'."

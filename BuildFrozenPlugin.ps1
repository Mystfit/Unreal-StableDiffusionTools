param(
    [Parameter(Mandatory=$true)]
    [string]$InputFolder,

    [Parameter(Mandatory=$true)]
    [string]$IgnorePathsFile
)

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

# Create a new root folder within the temporary folder
$rootFolder = Join-Path -Path $tempFolder -ChildPath (Split-Path -Leaf $InputFolder)
New-Item -ItemType Directory -Path $rootFolder | Out-Null

# Copy the contents of the input folder to the root folder, excluding the specified paths
Copy-Item -Path $InputFolder\* -Destination $rootFolder -Recurse -Exclude $ignorePaths

# Create the 7zip command
$sevenZipExe = "7z.exe"  # Replace with the actual path to 7zip's command line program
$splitSize = "1990m"  # Split the zip file into parts of approximately 1.99 GB each
$sevenZipCommand = "$sevenZipExe a -tzip -v$splitSize `"$zipFileName`" `"$tempFolder\*`""

# Execute the 7zip command
Invoke-Expression -Command $sevenZipCommand

# Clean up the temporary folder
Remove-Item -Path $tempFolder -Recurse -Force

Write-Host "Folder '$InputFolder' compressed to '$zipFileName' and split into parts."

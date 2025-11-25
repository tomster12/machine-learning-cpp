$ErrorActionPreference = "Stop"

$SFML_VERSION = "3.0.0"
$ARCH = "64"
$ZIP = "SFML-$SFML_VERSION-windows-vc17-$ARCH-bit.zip"
$URL = "https://www.sfml-dev.org/files/$ZIP"
$DIR = "SFML-$SFML_VERSION"

$Projects = @(
    "TBMLGeneticAlgorithm",
    "TBMLMNISTDrawer"
)

Write-Host "Downloading SFML..."
Invoke-WebRequest -Uri $URL -OutFile $ZIP

Write-Host "Extracting..."
Expand-Archive -Path $ZIP -DestinationPath "." -Force

if (-not (Test-Path $DIR)) {
    throw "Extraction didn't produce expected folder: $DIR"
}

Write-Host "Creating dependency directories..."
New-Item -ItemType Directory -Force -Path "dependencies/SFML/include" | Out-Null
New-Item -ItemType Directory -Force -Path "dependencies/SFML/lib" | Out-Null

Write-Host "Copying includes + libs..."
Copy-Item "$DIR/include/*" "dependencies/SFML/include" -Recurse -Force
Copy-Item "$DIR/lib/*"     "dependencies/SFML/lib"     -Recurse -Force

Write-Host "Copying DLLs to each project..."
foreach ($p in $Projects) {
    $DebugPath = "bin/$p/x64/output/Debug"
    $ReleasePath = "bin/$p/x64/output/Release"

    New-Item -ItemType Directory -Force -Path $DebugPath   | Out-Null
    New-Item -ItemType Directory -Force -Path $ReleasePath | Out-Null

    Get-ChildItem "$DIR/bin/*.dll" | ForEach-Object {
        if ($_.Name -match "-d-") {
            Copy-Item $_.FullName $DebugPath -Force
        }
        else {
            Copy-Item $_.FullName $ReleasePath -Force
        }
    }
}

Write-Host "Cleaning..."
Remove-Item $DIR -Recurse -Force
Remove-Item $ZIP -Force

Write-Host "SFML setup complete."

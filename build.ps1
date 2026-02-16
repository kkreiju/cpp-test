# NCTV Player Auto-Build & Deploy Script
Write-Host "=== NCTV Player C++ Build Pipeline ===" -ForegroundColor Green

$ProjectRoot = Get-Location
$BuildOutDir = Join-Path $ProjectRoot "build"
$AptlyDir = Join-Path $ProjectRoot "aptly"

# Ensure build output directory exists
if (!(Test-Path $BuildOutDir)) { New-Item -Path $BuildOutDir -ItemType Directory }

# 2. Run the Compilers
Write-Host "[1/4] Compiling and Packaging .deb (Multi-Arch)..." -ForegroundColor Cyan

# Define architectures to build
$Architectures = @("linux/arm64", "linux/arm/v7")

foreach ($Arch in $Architectures) {
    Write-Host ">>> Preparing Build Engine for $Arch..." -ForegroundColor Yellow
    
    # We build the builder image specifically for each architecture
    # Use a specific tag per arch to avoid multi-arch local storage issues
    $Tag = "nctv-builder-" + $Arch.Replace("/", "-")
    docker build --platform $Arch -t $Tag -f Dockerfile.build .
    
    if ($LASTEXITCODE -ne 0) { throw "Docker build failed for $Arch" }

    Write-Host ">>> Compiling for $Arch..." -ForegroundColor Cyan
    
    docker run --rm --platform $Arch `
        -v "${ProjectRoot}:/app" `
        -v "${BuildOutDir}:/app/build-output" `
        $Tag

    if ($LASTEXITCODE -ne 0) { 
        Write-Host "FAILED build for $Arch" -ForegroundColor Red
        continue # Move to next architecture instead of stopping entirely
    }

    # Move newly created .deb files to Aptly folder immediately
    $NewDebs = Get-ChildItem "*.deb"
    if ($NewDebs.Count -gt 0) {
        foreach ($File in $NewDebs) {
            Move-Item $File.FullName (Join-Path $AptlyDir $File.Name) -Force
            Write-Host "Successfully staged: $($File.Name)" -ForegroundColor Green
        }
    }
}

# 3. Start/Restart Aptly
# Check if we have any packages in the aptly folder before proceeding
if ((Get-ChildItem $AptlyDir -Filter *.deb).Count -gt 0) {
    Write-Host "[3/4] Updating Aptly Repository..." -ForegroundColor Cyan
    Set-Location $AptlyDir

    Write-Host "Stopping Aptly container..." -ForegroundColor Cyan
    docker-compose down

    Write-Host "Starting Aptly container with rebuild..." -ForegroundColor Cyan
    docker-compose up -d --build

    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Aptly container start failed!" -ForegroundColor Red
        exit 1
    }

    Write-Host "Aptly repository updated successfully!" -ForegroundColor Green
} else {
    Write-Host "ERROR: Aptly directory not found at $AptlyDir" -ForegroundColor Red
    exit 1
}

# Return to project root
Write-Host "Returning to project root..." -ForegroundColor Yellow
Set-Location $ProjectRoot

Write-Host "=== Build and Deploy Complete ===" -ForegroundColor Green

param (
    [string[]]$args  # Capture all arguments
)

# Initialize options
$keep = $false
$force = $false
$file = $null

# Parse arguments
foreach ($arg in $args) {
    switch ($arg) {
        "-k" { $keep = $true }
        "-f" { $force = $true }
        default { $file = $arg }
    }
}

# Validate input file
if (-not $file) {
    Write-Host "gzip: missing file operand" -ForegroundColor Red
    exit 1
}

if (!(Test-Path $file)) {
    Write-Host "gzip: $($file): No such file or directory" -ForegroundColor Red
    exit 1
}

# Determine output file name
$outputFile = "$file.gz"

# Check if output file exists
if (Test-Path $outputFile) {
    if (-not $force) {
        Write-Host "gzip: $($outputFile) already exists. Use -f to overwrite." -ForegroundColor Red
        exit 1
    }
    Remove-Item -Force $outputFile
}

# Compress the file
try {
    $inputStream = [System.IO.File]::OpenRead($file)
    $outputStream = [System.IO.Compression.GZipStream]::new(
        [System.IO.File]::Create($outputFile),
        [System.IO.Compression.CompressionMode]::Compress
    )
    $inputStream.CopyTo($outputStream)
    $outputStream.Close()
    $inputStream.Close()

    # Remove the original file unless -k (keep) is specified
    if (-not $keep) {
        Remove-Item -Force $file
    }
    Write-Host "gzip: $($file) compressed to $($outputFile) successfully."
} catch {
    Write-Host "gzip: An error occurred while compressing $($file)." -ForegroundColor Red
    exit 1
}

param (
    [string[]]$args  # Capture all arguments
)

# Initialize options
$keep = $false
$force = $false
$stdout = $false
$file = $null

# Parse arguments
foreach ($arg in $args) {
    switch ($arg) {
        "-k" { $keep = $true }
        "-f" { $force = $true }
        "-c" { $stdout = $true }
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

# Define compression logic
try {
    $inputStream = [System.IO.File]::OpenRead($file)
    $memoryStream = New-Object System.IO.MemoryStream
    $gzipStream = [System.IO.Compression.GZipStream]::new(
        $memoryStream,
        [System.IO.Compression.CompressionMode]::Compress,
        $true
    )

    # Write input to GZip stream
    $inputStream.CopyTo($gzipStream)
    $gzipStream.Close()
    $compressedData = $memoryStream.ToArray()

    if ($stdout) {
        # Output compressed data to stdout
        [Console]::OpenStandardOutput().Write($compressedData, 0, $compressedData.Length)
    } else {
        # Write compressed data to file
        $outputFile = "$file.gz"

        # Check if output file exists
        if (Test-Path $outputFile) {
            if (-not $force) {
                Write-Host "gzip: $($outputFile) already exists. Use -f to overwrite." -ForegroundColor Red
                exit 1
            }
            Remove-Item -Force $outputFile
        }

        [System.IO.File]::WriteAllBytes($outputFile, $compressedData)
        Write-Host "gzip: $($file) compressed to $($outputFile) successfully."

        # Remove the original file unless -k (keep) is specified
        if (-not $keep) {
            Remove-Item -Force $file
        }
    }

    # Clean up
    $inputStream.Close()
    $memoryStream.Close()
} catch {
    Write-Host "gzip: An error occurred while compressing $($file)." -ForegroundColor Red
    exit 1
}

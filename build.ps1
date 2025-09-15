$ErrorActionPreference = 'Stop'

# Read the .env file and set environment variables
Get-Content .\.env | ForEach-Object {
  if ($_ -match '^\s*([^#].*?)\s*=\s*(.*)') {
    $key = $Matches[1].Trim()
    $value = $Matches[2].Trim()
    Write-Host "Setting env var: $key"
    [System.Environment]::SetEnvironmentVariable($key, $value, 'Process')
  }
}

# Run the zig build command
zig build

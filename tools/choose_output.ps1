param([string]$InputFile)
Add-Type -AssemblyName System.Windows.Forms
$dialog = New-Object System.Windows.Forms.SaveFileDialog
$dialog.Title = 'اختيار مكان الملف المرتب'
$dialog.Filter = 'Adobe Illustrator (*.ai)|*.ai'
$dialog.FileName = ([IO.Path]::GetFileNameWithoutExtension($InputFile) + '.ordered.ai')
$dialog.InitialDirectory = [IO.Path]::GetDirectoryName($InputFile)
if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) { exit 2 }
$script = Join-Path $PSScriptRoot 'rdworks_sort.py'
& python $script $InputFile --output $dialog.FileName
if ($LASTEXITCODE -eq 0) { Start-Process ($dialog.FileName -replace '\.ai$', '.preview.html') }
exit $LASTEXITCODE

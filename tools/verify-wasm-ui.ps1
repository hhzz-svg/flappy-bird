$ErrorActionPreference = 'Stop'

$requiredFiles = @(
    'assets/fonts/NotoSansSC-UI-Subset.otf'
    'assets/fonts/OFL.txt'
    'assets/fonts/SOURCE.md'
    'assets/fonts/subset-glyphs.txt'
    'assets/qtlogo.svg'
    'assets/favicon.svg'
    'assets/favicon.ico'
    'resources.qrc'
)

foreach ($file in $requiredFiles) {
    if (-not (Test-Path -LiteralPath $file)) {
        throw "Missing WASM UI resource: $file"
    }
}

$main = Get-Content -Raw -Encoding UTF8 -LiteralPath 'main.cpp'
$game = Get-Content -Raw -Encoding UTF8 -LiteralPath 'gamewidget.cpp'
$project = Get-Content -Raw -Encoding UTF8 -LiteralPath 'FlappyBird.pro'
$fontSource = Get-Content -Raw -Encoding UTF8 -LiteralPath 'assets/fonts/SOURCE.md'
$subsetHash = (Get-FileHash -Algorithm SHA256 -LiteralPath 'assets/fonts/NotoSansSC-UI-Subset.otf').Hash

if (-not $main.Contains('QFontDatabase::addApplicationFont')) {
    throw 'WASM font is not registered'
}
if (-not $main.Contains('#ifdef Q_OS_WASM')) {
    throw 'WASM font registration is not guarded'
}
if (-not $game.Contains('QApplication::font()')) {
    throw 'WASM label path does not use the application font'
}
if (-not $project.Contains('resources.qrc')) {
    throw 'Qt resource file is not in the qmake project'
}
if (-not $fontSource.Contains($subsetHash)) {
    throw 'Font provenance does not contain the actual subset SHA-256'
}

$unsupportedEmojiCodePoints = @(
    0x1F424, 0x1F343, 0x1F525, 0x1F319, 0x26C8, 0x26A1, 0x1F6D2,
    0x23F8, 0x1F3C5, 0x1F947, 0x1F948, 0x1F949, 0x1F423, 0x1F389
)
foreach ($codePoint in $unsupportedEmojiCodePoints) {
    $emoji = [char]::ConvertFromUtf32($codePoint)
    if ($game.Contains($emoji)) {
        throw ('Unsupported decorative emoji remains: U+{0:X}' -f $codePoint)
    }
}

$workflow = Get-Content -Raw -Encoding UTF8 -LiteralPath '.github/workflows/deploy-pages.yml'
$workflowRequirements = @(
    'site/qtlogo.svg'
    'site/favicon.svg'
    'site/favicon.ico'
    'rel="icon" href="favicon.svg" type="image/svg+xml"'
    'test -s site/qtlogo.svg'
    'test -s site/favicon.svg'
    'test -s site/favicon.ico'
)
foreach ($requirement in $workflowRequirements) {
    if (-not $workflow.Contains($requirement)) {
        throw "Pages workflow is missing: $requirement"
    }
}

Write-Output 'WASM UI font assertions passed.'

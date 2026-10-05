param([string] $GeneratedProjectPath = ".generated/gbarecomp")

. "$PSScriptRoot/common.ps1"
$project = Resolve-RepoPath $GeneratedProjectPath
Assert-GenerationProvenance -Project $project
$original = Get-Content -LiteralPath (Join-Path $project 'sacredstones-generation.json') -Raw
$testRoot = Join-Path $script:RepoRoot ("build/provenance-tests/" + [guid]::NewGuid().ToString('N'))
[IO.Directory]::CreateDirectory($testRoot) | Out-Null
$manifestPath = Join-Path $testRoot 'sacredstones-generation.json'
foreach ($case in @('framework-revision', 'core-revision', 'source-change', 'input-change')) {
    $manifest = $original | ConvertFrom-Json
    switch ($case) {
        'framework-revision' { $manifest.framework.revision = 'different revision' }
        'core-revision' { $manifest.framework.core_revision = 'different revision' }
        'source-change' { $manifest.framework.source_digest = 'different source' }
        'input-change' { $manifest.inputs[0].sha256 = 'different input' }
    }
    $manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $manifestPath -Encoding utf8
    $rejected = $false
    try { Assert-GenerationProvenance -Project $testRoot } catch {
        if ($_.Exception.Message -notmatch 'Generated game and framework sources differ|Generation input changed') { throw }
        $rejected = $true
    }
    if (-not $rejected) { throw "Stale generation accepted: $case" }
    Write-Host "PASS: rejected $case"
}

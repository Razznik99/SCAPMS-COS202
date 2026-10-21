# Rebuild the submission report
#
# Requirements: pandoc 3.x, a LaTeX distribution (pdflatex) and the
# screenshots/diagrams already present in the repository.
#
# Usage:  powershell -ExecutionPolicy Bypass -File docs/report/build-report.ps1

$ErrorActionPreference = 'Stop'
Set-Location (Resolve-Path (Join-Path $PSScriptRoot '..\..'))

pandoc 'docs/report/COS202_SCAPMS_Report.md' `
    -o 'submission/COS202_SCAPMS_Submission_Report.pdf' `
    --pdf-engine=pdflatex `
    --include-in-header=docs/report/header.tex `
    '--resource-path=.;screenshots;docs/uml' `
    -V geometry:margin=2.1cm `
    -V fontsize=10pt `
    --number-sections `
    --syntax-highlighting=tango

Write-Host 'Report written to submission/COS202_SCAPMS_Submission_Report.pdf'

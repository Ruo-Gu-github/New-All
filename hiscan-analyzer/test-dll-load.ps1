param(
    [string]$DllDir = (Resolve-Path (Join-Path $PSScriptRoot '..\ConsoleDllTest\Dlls\bin')).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if (-not (Test-Path -LiteralPath $DllDir)) {
    throw "DLL directory not found: $DllDir"
}

Write-Host "DLL directory: $DllDir"

Push-Location -LiteralPath $DllDir
try {
    Add-Type -Language CSharp -TypeDefinition @"
using System;
using System.Runtime.InteropServices;

public static class Win32
{
    [DllImport("kernel32.dll", SetLastError=true, CharSet=CharSet.Unicode)]
    public static extern bool SetDllDirectory(string lpPathName);
}

public static class NativeDicom
{
    [DllImport("DllDicom.dll", CallingConvention=CallingConvention.Cdecl)]
    public static extern IntPtr Dicom_Volume_Create();

    [DllImport("DllDicom.dll", CallingConvention=CallingConvention.Cdecl)]
    public static extern void Dicom_Volume_Destroy(IntPtr h);

    [DllImport("DllDicom.dll", CallingConvention=CallingConvention.Cdecl)]
    public static extern IntPtr Dicom_GetLastError();
}

public static class NativeBone
{
    [DllImport("DllBoneAnalysis.dll", CallingConvention=CallingConvention.Cdecl)]
    public static extern IntPtr BoneAnalysis_GetLastError();
}
"@

    [void][Win32]::SetDllDirectory($DllDir)

    Write-Host "\n[1/2] Loading + calling DllDicom..."
    try {
        $h = [NativeDicom]::Dicom_Volume_Create()
        if ($h -eq [IntPtr]::Zero) {
            $errPtr = [NativeDicom]::Dicom_GetLastError()
            $err = if ($errPtr -ne [IntPtr]::Zero) { [Runtime.InteropServices.Marshal]::PtrToStringAnsi($errPtr) } else { $null }
            throw "Dicom_Volume_Create returned NULL. LastError: $err"
        }

        Write-Host "✅ DllDicom OK (Dicom_Volume_Create returned non-null)"
    }
    finally {
        if ($h -and $h -ne [IntPtr]::Zero) {
            [NativeDicom]::Dicom_Volume_Destroy($h)
        }
    }

    Write-Host "\n[2/2] Loading + calling DllBoneAnalysis..."
    try {
        $errPtr2 = [NativeBone]::BoneAnalysis_GetLastError()
        $err2 = if ($errPtr2 -ne [IntPtr]::Zero) { [Runtime.InteropServices.Marshal]::PtrToStringAnsi($errPtr2) } else { "" }
        Write-Host "✅ DllBoneAnalysis OK (BoneAnalysis_GetLastError returned: '$err2')"
    }
    catch {
        throw "DllBoneAnalysis call failed: $($_.Exception.GetType().FullName): $($_.Exception.Message)"
    }

    Write-Host "\nAll checks passed."
}
finally {
    Pop-Location
}

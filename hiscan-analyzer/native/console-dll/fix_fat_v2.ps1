$path = Join-Path $PSScriptRoot "src\FatAnalysis.cpp"
$txt = Get-Content -LiteralPath $path -Raw -Encoding UTF8

$helpers = @("FatMainRegionByRadius", "Dilate3D", "Corrosion3D", "Fast3DFloodFill", "MaskCombineWithDilatedPrev", "BoneMaskConvexHullInternalMask", "cross", "convexHull", "pointInPolygon", "calc_area", "ErodeMask", "DilateMask")

foreach ($h in $helpers) {
     $txt = $txt -replace "FatAnalysis::$h", "$h"
}

$txt = $txt.Replace("void FatMainRegionByRadius", "static void FatMainRegionByRadius")
$txt = $txt.Replace("void Dilate3D", "static void Dilate3D")
$txt = $txt.Replace("void Corrosion3D", "static void Corrosion3D")
$txt = $txt.Replace("void Fast3DFloodFill", "static void Fast3DFloodFill")
$txt = $txt.Replace("void MaskCombineWithDilatedPrev", "static void MaskCombineWithDilatedPrev")
$txt = $txt.Replace("void BoneMaskConvexHullInternalMask", "static void BoneMaskConvexHullInternalMask")
$txt = $txt.Replace("int cross", "static int cross")
$txt = $txt.Replace("std::vector<Point2D> convexHull", "static std::vector<Point2D> convexHull")
$txt = $txt.Replace("bool pointInPolygon", "static bool pointInPolygon")
$txt = $txt.Replace("int calc_area", "static int calc_area")
$txt = $txt.Replace("void ErodeMask", "static void ErodeMask")
$txt = $txt.Replace("void DilateMask", "static void DilateMask")

$forwardDecl = "static void BoneMaskConvexHullInternalMask(const BYTE* boneMask, int width, int height, int depth, int start, int end, BYTE* internalMask);"
$txt = $txt -replace "(?s)struct Point2D\s*\{\s*int x,\s*y;\s*\};", "$&`r`n$forwardDecl"

Set-Content -LiteralPath $path -Value $txt -Encoding UTF8

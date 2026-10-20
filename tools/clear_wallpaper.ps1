# tools/clear_wallpaper.ps1 - clear the Windows desktop wallpaper (test).
# SPI_SETDESKWALLPAPER with an empty path removes the wallpaper image, so the
# shell's DefView stops painting an opaque layer over the desktop.
# Returns the previous wallpaper path so it can be restored.
$ErrorActionPreference = 'Stop'
Add-Type @"
using System;using System.Runtime.InteropServices;using System.Text;
public class WP {
  [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  public static extern int SystemParametersInfo(uint a, uint b, string c, uint d);
  [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)]
  public static extern int SystemParametersInfo(uint a, uint b, IntPtr c, uint d);
  [DllImport("shell32.dll", CharSet=CharSet.Unicode)]
  public static extern void SHGetFileInfo(string pszPath, uint dwFileAttributes, ref SHFILEINFO psfi, uint cbFileInfo, uint uFlags);
  [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)]
  public struct SHFILEINFO { public IntPtr hIcon; public int iIcon; public IntPtr dwAttributes;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=260)] public string szDisplayName;
    [MarshalAs(UnmanagedType.ByValTStr, SizeConst=80)] public string szTypeName; }
}
"@
$SPI_GETDESKWALLPAPER = 0x0074
$SPI_SETDESKWALLPAPER = 0x0014
$SPIF_UPDATEINIFILE = 0x01
$SPIF_SENDCHANGE = 0x02

# Read the current wallpaper path.
$buf = New-Object System.Text.StringBuilder 260
$null = [WP]::SystemParametersInfo($SPI_GETDESKWALLPAPER, 260, $buf, 0)
$prev = $buf.ToString()
Write-Host "current wallpaper: $prev"

$mode = $args[0]
if ($mode -eq 'clear') {
    [void][WP]::SystemParametersInfo($SPI_SETDESKWALLPAPER, 0, '', $SPIF_UPDATEINIFILE -bor $SPIF_SENDCHANGE)
    Write-Host "cleared (empty path)"
}
elseif ($mode -eq 'restore' -and $prev) {
    [void][WP]::SystemParametersInfo($SPI_SETDESKWALLPAPER, 0, $prev, $SPIF_UPDATEINIFILE -bor $SPIF_SENDCHANGE)
    Write-Host "restored: $prev"
}
else {
    Write-Host "usage: clear_wallpaper.ps1 clear|restore"
}

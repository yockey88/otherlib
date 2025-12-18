param(
  [Parameter(Mandatory=$true)]
  [string]$ExeName
)

$AllProcesses = Get-Process
$ProcessName = [System.IO.Path]::GetFileNameWithoutExtension($ExeName)
# $AllProcesses |
#   Sort-Object -Property ProcessName |
#   Format-Table -AutoSize -Property Id, ProcessName

$MatchingProcesses = @(
  $AllProcesses |
    Where-Object { $_.ProcessName -ieq $ProcessName }
)
$MemberDefinition = '
    [DllImport("kernel32.dll")]public static extern bool FreeConsole();
    [DllImport("kernel32.dll")]public static extern bool AttachConsole(uint p);
    [DllImport("kernel32.dll")]public static extern bool GenerateConsoleCtrlEvent(uint e, uint p);
    public static void SendCtrlC(uint p) {
      FreeConsole();
      AttachConsole(p);
      GenerateConsoleCtrlEvent(0, p);
      FreeConsole();
      AttachConsole(uint.MaxValue);
    }'

if ($MatchingProcesses.Count -gt 0) {
  Add-Type -Name 'Console' -Namespace 'Other' -MemberDefinition $MemberDefinition
  [Other.Console]::SendCtrlC($ProcessID)
  echo "Killing processes with name '$ProcessName'..."
} else {
  echo "No running process found with name '$ProcessName'."
}
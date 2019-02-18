
#include "hid.au3"
#include <MsgBoxConstants.au3>
#include <AutoItConstants.au3>

Func OpenButton()
   $aDevices = _HIDListDevices()

   For $i = 0 To UBound($aDevices) - 1
	  If Not StringInStr($aDevices[$i], "vid_16c0&pid_27dc") Then
		 ContinueLoop
	  EndIf

	  ; Open existing file (2)
	  ; Open for read (2) and write (4)
	  ; Share read (2) and write (4)
	  $hDevHandle = _WinAPI_CreateFile($aDevices[$i], 2, 6, 6, Null)
	  If $hDevHandle == 0 Then
		 ContinueLoop
	  EndIf

	  $sSerial = _HID_GetSerialNumberString($hDevHandle)
	  If StringLeft($sSerial, 24) <> "orca.pet:Emergency stop:" Then
		 _WinAPI_CloseHandle($hDevHandle)
		 ContinueLoop
	  EndIf

	  Return $hDevHandle
   Next

   Return 0
EndFunc

Func IsButtonPressed($hButton)
   $tReport = DllStructCreate("byte reportId; byte status;")
   DllStructSetData($tReport, "reportId", 0)

   If Not _HID_GetInputReport($hButton, $tReport) Then
	  MsgBox($MB_ICONERROR, "Emergency button", "I/O error (is device still connected?)")
	  Exit(1)
   EndIf

   Return DllStructGetData($tReport, "status")
EndFunc

$hButton = OpenButton()
If $hButton == 0 Then
   MsgBox($MB_ICONERROR, "Emergency button", "No device found")
   Exit(1)
EndIf

Func MinimizeWindows($sTitle, ByRef $aMinimized)
   $aWindows = WinList($sTitle)
   For $i = 1 To $aWindows[0][0]
	  $hWinHandle = $aWindows[$i][1]
	  ConsoleWrite($aWindows[$i][0] & @CRLF)
	  $iState = WinGetState($hWinHandle)
	  If BitAND($iState, $WIN_STATE_MINIMIZED) = 0 Then
		 If WinSetState($hWinHandle, "", @SW_MINIMIZE) Then
			ReDim $aMinimized[UBound($aMinimized) + 1]
			$aMinimized[UBound($aMinimized) - 1] = $hWinHandle
		 EndIf
	  EndIf
   Next
EndFunc

While True
   While Not IsButtonPressed($hButton)
	  Sleep(100)
   WEnd

   $bTelegramClosed = WinClose("Telegram")
   Local $aMinimized[0]
   MinimizeWindows("[REGEXPTITLE:.*Firefox.*\(Navegación privada\).*]", $aMinimized)

   While IsButtonPressed($hButton)
	  Sleep(100)
   WEnd

   If $bTelegramClosed Then
	  Run("D:\Telegram\Telegram.exe")
   EndIf

   For $i = 0 To UBound($aMinimized) - 1
	  WinSetState($aMinimized[$i], "", @SW_RESTORE)
   Next
WEnd

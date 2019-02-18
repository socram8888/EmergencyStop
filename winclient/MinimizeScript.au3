
#include "EmergencyButton.au3"
#include <MsgBoxConstants.au3>
#include <AutoItConstants.au3>

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

_EmergencyButton_WaitStatus(0)

While True
   _EmergencyButton_WaitStatus(1)

   $bTelegramClosed = WinClose("Telegram")
   Local $aMinimized[0]
   MinimizeWindows("[REGEXPTITLE:.*Firefox.*\(Navegación privada\).*]", $aMinimized)

   _EmergencyButton_WaitStatus(0)

   If $bTelegramClosed Then
	  Run("D:\Telegram\Telegram.exe")
   EndIf

   For $i = 0 To UBound($aMinimized) - 1
	  WinSetState($aMinimized[$i], "", @SW_RESTORE)
   Next
WEnd


#include "WinAPIHID.au3"

Func _EmergencyButton_Open()
   $aDevices = _HID_ListDevices()

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

Func _EmergencyButton_Pressed($hButton)
   $tReport = DllStructCreate("byte reportId; byte status;")
   DllStructSetData($tReport, "reportId", 0)

   If Not _HID_GetInputReport($hButton, $tReport) Then
	  Return SetError(1, 0, -1)
   EndIf

   Return DllStructGetData($tReport, "status")
EndFunc

Func _EmergencyButton_Close($hButton)
   _WinAPI_CloseHandle($hButton)
EndFunc

Func _EmergencyButton_WaitStatus($iWanted)
   While True
	  $hButton = _EmergencyButton_Open()
	  If $hButton <> 0 Then
		 $iPressed = _EmergencyButton_Pressed($hButton)
		 _EmergencyButton_Close($hButton)
		 If $iPressed = $iWanted Then
			Return
		 EndIf
	  EndIf
	  Sleep(100)
   WEnd
EndFunc

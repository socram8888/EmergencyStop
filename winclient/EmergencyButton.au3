
#include "HID.au3"

Func _EmergencyButton_Open()
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

Func _EmergencyButton_Pressed($hButton)
   $tReport = DllStructCreate("byte reportId; byte status;")
   DllStructSetData($tReport, "reportId", 0)

   If Not _HID_GetInputReport($hButton, $tReport) Then
	  MsgBox($MB_ICONERROR, "Emergency button", "I/O error (is device still connected?)")
	  Exit(1)
   EndIf

   Return DllStructGetData($tReport, "status")
EndFunc

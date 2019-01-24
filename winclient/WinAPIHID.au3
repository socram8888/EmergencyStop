
#include-once

#include <StructureConstants.au3>

Global $__g_hHidDllHandle = DllOpen("hid.dll")

Const $HID_INTERFACE_GUID = DllStructCreate($tagGUID)
DllCall($__g_hHidDllHandle, "BOOLEAN", "HidD_GetHidGuid", "ptr", DllStructGetPtr($HID_INTERFACE_GUID))

Func _HID_GetSerialNumberString($hHidDeviceObject)
   $tSerialString = DllStructCreate("wchar serial[128]")
   $vRet = DllCall($__g_hHidDllHandle, "boolean", "HidD_GetSerialNumberString", "handle", $hHidDeviceObject, "ptr", DllStructGetPtr($tSerialString), "dword", DllStructGetSize($tSerialString))

   If @error or Not $vRet[0] Then
	  Return Null
   EndIf

   Return DllStructGetData($tSerialString, "serial")
EndFunc

Func _HID_GetInputReport($hHidDeviceObject, $tReport)
   $vRet = DllCall($__g_hHidDllHandle, "boolean", "HidD_GetInputReport", "handle", $hHidDeviceObject, "ptr", DllStructGetPtr($tReport), "ulong", DllStructGetSize($tReport))
   Return Not @error And $vRet[0]
EndFunc


#include-once

#include <StructureConstants.au3>
#include <WinAPIFiles.au3>
#include <WinAPIHObj.au3>
#include "WinAPISetup.au3"

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

Func _HID_ListDevices()
   Local $aDevices[0]

   ; Get HID device info
   $hHidDevInfo = _SetupAPI_SetupDiGetClassDevs($HID_INTERFACE_GUID, Null, Null, BitOR($DIGCF_PRESENT, $DIGCF_DEVICEINTERFACE))

   ; Allocate device interface data
   $tDevData = DllStructCreate($tagSP_DEVICE_INTERFACE_DATA)
   DllStructSetData($tDevData, "cbSize", DllStructGetSize($tDevData))

   ; Iterate through all HID devices
   $iDevIdx = 0
   While _SetupAPI_SetupDiEnumDeviceInterfaces($hHidDevInfo, Null, $HID_INTERFACE_GUID, $iDevIdx, $tDevData)
	  ; Get required struct size. This call will fail but will update $iRequiredSize with required size
	  Local $iRequiredSize = 0
	  $tInterfaceDetail = _SetupAPI_SetupDiGetDeviceInterfaceDetail($hHidDevInfo, $tDevData, Null)

	  If $tInterfaceDetail <> Null Then
		 ReDim $aDevices[UBound($aDevices) + 1]
		 $aDevices[UBound($aDevices) - 1] = DllStructGetData($tInterfaceDetail, "DevicePath")
	  EndIf

	  $iDevIdx += 1
   WEnd

   Return $aDevices
EndFunc

Func _HID_Close($hHandle)
   Return _WinAPI_CloseHandle($hHandle)
EndFunc

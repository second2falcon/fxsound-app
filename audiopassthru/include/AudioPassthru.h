/*
FxSound
Copyright (C) 2025  FxSound LLC

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
*  Public defines for the AudioPassthru class
*/

#ifndef _AUDIOPASSTHRU_H_
#define _AUDIOPASSTHRU_H_ 

#ifndef __ANDROID__
#include <Windows.h>
#endif //WIN32
#include <Mmdeviceapi.h>
#include <vector> 
#include "DfxDsp.h"

struct SoundDevice {
	IMMDevice *pAllDevices = NULL; // Object pointers for each device.
	bool isCaptureDevice = false;
	bool isPlaybackDevice = false;
	bool isTargetedRealPlaybackDevice = false;
	bool isRealDevice = false;
	bool isDFXDevice = false;
	bool isUserSelectedPlaybackDevice = false;
	bool isDefaultDevice = false;
    bool isActive = false;

	std::wstring pwszID; // For the GUID ID strings for each device, all devices combined.
	std::wstring pwszIDRealDevices; // For the GUID ID strings for each real playback device.
	WCHAR pwszIDPreviousRealDevices[512]; // To detect when a new devices is added.

	std::wstring deviceFriendlyName;  // Friendly name, ie "DFX Audio Enhancer 10.5", all devices.
	std::wstring deviceDescription;  // Descriptive name, ie "Speakers", all devices.
	std::wstring deviceFriendlyNameRealDevices;  // Friendly names, just the real devices.
	std::wstring deviceDescriptionRealDevices;  // Descriptive names, just the real devices.
	std::wstring deviceFormFactor;  // Form factor type string, eg "HDMI", "Speakers", etc.
	int deviceNumChannel; // Number of channels for all devices.
};

// An extra FxSound device that is played through its own effects to a real device.
// Apps are assigned to an extra FxSound device in Windows' per-app output settings.
struct AudioRouteSettings {
	std::wstring fx_device_id;
	std::wstring real_device_id;
	// Effects and EQ come from this DfxDsp, the main one when null. Power and the app wide settings
	// (master gain, balance, ...) always come from the main one. Must outlive the route.
	DfxDsp* preset_dsp = nullptr;
};

class AudioPassthruCallback
{
public:
	virtual void onSoundDeviceChange(bool processing) = 0;
};

class AudioPassthruPrivate;
class AudioPassthru
{
public:
	AudioPassthru();
	~AudioPassthru();
	int init();
	void mute(bool mute);
	std::vector<SoundDevice> getSoundDevices(bool active_devices = true);
	int setBufferLength(int i_buffer_length_msecs);
	int processTimer();
	void setDspProcessingModule(DfxDsp* pDspProcessingModule);
	void setAsPlaybackDevice(const SoundDevice sound_device);
	void registerCallback(AudioPassthruCallback *callback);
    bool isPlaybackDeviceAvailable();
	bool checkDeviceChanges();
	void restoreDefaultPlaybackDevice();
	void setRoutes(const std::vector<AudioRouteSettings>& routes);
	// Changes where a route's effects and EQ come from without interrupting its audio.
	void setRoutePresetDsp(const std::wstring& fx_device_id, DfxDsp* preset_dsp);

private:
	AudioPassthruPrivate *data_;
};

#endif

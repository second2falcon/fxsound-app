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
#include "u_AudioPassthru.h"
#include <Audioclient.h>
#include <ks.h>
#include <ksmedia.h>
#include <algorithm>
#include <cmath>
#include <cstring>

_COM_SMARTPTR_TYPEDEF(IAudioClient, __uuidof(IAudioClient));
_COM_SMARTPTR_TYPEDEF(IAudioCaptureClient, __uuidof(IAudioCaptureClient));
_COM_SMARTPTR_TYPEDEF(IAudioRenderClient, __uuidof(IAudioRenderClient));

namespace
{
	constexpr REFERENCE_TIME BUFFER_DURATION = 100 * 10000;	// 100ms WASAPI buffers, in 100ns units
	constexpr UINT32 PREROLL_MSECS = 30;			// Silence queued ahead of the data after a start or underrun, absorbs polling jitter
	constexpr UINT32 MAX_QUEUED_MSECS = 90;			// Drop captured data rather than queue more than this (clock drift between devices)
	constexpr DWORD POLL_MSECS = 2;
	constexpr ULONGLONG IDLE_MSECS = 1000;			// Stop the real device stream after this long without data, so the PC can sleep
	constexpr ULONGLONG RETRY_MSECS = 3000;
	constexpr ULONGLONG SYNC_INTERVAL_MSECS = 100;

	bool differ(float a, float b)
	{
		return std::fabs(a - b) > 0.0001f;
	}
}

AudioRoute::AudioRoute(const std::wstring& fx_device_id, const std::wstring& real_device_id, DfxDsp* settings_source) :
	fx_device_id_(fx_device_id), real_device_id_(real_device_id), settings_source_(settings_source),
	thread_(&AudioRoute::run, this)
{
}

AudioRoute::~AudioRoute()
{
	stop_ = true;
	thread_.join();
}

void AudioRoute::run()
{
	// Same priority as the main processing thread.
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);

	HRESULT com_init = CoInitializeEx(NULL, COINIT_MULTITHREADED);

	while (!stop_)
	{
		{
			IMMDeviceEnumeratorPtr enumerator;
			if (SUCCEEDED(enumerator.CreateInstance(__uuidof(MMDeviceEnumerator))))
				stream(enumerator);
		}

		// stream() returns when a device is missing, goes away or changes format, try again in a bit.
		for (ULONGLONG start = GetTickCount64(); !stop_ && GetTickCount64() - start < RETRY_MSECS; )
			Sleep(50);
	}

	if (SUCCEEDED(com_init))
		CoUninitialize();
}

void AudioRoute::stream(IMMDeviceEnumerator* enumerator)
{
	IMMDevicePtr fx_device;
	IMMDevicePtr real_device;
	IAudioClientPtr capture_client;
	IAudioClientPtr render_client;
	IAudioCaptureClientPtr capture;
	IAudioRenderClientPtr render;
	WAVEFORMATEX* mix_format = NULL;

	if (FAILED(enumerator->GetDevice(fx_device_id_.c_str(), &fx_device)) ||
		FAILED(enumerator->GetDevice(real_device_id_.c_str(), &real_device)) ||
		FAILED(fx_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&capture_client)) ||
		FAILED(real_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, (void**)&render_client)) ||
		FAILED(capture_client->GetMixFormat(&mix_format)))
		return;

	std::unique_ptr<WAVEFORMATEX, decltype(&CoTaskMemFree)> format(mix_format, &CoTaskMemFree);

	// The shared mode mix format is 32 bit float, which DfxDsp processes in place, check anyway.
	auto is_float = format->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
		(format->wFormatTag == WAVE_FORMAT_EXTENSIBLE && reinterpret_cast<WAVEFORMATEXTENSIBLE*>(format.get())->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT);
	if (!is_float || format->wBitsPerSample != 32)
		return;

	// The real device is opened with the FxSound device's format, AUTOCONVERTPCM has Windows convert
	// to the real device's own sample rate and channel count.
	if (FAILED(capture_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, BUFFER_DURATION, 0, format.get(), NULL)) ||
		FAILED(render_client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,
			BUFFER_DURATION, 0, format.get(), NULL)) ||
		FAILED(capture_client->GetService(__uuidof(IAudioCaptureClient), (void**)&capture)) ||
		FAILED(render_client->GetService(__uuidof(IAudioRenderClient), (void**)&render)))
		return;

	UINT32 render_buffer_frames = 0;
	if (FAILED(render_client->GetBufferSize(&render_buffer_frames)))
		return;

	const UINT32 frame_bytes = format->nBlockAlign;
	const UINT32 channels = format->nChannels;
	const UINT32 preroll_frames = format->nSamplesPerSec * PREROLL_MSECS / 1000;
	const UINT32 max_queued_frames = (std::min<UINT32>)(format->nSamplesPerSec * MAX_QUEUED_MSECS / 1000, render_buffer_frames);
	// Like the main output, DfxDsp is not used on mono or more than 8 channels.
	const bool process = channels >= 2 && channels <= 8;
	if (process)
		dsp_.setSignalFormat(32, channels, format->nSamplesPerSec, 32);

	if (FAILED(capture_client->Start()))
		return;

	HRESULT hr = S_OK;
	bool playing = false;
	ULONGLONG last_data_time = 0;

	while (!stop_ && SUCCEEDED(hr))
	{
		Sleep(POLL_MSECS);

		if (GetTickCount64() - last_sync_ >= SYNC_INTERVAL_MSECS)
		{
			syncSettings();
			last_sync_ = GetTickCount64();
		}

		UINT32 packet_frames = 0;
		while (SUCCEEDED(hr) && SUCCEEDED(hr = capture->GetNextPacketSize(&packet_frames)) && packet_frames > 0)
		{
			BYTE* data;
			UINT32 frames;
			DWORD flags;
			if (FAILED(hr = capture->GetBuffer(&data, &frames, &flags, NULL, NULL)))
				break;

			UINT32 queued = 0;
			hr = render_client->GetCurrentPadding(&queued);

			// Nothing queued means a fresh start or an underrun, queue some silence first so polling jitter doesn't cause gaps.
			BYTE* out;
			if (SUCCEEDED(hr) && queued == 0 && SUCCEEDED(hr = render->GetBuffer(preroll_frames, &out)))
			{
				hr = render->ReleaseBuffer(preroll_frames, AUDCLNT_BUFFERFLAGS_SILENT);
				queued = preroll_frames;
				if (SUCCEEDED(hr) && !playing)
				{
					hr = render_client->Start();
					playing = true;
				}
			}

			if (SUCCEEDED(hr) && queued + frames <= max_queued_frames && SUCCEEDED(hr = render->GetBuffer(frames, &out)))
			{
				if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
					memset(out, 0, frames * frame_bytes);
				else
					memcpy(out, data, frames * frame_bytes);

				if (process)
					dsp_.processAudio((short int*)out, (short int*)out, frames, 0);

				hr = render->ReleaseBuffer(frames, 0);
			}

			HRESULT release_hr = capture->ReleaseBuffer(frames);
			if (SUCCEEDED(hr))
				hr = release_hr;
			last_data_time = GetTickCount64();
		}

		if (SUCCEEDED(hr) && playing && GetTickCount64() - last_data_time > IDLE_MSECS)
		{
			render_client->Stop();
			hr = render_client->Reset();
			playing = false;
		}
	}

	capture_client->Stop();
	render_client->Stop();
}

// Copies the settings of the main DfxDsp, which the GUI controls, into this route's own DfxDsp.
// Runs on the route thread so dsp_ is only ever touched by one thread.
void AudioRoute::syncSettings()
{
	DfxDsp& source = *settings_source_;

	// isPowerOn() returns the bypass state, compare as is and invert when setting.
	if (source.isPowerOn() != dsp_.isPowerOn())
		dsp_.powerOn(!source.isPowerOn());

	for (int i = 0; i < DfxDsp::NumEffects; i++)
	{
		auto effect = static_cast<DfxDsp::Effect>(i);
		// getEffectValue() returns a tenth of the value setEffectValue() takes.
		if (differ(source.getEffectValue(effect), dsp_.getEffectValue(effect)))
			dsp_.setEffectValue(effect, source.getEffectValue(effect) * 10);
	}

	int num_bands = source.getNumEqBands();
	if (num_bands != dsp_.getNumEqBands())
		dsp_.setNumBands(num_bands);
	for (int band = 0; band < num_bands; band++)
	{
		// A band read while the GUI changes the number of bands comes back as 0Hz, skip it until the next sync.
		auto freq = source.getEqBandFrequency(band);
		if (freq > 0 && differ(freq, dsp_.getEqBandFrequency(band)))
			dsp_.setEqBandFrequency(band, freq);
		if (differ(source.getEqBandBoostCut(band), dsp_.getEqBandBoostCut(band)))
			dsp_.setEqBandBoostCut(band, source.getEqBandBoostCut(band));
	}

	if (differ(source.getBalance(), dsp_.getBalance()))
		dsp_.setBalance(source.getBalance());
	if (differ(source.getNormalization(), dsp_.getNormalization()))
		dsp_.setNormalization(source.getNormalization());
	if (differ(source.getVolumeLeveling(), dsp_.getVolumeLeveling()))
		dsp_.setVolumeLeveling(source.getVolumeLeveling());
	if (differ(source.getMasterGain(), dsp_.getMasterGain()))
		dsp_.setMasterGain(source.getMasterGain());
	if (differ(source.getFilterQ(), dsp_.getFilterQ()))
		dsp_.setFilterQ(source.getFilterQ());
}

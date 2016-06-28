#include "devices.h"

std::map<PaDeviceIndex, const PaDeviceInfo*> devices::getInputDevices(){
	std::map<PaDeviceIndex, const PaDeviceInfo*> ret;

	for (int i = 0; i < Pa_GetHostApiCount(); i++){
		const PaHostApiInfo * info = Pa_GetHostApiInfo(i);
		ret.insert(std::make_pair(info->defaultInputDevice, Pa_GetDeviceInfo(info->defaultInputDevice)));
	}
	
	return ret;
}

std::map<PaDeviceIndex, const PaDeviceInfo*> devices::getOutputDevices(){
	std::map<PaDeviceIndex, const PaDeviceInfo*> ret;

	for (int i = 0; i < Pa_GetHostApiCount(); i++){
		const PaHostApiInfo * info = Pa_GetHostApiInfo(i);
		ret.insert(std::make_pair(info->defaultOutputDevice, Pa_GetDeviceInfo(info->defaultOutputDevice)));
	}

	return ret;
}
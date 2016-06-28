#include <portaudio.h>
#include <map>

class devices{
public:
	static std::map<PaDeviceIndex, const PaDeviceInfo*> getInputDevices();
	static std::map<PaDeviceIndex, const PaDeviceInfo*> getOutputDevices();

};
#include <iostream>
#include "vulkan/vulkan.h"
class RenderPipelineBase
{
private:
	std::string GetWindowTile();
	/** brief Indicates that the view (position, rotation) has changed and buffers containing camera matrices need to be updated */
	bool view_updated_ = false;
	// Destination dimensions for resizing the window
	uint32_t dest_width_;
	uint32_t dest_height;
	bool resizing_ = false;
	// Called if the window is resized and some resources have to be recreated
	void ResizeWindow();
	void HandleMouseMove(int32_t x, int32_t y);
};

std::string RenderPipelineBase::GetWindowTile()
{
	//std::string device(deviceProperties.deviceName);
	//std::string windowTitle;
	//windowTitle = title + " - " + device;
	//if (!settings.overlay) {
	//	windowTitle += " - " + std::to_string(frameCounter) + " fps";
	//}
	//return windowTitle;
	return "lalala";
}
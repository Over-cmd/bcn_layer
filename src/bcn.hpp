#ifndef __BCN_HPP
#define __BCN_HPP

#include "bcn_layer.hpp"

struct push_constants {
	int format;
	int width;
	int height;
	int offset;	
	int bufferRowLength;
	int offsetX;
	int offsetY;
	int use_image_view;
};

bool is_s3tc(VkFormat format);
bool is_rgtc(VkFormat format);
bool is_bc6(VkFormat format);
bool is_bc7(VkFormat format);
bool is_supported_bcn_format(struct device *device, VkFormat format);
VkFormat get_format_for_bcn(VkFormat format);
VkResult create_bcn_compute_pipelines(struct device *dev);
VkResult decompress_bcn_compute(struct device *dev,
                       			VkCommandBuffer commandbuffer,
                       			VkFormat format,
                       			VkBufferImageCopy *copy_region,
                       			struct buffer *srcBuffer,
                       			struct buffer *stagingBuffer,
                       			struct image *dstImage,
                       			VkImageLayout dstImageLayout);

#endif



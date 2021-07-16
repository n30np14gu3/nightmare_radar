#pragma once

typedef struct {
	unsigned short magic;
	unsigned int length;
	char buffer[1];
} image_data;

int map_image(image_data* buffer, unsigned int size);
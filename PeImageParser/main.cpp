#include "ImageParser.h"

int main() {

	// Get own image base
	HMODULE imageBase = GetModuleHandleA(NULL);
	if (!imageBase) {
		printf("GetModuleHandle failed\n");
		return 1;
	}

	printf("Image base : %p\n", imageBase);

	// Parse
	ImageInfo imageInfo((const void*)imageBase);

	if (imageInfo.Parse()) {
		imageInfo.Log();
	}
	else {
		printf("Parse failed\n");
		return 1;
	}

	return 0;
}

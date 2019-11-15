CXXFLAGS = -std=c++17 -I$(VULKAN_SDK_PATH)/include
LDFLAGS =  `pkg-config --libs glfw3 vulkan`

VulkanTest: main.cpp vulkanprog.cpp
	$(CXX) $(CXXFLAGS) -o vulkan-test main.cpp vulkanprog.cpp $(LDFLAGS)

.PHONY: test clean

test: vulkan-test
	./vulkan-test

clean:
	rm -f vulkan-test

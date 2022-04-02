/*
 * Author: Emoy Kim
 * E-mail: emoy.kim_AT_gmail.com
 * 
 * This code is a free software; it can be freely used, changed and redistributed.
 * If you use any version of the code, please reference the code.
 * 
 */

#pragma once

#include <vulkan/vulkan.h>
#include <glm.hpp>
#include <common.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/constants.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/quaternion.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <gtx/quaternion.hpp>

#include <FreeImage.h>
#include <iostream>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <limits>
#include <array>
#include <vector>
#include <string>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <optional>
#include <sstream>
#include <fstream>
#include <filesystem>
#include <memory>
#include <chrono>

#include "project_constants.h"

using uint = unsigned int;
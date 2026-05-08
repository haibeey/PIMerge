# PIMerge

PIMerge is a high-performance C++ library designed for seamless panoramic image stitching. It provides advanced algorithms for seam detection, multi-band blending, and global color correction, enabling the creation of high-quality panoramas from multiple overlapping images.

Originally designed for mobile and embedded platforms, PIMerge leverages SIMD optimizations and fast JPEG processing to deliver professional-grade results with efficiency.

## Key Features

- **Intelligent Seam Finding**: Employs dynamic programming to find minimum-cost paths through overlapping regions, utilizing both color difference and Sobel gradient magnitude to avoid cutting through visible edges.
- **Multi-band Blending**: Supports sophisticated blending techniques, including multi-level Laplacian pyramids (Multi-band) and feathering, to eliminate visible boundaries and ensure smooth transitions.
- **Global Color Correction**: Implements sequence-wide color adjustment using linearization and gamma encoding to harmonize exposure and white balance across all source images.
- **Geometry-Aware Stitching**: Uses Horizontal Field of View (HFOV) and camera rotation data to accurately estimate overlap and project images.
- **Advanced Image Operations**: 
    - Gaussian blurring for edge softening.
    - Intelligent cropping to remove non-image artifacts.
    - Dynamic padding (height adjustment) with feathering for aspect ratio correction.
    - Efficient downsampling for preview generation.
- **Cross-Platform Support**: Robust build system for Android (NDK/JNI) and iOS (XCFramework), as well as standard desktop environments.

## Architecture

The project is structured into three main layers:

1.  **PIMerge API (C++)**: The primary interface (`LendoMerge` class) providing high-level functionality for horizontal and vertical stitching.
2.  **NativeStitcher (C)**: A low-level core library located in the `stitcher/` directory, handling the heavy lifting of image manipulation, pyramid blending, and buffer management.
3.  **Dependencies**:
    - **libjpeg-turbo**: High-speed JPEG compression/decompression.
    - **SIMDe**: SIMD Everywhere, providing architecture-independent SIMD acceleration (SSE, NEON, etc.).

## Getting Started

### Prerequisites

- CMake 3.16+
- C++14 compliant compiler
- libjpeg-turbo (managed as an ExternalProject)
- SIMDe (submodule)

### Building

PIMerge uses a standard CMake build system but requires explicit `OS` and `ARCH` definitions for cross-compilation.

```bash
mkdir build && cd build
cmake .. -DOS=Darwin -DARCH=arm64
make
```

For mobile platforms, use the provided helper scripts:
- `./build.andriod.sh`: For Android NDK builds.
- `./build.xcframework.sh`: For iOS XCFramework generation.

## Usage Example

```cpp
#include "PIMerge.hpp"
#include <vector>
#include <string>

int main() {
    // Initialize with HFOV (104°) and Camera Rotation (60°)
    LendoMerge stitcher(104.0f, 60.0f);

    std::vector<std::string> images = {
        "img1.jpg", "img2.jpg", "img3.jpg", 
        "img4.jpg", "img5.jpg", "img6.jpg"
    };

    // Perform horizontal panorama stitching
    bool success = stitcher.merge_image_path_horizontal(images, "panorama.jpg");

    if (success) {
        // Post-process: Crop and adjust aspect ratio
        stitcher.crop_panorama_by_path("panorama.jpg", "final_panorama.jpg");
    }

    return 0;
}
```

## Technical Overview

### Seam Detection
The seam-finding algorithm minimizes an energy function:
$E(i, j) = \sum_{ch} (I_1(i, j) - I_2(i, j))^2 + (\nabla I_1(i, j) + \nabla I_2(i, j))$
This ensures that the seam passes through regions where images are most similar and where there is low detail (avoiding edges).

### Color Correction
To handle varying exposures:
1.  Images are linearized: $L = I^\gamma$ (where $\gamma = 2.2$).
2.  Overlap regions are analyzed to compute gain factors ($\alpha$).
3.  A global adjustment is calculated to minimize variance across the sequence.
4.  Final images are gamma-encoded back to the sRGB space.

## License

This project is licensed under the terms found in the `LICENSE` file.
Built-in dependencies (`libjpeg-turbo`, `SIMDe`) carry their respective licenses.

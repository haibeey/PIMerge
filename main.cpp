#include "PIMerge.hpp"
#include "PiImage.hpp"
#include "jpeg.hpp"
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
  #include <windows.h>
  #include <psapi.h>
#elif defined(__APPLE__) || defined(__linux__)
  #include <sys/resource.h>
  #include <unistd.h>
#endif

void print_memory_usage(const std::string& label) {
    double rss_mb = 0.0;

  #if defined(_WIN32)
    PROCESS_MEMORY_COUNTERS info;
    if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
      rss_mb = info.WorkingSetSize / 1024.0 / 1024.0;
    }
  #elif defined(__APPLE__) || defined(__linux__)
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
      long rss_kb;
    # if defined(__APPLE__)
      // macOS gives ru_maxrss in bytes
      rss_kb = usage.ru_maxrss / 1024;
    # else
      // Linux gives ru_maxrss in KB
      rss_kb = usage.ru_maxrss;
    # endif
      rss_mb = rss_kb / 1024.0;
    }
  #endif

    std::cout << "[MEM] " << label << ": RSS = "
              << rss_mb << " MB\n";
}

int main() {
  print_memory_usage("Start");
  LendoMerge lendoMerge(104, 60);
  print_memory_usage("After LendoMerge init");
  std::vector<std::string> imgs =     std::vector<std::string>{
      "files/debug/1.JPG",
      "files/debug/2.JPG",
      "files/debug/3.JPG",
      "files/debug/4.JPG",
      "files/debug/5.JPG",
      "files/debug/6.JPG",
  };
  lendoMerge.merge_image_path_horizontal(imgs,"out.jpg");

  PiImageU8 img = PiImageU8(create_image("files/debug/1.JPG"));
  print_memory_usage("After create_image");
  lendoMerge.blur_image(img, 0, img.height() - img.height()/2);
  print_memory_usage("After blur_image");
  img.save( "out1.jpg");
  print_memory_usage("After save_image");

  PiImageU8 img2 = PiImageU8(create_image("files/debug/1.JPG"));
  print_memory_usage("After create_image");
  lendoMerge.downsample_image("files/debug/1.JPG","1.JPG",200, 2);
  print_memory_usage("After downsample");
  return 0;
}

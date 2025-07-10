#include "LendoMerge.hpp"
#include "jpeg.h"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
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
  lendoMerge.merge_image_path_horizontal(
      std::vector<std::string>{
          "files/debug/1.JPG",
          "files/debug/2.JPG",
          "files/debug/3.JPG",
          "files/debug/4.JPG",
          "files/debug/5.JPG",
          "files/debug/6.JPG",
      },
      "out.jpg");

  // std::this_thread::sleep_for(std::chrono::seconds(10));
  Image img = create_image("files/debug/1.JPG");
  print_memory_usage("After create_image");
  lendoMerge.blur_image(&img, 0, img.height - img.height/2);
  print_memory_usage("After blur_image");
  save_image(&img, "out1.jpg");
  print_memory_usage("After save_image");
  destroy_image(&img);
  print_memory_usage("After destroy_image");
  std::cout << "done" << std::endl;
  print_memory_usage("After done print");
  return 0;
}

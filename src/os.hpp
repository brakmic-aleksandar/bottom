#pragma once

#include <vector>
#include <string.h>
#include <fstream>
#include <cmath>
#include <filesystem>

using namespace std::string_literals;

struct Process {
  int pid;
  std::string name;
  std::string cmd;
  long long login_uid;
};

struct Memory {
  size_t total;
  size_t free;
};

struct CpuUsage {
  double total;
  std::vector<double> core_usage;
};

std::string read_file(const std::filesystem::path& path) {
  std::ifstream input_file(path);
  if (!input_file.is_open()) {
    return "";
  }
  return std::string(std::istreambuf_iterator<char>(input_file), std::istreambuf_iterator<char>());
}

bool is_process(const std::filesystem::path& proc_entry) {
  auto name = proc_entry.stem().string();
  return (!name.empty()
    && std::find_if(name.begin(), name.end(), [](unsigned char c) { return !std::isdigit(c); }) == name.end())
    && !read_file(proc_entry / "cmdline").empty();
}

Process read_proc_data(int pid) {
  Process proc;
  proc.pid = pid;

  std::ifstream f("/proc/"s + std::to_string(pid) + "/status");
  if (f.is_open()) {
    std::string tp;
    while(std::getline(f, tp)) {
      auto name_column = "Name:"s;
      if(tp.substr(0, name_column.length()) == name_column) {
	proc.name = tp.substr(name_column.length());
      }
    }
    f.close();
  }

  proc.cmd = read_file("/proc/"s + std::to_string(pid) + "/cmdline");
  auto uid = read_file("/proc/"s + std::to_string(pid) + "/loginuid");
  if(!uid.empty()) {
    proc.login_uid = std::stoll(uid);
  }

  return proc;
}

Memory memory() {
  Memory mem;
  std::ifstream f("/proc/meminfo");
  if (f.is_open()) {
    std::string tp;
    while(std::getline(f, tp)) {
      auto name_column = "MemTotal:"s;
      if(tp.substr(0, name_column.length()) == name_column) {
	mem.total = std::stoll(tp.substr(name_column.length()));
      }
      name_column = "MemFree:"s;
      if(tp.substr(0, name_column.length()) == name_column) {
	mem.free = std::stoll(tp.substr(name_column.length()));
      }
    }
    f.close();
  }
  return mem;
}

std::vector<Process> processes() {
  namespace fs = std::filesystem;

  std::vector<Process> processes;

  std::string path = "/proc";
  for (const auto &entry : fs::directory_iterator(path)) {
    auto val = entry.path().stem().string();
    if(is_process(entry)) {
      auto pid = std::stoi(val);
      auto proc = read_proc_data(pid);
      processes.push_back(proc);
    }
  }

  return processes;
}

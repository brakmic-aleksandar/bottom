#include <iostream>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/table.hpp>
#include "ftxui/component/captured_mouse.hpp"  // for ftxui
#include "ftxui/component/component.hpp"       // for Renderer, Button, Vertical
#include "ftxui/component/component_base.hpp"  // for ComponentBase
#include "ftxui/component/screen_interactive.hpp"  // for ScreenInteractive
#include "ftxui/dom/elements.hpp"  // for operator|, Element, text, bold, border, center, color
#include "ftxui/screen/color.hpp"  // for Color, Color::Red

using namespace std::string_literals;

struct Process
{
  int pid;
  std::string name;
  std::string cmd;
  long long login_uid;
};

bool is_process(const std::string& s)
{
    return !s.empty()
	    && std::find_if(s.begin(), s.end(),
			    [](unsigned char c) {
			    return !std::isdigit(c);
			    }) == s.end();
}

std::string read_file(const std::string& path) {
  std::ifstream input_file(path);
    if (!input_file.is_open()) {
      return "";
    }
    return std::string((std::istreambuf_iterator<char>(input_file)), std::istreambuf_iterator<char>());
}

Process read_proc_data(int pid)
{
  Process proc;
  proc.pid = pid;

  std::ifstream f("/proc/"s + std::to_string(pid) + "/status");
  if (f.is_open())
  {
    std::string tp;
    while(std::getline(f, tp))
    {
      auto name_column = "Name:"s;
      if(tp.substr(0, name_column.length()) == name_column)
      {
	proc.name = tp.substr(name_column.length());
      }
    }
    f.close();
  }

  proc.cmd = read_file("/proc/"s + std::to_string(pid) + "/cmdline");
  auto uid = read_file("/proc/"s + std::to_string(pid) + "/loginuid");
  if(!uid.empty()) {
    std::cout << uid << std::endl;
    proc.login_uid = std::stoll(uid);
  }

  return proc;
}

std::vector<Process> list_procs()
{
  namespace fs = std::filesystem;

  std::vector<Process> processes;

  std::string path = "/proc";
  for (const auto &entry : fs::directory_iterator(path))
  {
    auto val = entry.path().stem().string();
    if(is_process(val))
    {
      auto pid = std::stoi(val);
      auto proc = read_proc_data(pid);
      processes.push_back(proc);
    }
  }

  return processes;
}

ftxui::Table create_proc_table(std::vector<Process> procs)
{
  std::vector<std::vector<std::string>> table_data;
  auto &columns = table_data.emplace_back();
  columns.push_back("PID");
  columns.push_back("UID");
  columns.push_back("Name");
  columns.push_back("Command");
  for(auto proc: procs)
  {
   auto& row = table_data.emplace_back();
   row.emplace_back(std::to_string(proc.pid));
   row.emplace_back(std::to_string(proc.login_uid));
   row.emplace_back(proc.name);
   row.emplace_back(proc.cmd);
  }
  return ftxui::Table(table_data);
}

int main(void) {
  using namespace ftxui;
  auto screen = ScreenInteractive::FitComponent();

  auto procs = list_procs();

  auto table = create_proc_table(procs);

  table.SelectAll().Border(LIGHT);
  table.SelectRow(0).Decorate(bold);
  table.SelectRow(0).SeparatorVertical(LIGHT);
  table.SelectRow(0).Border(DOUBLE);
  table.SelectColumn(0).DecorateCells(size(WIDTH, EQUAL, 80) | flex);
  table.SelectColumn(1).DecorateCells(size(WIDTH, EQUAL, 50));
  table.SelectColumn(2).DecorateCells(flex);
  table.SelectColumn(3).DecorateCells(flex);

  auto doc = table.Render() | focus;
  auto table_renderer = Renderer([&] (bool focused) {
      return doc;
  });

  auto resource_usage_renderer = Renderer([&] {
    return text("test");
  });

  screen.Loop(Container::Vertical({
    resource_usage_renderer,
    table_renderer
  }));

  return EXIT_SUCCESS;
}


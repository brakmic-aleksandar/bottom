#include <iostream>
#include <filesystem>
#include <fstream>
#include <cctype>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/table.hpp>
using namespace std::string_literals;
struct Process
{
  int pid;
  std::string name;
};

struct Status
{
  std::string name;
};

bool is_process(const std::string& s)
{
    return !s.empty()
	    && std::find_if(s.begin(), s.end(),
			    [](unsigned char c) {
			    return !std::isdigit(c);
			    }) == s.end();
}

Status read_proc_status(int pid)
{
  Status status;
  std::ifstream f("/proc/"s + std::to_string(pid) + "/status");
  if (f.is_open())
  {
    std::string tp;
    while(std::getline(f, tp))
    {
      auto name_column = "Name:"s;
      //std::cout << tp.substr(0, name_column.length()) << std::endl;
      if(tp.substr(0, name_column.length()) == name_column)
      {
	status.name = tp.substr(name_column.length());
      }
    }
    f.close();
  }
  return status;
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
      auto status = read_proc_status(pid);
      processes.push_back( { pid, status.name });
    }
  }

  return processes;
}

ftxui::Table create_proc_table(std::vector<Process> procs)
{
  std::vector<std::vector<std::string>> table_data;
  auto &columns = table_data.emplace_back();
  columns.push_back("PID");
  columns.push_back("Name");
  for(auto proc: procs)
  {
   auto& row = table_data.emplace_back();
   row.emplace_back(std::to_string(proc.pid));
   row.emplace_back(proc.name);
  }
  return ftxui::Table(table_data);
}

int main(void) {
  using namespace ftxui;

  auto procs = list_procs();
  // Define the document
  auto table = create_proc_table(procs);

  table.SelectAll().Border(LIGHT);
  table.SelectRow(0).Decorate(bold);
  table.SelectRow(0).SeparatorVertical(LIGHT);
  table.SelectRow(0).Border(DOUBLE);

  auto document = table.Render();
  auto screen = Screen::Create(
    Dimension::Fit(document) // Height
  );
  Render(screen, document);

  screen.Print();

  return EXIT_SUCCESS;
}


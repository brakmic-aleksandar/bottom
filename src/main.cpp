#include <iostream>
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

#include "scroller.hpp"
#include "os.hpp"
#include <mutex>
#include <thread>
#include <chrono>
#include <cmath>
#include <cstdlib>

using namespace ftxui;

Table process_table(std::vector<Process> procs) {
  std::vector<std::vector<std::string>> table_data;
  auto &columns = table_data.emplace_back();
  columns.push_back("PID");
  columns.push_back("UID");
  columns.push_back("Name");
  columns.push_back("Command");
  for(auto proc: procs) {
   auto& row = table_data.emplace_back();
   row.emplace_back(std::to_string(proc.pid));
   row.emplace_back(std::to_string(proc.login_uid));
   row.emplace_back(proc.name);
   row.emplace_back(proc.cmd);
  }

  auto table = Table(table_data);
  table.SelectAll().Border(LIGHT);
  table.SelectRow(0).Decorate(bold);
  table.SelectRow(0).SeparatorVertical(LIGHT);
  table.SelectRow(0).Border(DOUBLE);
  table.SelectColumn(0).DecorateCells(size(WIDTH, EQUAL, 50));
  table.SelectColumn(1).DecorateCells(size(WIDTH, EQUAL, 50));
  table.SelectColumn(2).DecorateCells(size(WIDTH, EQUAL, 150));

  return table;
}

Element render_process_table() {
  auto procs = processes();
  auto proc = process_table(procs);
  return proc.Render();
}

int main(void) {

  auto total_mem = memory().total;

  auto screen = ScreenInteractive::FitComponent();

  //Process table
  auto pt_doc = render_process_table();
  auto process_table = Scroller(Renderer([&] (bool focused) {
      return pt_doc;
  }));

  //Resource usage
  //Memory usage
  std::vector<size_t> free_memory;

  auto mem_usage_func = [&free_memory, total_mem](int width, int height) {
    std::vector<int> output(width, 0);

    //We will need to copy last n elements from free memory graph, where n = width of graph
    auto start = free_memory.size() <= width ? free_memory.begin() : free_memory.end() - width;
    auto end = free_memory.end();

    std::transform(start, end, output.begin(), [total_mem, height](auto datum) {
        //transform into used memory precentage
        float used_mem_precentage = (total_mem - datum) * 100 / total_mem;
        //scale to graph height
        return (int) used_mem_precentage * ((float) height/100);
    });
    return output;
  };

  auto used_mem_color = [&] {
    auto fm = free_memory.size() > 0 ? free_memory.back() : total_mem;
    auto used_mem = (total_mem - fm) * 100 / total_mem;
    if(used_mem < 50) {
      return Color::White;
    } else if(used_mem < 70) {
      return Color::YellowLight;
    } else {
      return Color::RedLight;
    }
  };

  auto used_mem_str = [&] {
    auto fm = free_memory.size() > 0 ? free_memory.back() : total_mem;
    auto used_mem = total_mem - fm;
    return std::to_string(used_mem) + "/" + std::to_string(total_mem) + " kb";
  };

  auto resource_usage = Renderer([&] {
    return vbox(
             text("Memory usage: " + used_mem_str()) | underlined,
             graph(std::ref(mem_usage_func)) | size(HEIGHT, EQUAL, 10) | color(used_mem_color())
           ) | border;
  });

  //Loop used to refresh data and notify UI thread to refresh
  bool refresh_ui_continue = true;
  std::thread refresh_ui([&] {
    while (refresh_ui_continue) {
      using namespace std::chrono_literals;

      std::this_thread::sleep_for(0.5s);

      auto mem = memory();
      free_memory.push_back(mem.free);
      pt_doc = render_process_table();

      screen.PostEvent(Event::Custom);
    }
  });

  screen.Loop(Container::Vertical({
    resource_usage,
    process_table
  }));

  refresh_ui_continue = false;
  refresh_ui.join();

  return EXIT_SUCCESS;
}


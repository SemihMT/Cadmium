#include <cadmium/core/imgui_layer.hpp>

#ifdef CADMIUM_IMGUI
#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>
#endif

namespace Cadmium
{
  void ImGuiLayer::Init(SDL_Window* window, SDL_Renderer* renderer)
  {
#ifdef CADMIUM_IMGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);
#endif
  }

  void ImGuiLayer::Shutdown()
  {
#ifdef CADMIUM_IMGUI
    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
#endif
  }

  void ImGuiLayer::Begin()
  {
#ifdef CADMIUM_IMGUI
    ImGui_ImplSDLRenderer3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
#endif
  }

  void ImGuiLayer::End(SDL_Renderer* renderer)
  {
#ifdef CADMIUM_IMGUI
    ImGui::Render();
    ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
#endif
  }

  void ImGuiLayer::ProcessEvent(SDL_Event& event)
  {
#ifdef CADMIUM_IMGUI
    ImGui_ImplSDL3_ProcessEvent(&event);
#endif
  }
}

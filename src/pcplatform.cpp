#include "pcplatform.h"


bool mount_filesystem()
{
	return true;
}

void init_imgui(SDL_Window* window) {
// 	ImGui::CreateContext();
// 	ImGuiIO& io = ImGui::GetIO(); (void)io;
// 	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
// 	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
// 	io.IniFilename = nullptr;
// 	// Setup Dear ImGui style
// 	ImGui::StyleColorsDark();
// 	//ImGui::StyleColorsClassic();
// 
// 	// Setup Platform/Renderer bindings
// 	ImGui_ImplSDL2_InitForOpenGL(window, nullptr);
// 	ImGui_ImplOpenGL2_Init();
}

void imgui_newframe(SDL_Window* window) {
	// Start the Dear ImGui frame
// 	ImGui_ImplOpenGL2_NewFrame();
// 	ImGui_ImplSDL2_NewFrame(window);
// 	ImGui::NewFrame();


}
void imgui_input(SDL_Event* event)
{
	/*ImGui_ImplSDL2_ProcessEvent(event);*/
}

void initialize_platform(void) {
}

int refresh_input_device(void)
{
return 0;
}

void close_platform(void)
{
}

bool load_game(PersistentState* state)
{

	return true;
}

bool save_game(PersistentState* state)
{
	return true;
}
#include "Gui.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

bool Gui::Initialize_GUI(GLFWwindow *window, const char* glslVersion)
{

    //store the GLFW window pointer for further use
    mWindow = window;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO(); (void)io;

    //set theme is dark
    ImGui::StyleColorsDark();


    if(!ImGui_ImplGlfw_InitForOpenGL(mWindow, true)) return false;
    if(!ImGui_ImplOpenGL3_Init(glslVersion)) return false;

    //Making sure that the GUI is ready
    mInitialized = true;
    return true;
}

void Gui::BeginFrameGUI()
{
    if(!mInitialized) return;

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Gui::DrawGUI()
{
    if(!mInitialized) return;
    //Set default size and position of the GUI
    ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);


    ImGui::Begin("LiteCityForge");

    ImGui::Separator();

    ImGui::Checkbox("3D mode", &mEnable3D);

    if (ImGui::Button("Generate")) {}
    if (ImGui::Button("Clear")) {}
    
    ImGui::End();

}

void Gui::EndFrameGUI () 
{
    if (!mInitialized) return;

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

}

void Gui::ShutdownGUI()
{
    if (!mInitialized) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    mInitialized = false;
    mWindow = nullptr;
}
# Preparation
1. Register as developer for the unreal engine to get access to the source code
2. Install Visual Studio 2022: select the `Desktop development with C++` workload and additionally install either:
    1. Older version
        * C++ Universal Windows Platform support for v143 build tools (ARM64/ARM64EC)
        * MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools (v14.38-17.8)
        * MSVC v143 - VS 2022 C++ x64/x86 build tools (v14.38-17.8)
        * C++ v14.38 (17.8) MFC for v143 build tools (ARM)
        * C++ v14.38 (17.8) MFC for v143 build tools (ARM64)
        * C++ v14.38 (17.8) MFC for v143 build tools (x86 & x64)
        * C++ v14.38 (17.8) ATL for v143 build tools (ARM)
        * C++ v14.38 (17.8) ATL for v143 build tools (ARM64)
        * C++ v14.38 (17.8) ATL for v143 build tools (x86 & x64)
   2. Newer version
        * C++ Universal Windows Platform support for v143 build tools (ARM64/ARM64EC)
        * MSVC v143 - VS 2022 C++ ARM64/ARM64EC build tools (v14.44-17.14)
        * MSVC v143 - VS 2022 C++ x64/x86 build tools (v14.44-17.14)
        * C++ v14.44 (17.14) MFC for v143 build tools (ARM)
        * C++ v14.44 (17.14) MFC for v143 build tools (ARM64)
        * C++ v14.44 (17.14) MFC for v143 build tools (x86 & x64)
        * C++ v14.44 (17.14) ATL for v143 build tools (ARM)
        * C++ v14.44 (17.14) ATL for v143 build tools (ARM64)
        * C++ v14.44 (17.14) ATL for v143 build tools (x86 & x64)
3. Clone Repository: `git clone https://github.com/Olli1080/UnrealEngine.git -b 5.4.3` (you may wanna use `--single-branch` to avoid overhead)
4. Follow the Instructions in the readme or use the Unreal Engine builder `https://github.com/Olli1080/Unreal-Binary-Builder.git`

# Getting Started
1. Clone Repository: `git clone https://github.com/Olli1080/ar_integration.git -b 5.4.3` .
2. Clone Sub-Repositories `git submodule update --init --recursive`.
3. Run in the checked-out repository: `python setup_dependencies.py`.
4. Install Epic Games Launcher: https://launcher-public-service-prod06.ol.epicgames.com/launcher/api/installer/download/EpicGamesLauncherInstaller.msi
5. Right click on `ar_integration.uproject` and select Generate Visual Studio project files (requires Unreal Engine with support for HoloLens). 
6. Open the generated ar_integration.sln with Visual Studio 2022.
7. Right click on `ar_integration` in the solution explorer and select "Build". If you encounter build errors, examine the vcpkg logs and fix the ports. Use, for instance overlay ports as done in `Plugins\grpc_plugin\Source\overlay`
8. Connect the HoloLens to the same network as the host and note the IP addresses of both devices (will be needed in later steps)
9. Open `ar_integration.uproject` in Unreal Engine.
    1. Edit -> Project Settings -> [On the left of the dialog window] HoloLens under Platforms -> Signing Certificate under Packaging -> Click Generate. Under Toolchain select the installed Windows 10 SDK Version.
    2. Tools -> Find Blueprints -> Find Blueprints 1 -> Enter "192.168.100.100" and search -> double click the first match -> change the IP address to the one of the host
    3. Platforms (dropdown below navigation bar) -> Hololens -> Select "Shipping" and click "Cook Content" (redo this step if it fails)
    4. Create signing keys for the packaging by `Project Properties -> Platforms -> HoloLens -> Signing Certificate -> Generate New (You can leave the password empty)`
    5. Platforms -> Hololens -> Click "Package Project". Select the project root or create some folder as destination folder in the ar_integration repository.
10. Enter the IP address of the HoloLens in Edge browser to get the web interface: Views -> Apps -> Click "Allow me to select framework packages" and "Choose File" -> Select the files from step 10.4 (in the first step "ar_integration.appxbundle" and in the second step "Microsoft.VCLibs.arm64.14.00.appx")
11. Start the main application on the host
12. Start ArSurvey on the HoloLens.

# User Selection Flow (Assignments)

  1. `A_procedural_mesh_actor::handle_begin_grab` opens `A_assignment_menu_actor` only when the mesh is selectable and scenario is not `BASELINE`.
  2. `A_assignment_menu_actor::build_buttons` refreshes scenario and shows only allowed buttons.
  3. On button press, `A_assignment_menu_actor::handle_assignment` sets assignment mode, sends selection via game state, and updates local label/state.
  4. `A_integration_game_state::select_mesh_by_actor` resolves `object_id` and `pn_id` and calls `U_selection_client::send_selection`.
  5. `U_selection_client::send_selection` transmits `{object_id, pn_id, assignment}` through gRPC service `selection_com/send_selection`.

# Development (Advanced)
- If you want to see something in the editor you will want to click the following menu `Create -> Only Create Lighting`
- Every time you make changes to the code you have to
    1. Compile the changes for arm64 HoloLens in Visual Studio
    2. Package for HoloLens in Editor
    3. Upload the new package via device portal
    4. Compile or Hot-Reload the changes for development editor x64, to keep the environments behaviour in sync (optional)

# Troubleshooting
- To spare future developers hours and hours of pain and suffering, here comes one the most important pieces of information:

## HOW TO DEBUG A HOLOLENS APP WHILE IT IS RUNNING ON THE DEVICE

0. INSTALL THE WIN-UI DEVELOPMENT WORKLOAD IN VISUAL STUDIO
1. In Visual Studio, open Debug -> Other Debug Targets -> Debug Installed App Package.
2. The connection type must be "Remote machine"
3. Enter the IP address of the HoloLens device in the Adress field and let the auth method on "Universal (Unencrypted Protocol)".
4. After some time (i may take some), you can pair with the device by entering the PIN shown on the HoloLens (Settings --> Update and Security --> For Developers --> Pair device).
5. Select your app from the list and make sure "Enable native code debugging" is checked.
6. Click "Start" to start debugging. (the App will then Start on the HoloLens and Debug info will be transmitted to your Visual Studio) --> Make sure to use a build that enables debug information
7. Read the output window for information about crashes, logs, etc. 
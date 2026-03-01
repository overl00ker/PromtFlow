# PromtFlow

![C++](https://img.shields.io/badge/C++-17-blue.svg)
![Qt](https://img.shields.io/badge/Qt-6.0+-41CD52.svg)
![CMake](https://img.shields.io/badge/CMake-3.15+-green.svg)

**PromtFlow** is a powerful, node-based visual editor designed to streamline and enhance the process of AI image generation. Powered by the **Google Gemini API** (`gemini-3.1-pro-preview` and `gemini-3.1-flash-image-preview`), it allows users to visually construct complex generation pipelines by combining text prompts, image references, and stylistic presets into a cohesive workflow.

---

## ✨ Key Features

*   **Visual Node Interface:** An intuitive, drag-and-drop node graph built on Qt's Graphics View Framework. Connect your ideas visually rather than writing long, messy text blocks.
*   **Multi-Modal Image References:** Seamlessly upload multiple reference images and assign them specific visual roles (`Character`, `Pose`, `Style`). You can even add custom override instructions to each image!
*   **Two-Stage Generation Engine:**
    *   **JSON Builder:** First, PromtFlow uses Gemini Pro to analyze your inputs (text + images) and synthesize them into a highly descriptive, structured JSON prompt under the hood.
    *   **Image Generation:** The synthesized JSON is then sent to Gemini Flash Image models to render a breathtaking result.
*   **Iterative Image Editing:** Directly edit generated images by providing new natural language instructions to the AI.
*   **Workflow Management:** Save your entire node graph setup to a `.json` file and load it later to continue your work.
*   **History Tracking:** A built-in history panel saves all your generated images along with the exact metadata (prompts, style presets, API data) used to create them.

---

## 🚀 Getting Started

### 1. Download the App
For standard users, simply navigate to the **[Releases](https://github.com/overl00ker/PromtFlow/releases)** page on this GitHub repository, download the latest `.zip` archive, extract it, and double-click `PromtFlow.exe` to start. No installation required!

### 2. Configure your API Key
PromtFlow utilizes the Google Gemini API to power its generation engine. 
1. Get a free API key from Google AI Studio.
2. In the app, go to **Settings -> API** (or press `Ctrl+,`).
3. Paste your API key. It will be securely stored in your local Windows registry.

### 3. Create your first Workflow
1. **Right-click** anywhere on the canvas to open the node creation menu.
2. Add a **Text Prompt** node and an **Image Reference** node.
3. Add a **JSON Builder** node and connect your text/images to its inputs.
4. Add a **Generation** node and connect the `json_data` output to it.
5. Click **Make JSON** on the JSON Builder, then click **Generate** on the final node!

---

## 🛠️ Building from Source (For Developers)

If you wish to compile PromtFlow yourself or contribute to the code:

### Prerequisites
*   **C++17** compatible compiler (e.g., MSVC 2022).
*   **Qt 6** (Open Source or Commercial) with the `QtCore`, `QtGui`, `QtWidgets`, and `QtNetwork` modules.
*   **CMake** (3.15 or higher).

### Build Instructions (Windows / Visual Studio)
1. Clone the repository:
   ```cmd
   git clone https://github.com/overl00ker/PromtFlow.git
   cd PromtFlow
   ```
2. Open the directory as a CMake project in Microsoft Visual Studio 2022.
3. Ensure the active configuration is `x64-Release` or `x64-Debug`.
4. Click **Build -> Rebuild All**. Visual Studio and CMake will automatically configure the Qt dependencies and compile the executable.
5. Use `windeployqt` to pack the `.exe` with the required Qt `.dll` files if you plan to distribute your build.

---

## 🏗️ Project Architecture

*   `src/app/` - Main application entry points, Settings Dialog, and Main Window logic.
*   `src/graph/` - Core node-graph implementation (`NodeScene`, `NodeItem`, `ConnectionLine`). Handles visual rendering, dragging, and workflow serialization.
*   `src/nodes/` - Individual node definitions implementing custom logic (e.g., `GenerationNode`, `JsonPromptNode`).
*   `src/network/` - Network access layer (`GeminiApiClient`). Wraps the asynchronous HTTP REST requests for the Google Gemini generation endpoints.
*   `src/widgets/` - Reusable UI widgets, including the `HistoryPanel` dock.

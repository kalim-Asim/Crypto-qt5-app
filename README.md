# 🛡️ Crypto-App1

Crypto-App1 is a desktop application built with **C++** and the **Qt framework** that provides a user-friendly interface for performing various cryptographic operations. It leverages the powerful **Crypto++** library to offer a range of functionalities, including AES encryption/decryption, SHA-256 hashing, and HMAC digest generation.

## ✨ Features

*   **🔑 Symmetric Key Generation:** Generate AES symmetric keys for secure encryption.
*   **🔒 AES Encryption:** Encrypt files using AES symmetric encryption.
*   **🔓 AES Decryption:** Decrypt AES-encrypted files back to their original content.
*   **📝 SHA-256 Digest Generation:** Compute SHA-256 hash digests for files or text input.
*   **🔐 HMAC Digest Generation:** Generate HMAC digests using SHA-256 for message authentication.

## 🖥️ GUI

The application provides a simple and intuitive graphical user interface with the following key components:

*   **Operation Dropdown**: A combo box that allows you to select the desired cryptographic operation (e.g., AES Encrypt, AES Decrypt, Generate SHA-256 Digest, Generate HMAC).
*   **📝 Input Text Field**: A text area where you can directly type or paste plaintext for hashing or HMAC generation.
*   **📂 Upload Button**: Opens a file dialog to select an input file for operations that require file-based input (e.g., encryption or decryption).
*   **▶️ Process Button**: Executes the currently selected cryptographic operation using the provided input (text or file).
*   **💾 Download Button**: Becomes active after an operation that produces an output (like encryption, decryption, or digest). Clicking it opens a save file dialog to save the generated output to your local system.
*   **📊 Status Log**: A text area that displays real-time progress, success messages, and any errors encountered during operations.
*   **⏳ Progress Bar**: (Optional) A visual indicator that shows the progress of longer operations, though most cryptographic operations are very fast.

## Project layout
```
CryptoQtApp/
├── CMakeLists.txt
├── config.json
├── data.hmac
├── README.md
├── src/
│   ├── main.cpp
│   ├── mainwindow.h
│   ├── mainwindow.cpp
│   └── resources.qrc
└── build/
```


### 📂 Key Components

*   **`src/main.cpp`**: The main entry point of the application.
*   **`src/`**: Contains the Qt-based graphical user interface code.
    *   `mainWindow.h`, `mainWindow.cpp` define the main window of the application.
*   **`data.hmac`**: Stores the HMAC digest of application data to ensure integrity and authenticity.
*   **`config.json`**: Configuration file for the application.
*   **`CMakeLists.txt`**: The build script for the project. -->

## 🚀 Building the Project

The project is built using **CMake**. You will need to have CMake and a C++ compiler installed on your system. You will also need to have the **Qt6 development libraries** installed.

1.  **🔧 Clone the repository:**
    ```bash
    https://github.com/Cyber-Security-July-Dec-2025/C19.git
    ```
    
2.  **⚙️ Build (Ubuntu/Debian example)**
 -  ***Install Qt + Crypto++**
   ```bash
   sudo apt update
   sudo apt install -y build-essential cmake qtbase5-dev libqt5widgets5 libcrypto++-dev
   ```
-  ***▶️ Run Cmake and Build the project***
  ```bash
   mkdir build && cd build
   cmake ..
   cmake --build .
   ./CryptoQtApp
   ```

--Windows
 ```bash
 cmake --build . --config Release
 ```

- If using Qt6 adjust CMake find_package to Qt6 and install `qt6-base-dev`.
  
## ⚙️ Usage

Once the project is built, you can run the executable from the build directory. The application will launch a graphical user interface that allows you to select a cryptographic operation, choose a file, and perform the operation.  

* For AES operations → Upload a file and select **Encrypt** or **Decrypt**.  
* For SHA-256 digest → Provide text or upload a file.  
* For HMAC digest → Provide text or file, and the app will generate the HMAC.  

## 🛠️ Dependencies
*   **C++17**
*   **Qt6:** A cross-platform application development framework.
*   **Crypto++:** A free C++ class library of cryptographic schemes.
*   **CMake:** Build system generator.

## 📖 Notes
- `config.json` holds crypto parameters (key/IV sizes).
- Encrypted files have IV (raw bytes) prepended.
- The app reads files into memory (not streaming). For very large files, adapt to streaming.

### Example `config.json`

```json
{
  "aes_mode": "CBC",
  "aes_key_bytes": 32,
  "aes_iv_bytes": 16,
  "hmac_key_bytes": 32,
  "hash_algorithm": "SHA-256"
}
```
## Team Members
- Avaneesh Karthik S (IIT2023237)
- Anurag Singh Sikarwar (IIT2023242)
- Vedant Soni (IIT2023234)
- Asim Kalim (IIT2023219)
  
## 📸 Screenshots
1. UI of App
  <img width="735" height="608" alt="Screenshot 2025-09-28 160357" src="https://github.com/user-attachments/assets/16c515c9-cb3a-41c8-b261-4849a447acf5" />


2. Dropdown menu
  <img width="731" height="247" alt="Screenshot 2025-09-28 160408" src="https://github.com/user-attachments/assets/b31ea7ed-8562-4a83-9a3b-6849a7021322" />


3. Upload file
  <img width="743" height="624" alt="Screenshot 2025-09-28 160418" src="https://github.com/user-attachments/assets/cf751da8-b198-4428-b466-2121a584d1fe" />


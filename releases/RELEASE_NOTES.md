# ISHA AI — Release v1.0.0

## 📦 Downloadable App Distributions

### 1. 📱 Android Application (Native APK)
- **File**: [`isha-ai-v1.0.0.apk`](./isha-ai-v1.0.0.apk)
- **Architecture**: `arm64-v8a` (Modern Android smartphones)
- **Size**: ~10.8 MB
- **Installation**:
  1. Download [`isha-ai-v1.0.0.apk`](./isha-ai-v1.0.0.apk) onto your Android phone or transfer via USB.
  2. Tap on the APK file in your phone's File Manager / Downloads.
  3. When prompted, permit "Install unknown apps" for your browser/file manager.
  4. Tap **Install** and open **ISHA AI**!

### 2. 💻 Progressive Web App (PWA) & Desktop
- **Installation via Browser**:
  1. Start the runtime:
     ```bash
     python run_isha.py
     ```
     (or double-click `run_isha.bat` on Windows).
  2. Open in Chrome or Edge (`http://127.0.0.1:8000`).
  3. Click the **📲 Install App** button in the top header (or click the install icon in the URL address bar).
  4. ISHA AI is now installed on your desktop/mobile home screen as a standalone application.

### 3. ☁️ Automated GitHub Releases & CI/CD
- Whenever you push a tag `v*` (e.g., `git tag v1.0.0 && git push origin v1.0.0`) or trigger the GitHub Actions workflow manually:
  - GitHub Actions automatically compiles the Android APK and desktop bundle.
  - The packages appear under the **Releases** tab of your GitHub repository for 1-click public download.

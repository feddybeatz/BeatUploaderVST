# Build the BeatUploader installer

`BeatUploader.exe` is the standalone application, not an installer. The actual installer is created by compiling `BeatUploader.iss`; the current script packages both the standalone app and the VST3 bundle already staged at the project root.

## Build

1. Install Inno Setup 6 from its official download page.
2. Open `BeatUploader.iss` in Inno Setup Compiler.
3. Confirm `BeatUploader.exe` and the complete `BeatUploader.vst3` folder exist in the project root.
4. Click **Build → Compile** (or press F9).
5. The generated installer will be `installer/Output/BeatUploader-Installer.exe`.
6. Test the installer on a separate Windows account or Windows Sandbox. Choose **Choose components** on the install-type page to show checkboxes for **Standalone app** and **VST3 plugin**; the default **Full installation** selects both. The standalone app installs to `C:\Program Files\BeatUploader`. The VST3 bundle installs to `C:\Program Files\Common Files\VST3\BeatUploader.vst3`. Installing there requires administrator approval.

## Publish

Create a GitHub Release in `feddybeatz/BeatUploaderVST` and attach the generated file with the exact name `BeatUploader-Installer.exe`. The website's download button points to that release asset:

<https://github.com/feddybeatz/BeatUploaderVST/releases/latest/download/BeatUploader-Installer.exe>

`BeatUploader.exe` alone is not a suitable public download for users who want to install the VST3. Do not upload OAuth client secrets or token files to the website or release.

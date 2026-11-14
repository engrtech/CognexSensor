# Working from another computer

Repository: https://github.com/engrtech/CognexSensor

The GitHub repository lets you edit and build this project without the home PC being online. The current repository is public. Editing this repository does not remotely operate the MCU, A90, or Cognex reader.

## Work laptop

Clone into a new folder:

```powershell
git clone https://github.com/engrtech/CognexSensor.git C:\STM32Projects\CognexMCU
```

Open that folder in your editor or Codex. In STM32CubeIDE, import it using **Existing Projects into Workspace**; its project name remains **CognexMCU**.

Before starting work:

```powershell
git pull --ff-only
```

After editing, review and publish your changes:

```powershell
git status
git diff
git add Core CognexMCU.ioc README.md
git commit -m "Describe the sensor firmware change"
git push
```

Adjust the paths passed to `git add` for the files you changed. GitHub sign-in is required to push. Commit/push before switching computers, and pull before editing on the other computer. Concurrent changes may need merging.

## Build from GitHub

Each push and pull request runs the **Firmware build** workflow. You can also start it from **Actions → Firmware build → Run workflow**.

Open a successful run and download its **CognexMCU-firmware** artifact. It contains the ELF, BIN, MAP, and verification report built from that run's commit. No toolchain is needed on the work laptop to download these files.

The files committed under `review/build` are a known build snapshot. For newer source edits, use the artifact from the matching successful workflow run or rebuild locally. A GitHub edit alone does not update the committed ELF.

If editing through the GitHub browser, commit the change there, wait for the workflow result, and pull it into the local project before further local edits.

## Build locally

Install STM32CubeIDE's ARM toolchain, or put an `arm-none-eabi` toolchain on PATH. From this project folder:

```powershell
.\review\build.ps1
```

For a toolchain outside PATH:

```powershell
.\review\build.ps1 -ToolBin 'C:\path\to\ARM-toolchain\bin'
```

The build detects Windows executable names and Linux tools. The supplied linker scripts retain the inherited 768 KB application flash region.

## Continuing with an assistant

Ask it to read `README.md`, `AGENTS.md`, and the current source. Current behavior is polling, ON/OFF only on changes, orange LED follows detection, and a single SENS? query. Hardware testing is pending until you perform and record it.

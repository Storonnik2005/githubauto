# GitHub Automation Tool

A console application for GitHub repository automation, written in C++.

## Features

- GitHub authentication via browser (using GitHub CLI)
- Creating new repositories with proper username
- Uploading local projects to GitHub
- Updating existing repositories
- Selecting specific files to upload
- Support for both main and master branches
- Authentication status check

## Requirements

- C++ compiler with C++17 support
- Git installed and configured on your computer
- [GitHub CLI](https://cli.github.com/) (`gh`) installed and available in PATH

## Installing GitHub CLI

1. Download and install GitHub CLI from the official website: https://cli.github.com/
2. Make sure `gh` is available in the command line by running: `gh --version`

## Compiling the Program

### Windows (MSVC)

```
cl /EHsc /std:c++17 main.cpp
```

### Windows (MinGW)

```
g++ -std=c++17 main.cpp -o github_automation
```

### Linux/macOS

```
g++ -std=c++17 main.cpp -o github_automation
```

## Usage

1. Run the compiled program
2. On first launch, you'll see the main menu with options
3. You can check authentication status or log in to GitHub before creating/updating projects

### Authentication Options

- Check authentication status: Shows if you're already logged in and displays your GitHub username
- Login to GitHub: Opens a browser window for authentication with GitHub

### Creating a New Project

1. Select option "1. Create a new project"
2. Enter repository name and description
3. Specify whether the repository should be private
4. Enter the path to the local directory with project files
5. Choose whether to select specific files or upload all files
6. Enter a commit message (or leave empty for an automatic message)
7. The program will create the repository, initialize Git, and upload files to GitHub

### Updating an Existing Project

1. Select option "2. Update an existing project"
2. Enter the path to the local project directory
3. Choose whether to select specific files or update all files
4. Enter a commit message
5. The program will upload changes to GitHub

## Advanced Features

- **Selective File Upload**: Choose specific files to include in your commits
- **Branch Detection**: Automatically detects and uses the correct branch (main or master)
- **Repository URL with Username**: Creates proper repository URLs with your GitHub username
- **Git Status Display**: Shows Git status to help you understand what files have changed
- **Automatic Username Detection**: Displays your current GitHub username when logged in 
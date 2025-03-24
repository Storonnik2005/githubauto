#include <iostream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <sstream>

// Windows includes
#ifdef _WIN32
    #include <windows.h>
#endif

namespace fs = std::filesystem;

// Forward declarations
std::string getGitHubUsername();
void parseFileSelection(const std::string& selection, const std::vector<std::string>& availableFiles, std::vector<std::string>& selectedFiles);

// Function to execute commands in terminal
std::string executeCommand(const std::string& command) {
    std::string result;
    char buffer[4096];
    
    #ifdef _WIN32
        FILE* pipe = _popen(command.c_str(), "r");
    #else
        FILE* pipe = popen(command.c_str(), "r");
    #endif
    
    if (!pipe) {
        return "Error executing command";
    }
    
    while (!feof(pipe)) {
        if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
            result += buffer;
        }
    }
    
    #ifdef _WIN32
        _pclose(pipe);
    #else
        pclose(pipe);
    #endif
    
    return result;
}

// Check GitHub CLI authentication
bool checkGitHubAuth() {
    std::string output = executeCommand("gh auth status");
    return output.find("Logged in to github.com") != std::string::npos;
}

// GitHub authentication
bool authenticateGitHub() {
    std::cout << "GitHub authentication in progress..." << std::endl;
    
    // Проверим сначала текущий статус
    if (checkGitHubAuth()) {
        std::string username = getGitHubUsername();
        std::cout << "Already authenticated with GitHub as: " << username << std::endl;
        return true;
    }
    
    // Запустим полный процесс аутентификации через браузер
    std::cout << "Opening browser for GitHub authentication..." << std::endl;
    std::string output = executeCommand("gh auth login -w");
    std::cout << output << std::endl;
    
    // Дополнительная проверка токена
    if (checkGitHubAuth()) {
        std::cout << "Authentication successful!" << std::endl;
        
        // Получение и вывод информации о текущем пользователе
        std::string username = getGitHubUsername();
        if (!username.empty()) {
            std::cout << "Logged in as: " << username << std::endl;
        }
        
        return true;
    } else {
        std::cout << "Authentication failed. Please try again." << std::endl;
        return false;
    }
}

// Create a new repository
std::string createRepository(const std::string& repoName, const std::string& description, bool isPrivate) {
    std::string visibility = isPrivate ? "private" : "public";
    
    // Получение имени пользователя GitHub
    std::string username = getGitHubUsername();
    
    if (username.empty()) {
        std::cout << "Failed to determine GitHub username." << std::endl;
        return "";
    }
    
    std::string command = "gh repo create " + repoName + " --description \"" + description + "\" --" + visibility;
    
    std::cout << "Creating repository '" << repoName << "'..." << std::endl;
    std::string output = executeCommand(command);
    
    if (output.find("Created repository") != std::string::npos || 
        output.find("https://github.com/") != std::string::npos) {
        return "https://github.com/" + username + "/" + repoName;
    } else {
        std::cout << "Error creating repository: " << output << std::endl;
        return "";
    }
}

// Initialize Git in local directory
bool initializeGit(const std::string& localPath, const std::string& repoUrl) {
    // Change to project directory
    fs::current_path(localPath);
    
    // Get default branch name (may be main or master)
    std::string defaultBranch = "main"; // Modern default
    
    // Initialize Git if .git directory doesn't exist
    if (!fs::exists(".git")) {
        std::cout << "Initializing Git repository..." << std::endl;
        executeCommand("git init -b " + defaultBranch);
    } else {
        std::cout << "Git repository already exists, checking current branch..." << std::endl;
        std::string branchOutput = executeCommand("git branch --show-current");
        if (!branchOutput.empty()) {
            // Remove trailing newline
            defaultBranch = branchOutput;
            if (defaultBranch.back() == '\n') {
                defaultBranch.pop_back();
            }
            std::cout << "Using existing branch: " << defaultBranch << std::endl;
        }
    }
    
    // Check if remote already exists
    std::string remoteCheckOutput = executeCommand("git remote -v");
    if (remoteCheckOutput.find("origin") != std::string::npos) {
        std::cout << "Remote 'origin' already exists, updating URL..." << std::endl;
        executeCommand("git remote set-url origin " + repoUrl);
    } else {
        std::cout << "Adding remote 'origin'..." << std::endl;
        executeCommand("git remote add origin " + repoUrl);
    }
    
    return true;
}

// Add files to repository and commit
bool addFilesAndCommit(const std::string& message) {
    executeCommand("git add .");
    std::string output = executeCommand("git commit -m \"" + message + "\"");
    
    return output.find("file changed") != std::string::npos || 
           output.find("files changed") != std::string::npos ||
           output.find("nothing to commit") != std::string::npos;
}

// Push changes to remote repository
bool pushChanges() {
    // Get current branch name
    std::string currentBranch = executeCommand("git branch --show-current");
    // Remove trailing newline
    if (currentBranch.back() == '\n') {
        currentBranch.pop_back();
    }
    
    // If empty, use "main" as default
    if (currentBranch.empty()) {
        currentBranch = "main";
    }
    
    std::cout << "Pushing to branch: " << currentBranch << std::endl;
    std::string output = executeCommand("git push -u origin " + currentBranch);
    return output.find("error") == std::string::npos;
}

// List files in a directory
std::vector<std::string> listFiles(const std::string& path) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.path().filename().string() != ".git") {
                files.push_back(entry.path().filename().string());
            }
        }
    } catch (const std::exception& e) {
        std::cout << "Error listing files: " << e.what() << std::endl;
    }
    return files;
}

// Add selected files to repository and commit
bool addSelectedFilesAndCommit(const std::vector<std::string>& selectedFiles, const std::string& message) {
    // First remove all from staging
    executeCommand("git reset");
    
    // Add each selected file
    for (const auto& file : selectedFiles) {
        std::cout << "Adding file: " << file << std::endl;
        executeCommand("git add \"" + file + "\"");
    }
    
    // Commit the selected files
    std::string output = executeCommand("git commit -m \"" + message + "\"");
    
    return output.find("file changed") != std::string::npos || 
           output.find("files changed") != std::string::npos ||
           output.find("nothing to commit") != std::string::npos;
}

// Function to create a new project
void createProject() {
    std::string repoName, description, localPath, commitMessage;
    char isPrivateChar, selectFilesChar;
    bool isPrivate, selectFiles;
    
    std::cout << "=== Create a New Project ===" << std::endl;
    
    std::cout << "Enter repository name: ";
    std::getline(std::cin, repoName);
    
    std::cout << "Enter repository description: ";
    std::getline(std::cin, description);
    
    std::cout << "Private repository? (y/n): ";
    std::cin >> isPrivateChar;
    std::cin.ignore(); // Clear buffer after character input
    
    isPrivate = (isPrivateChar == 'y' || isPrivateChar == 'Y');
    
    // Create repository on GitHub
    std::string repoUrl = createRepository(repoName, description, isPrivate);
    
    if (repoUrl.empty()) {
        std::cout << "Repository creation failed. Exiting." << std::endl;
        return;
    }
    
    std::cout << "Repository created successfully: " << repoUrl << std::endl;
    
    // Ask for local project path
    std::cout << "Enter local project directory path: ";
    std::getline(std::cin, localPath);
    
    // Check if directory exists
    if (!fs::exists(localPath)) {
        std::cout << "Directory doesn't exist. Create it? (y/n): ";
        char createDir;
        std::cin >> createDir;
        std::cin.ignore();
        
        if (createDir == 'y' || createDir == 'Y') {
            fs::create_directories(localPath);
        } else {
            std::cout << "Project creation canceled." << std::endl;
            return;
        }
    }
    
    // Initialize Git in local directory
    if (!initializeGit(localPath, repoUrl)) {
        std::cout << "Failed to initialize Git in local directory." << std::endl;
        return;
    }
    
    // Ask if user wants to select specific files
    std::cout << "Do you want to select specific files to upload? (y/n): ";
    std::cin >> selectFilesChar;
    std::cin.ignore();
    
    selectFiles = (selectFilesChar == 'y' || selectFilesChar == 'Y');
    
    // Add files to repository
    std::cout << "Enter commit message (leave empty for auto-commit): ";
    std::getline(std::cin, commitMessage);
    
    if (commitMessage.empty()) {
        commitMessage = "Initial project upload";
    }
    
    bool commitSuccess = false;
    
    if (selectFiles) {
        // List all files in the directory
        std::vector<std::string> allFiles = listFiles(localPath);
        if (allFiles.empty()) {
            std::cout << "No files found in directory." << std::endl;
            return;
        }
        
        // Let user select files
        std::vector<std::string> selectedFiles;
        std::cout << "Available files:" << std::endl;
        for (size_t i = 0; i < allFiles.size(); ++i) {
            std::cout << i + 1 << ". " << allFiles[i] << std::endl;
        }
        
        std::cout << "Enter file numbers to add (comma-separated, e.g., 1,3,5), or 'all' to select all: ";
        std::string selection;
        std::getline(std::cin, selection);
        
        if (selection == "all") {
            selectedFiles = allFiles;
        } else {
            // Parse comma-separated list
            std::string number;
            for (char c : selection) {
                if (c == ',') {
                    if (!number.empty()) {
                        int index = std::stoi(number) - 1;
                        if (index >= 0 && index < static_cast<int>(allFiles.size())) {
                            selectedFiles.push_back(allFiles[index]);
                        }
                        number.clear();
                    }
                } else if (isdigit(c)) {
                    number += c;
                }
            }
            
            // Handle the last number
            if (!number.empty()) {
                int index = std::stoi(number) - 1;
                if (index >= 0 && index < static_cast<int>(allFiles.size())) {
                    selectedFiles.push_back(allFiles[index]);
                }
            }
        }
        
        if (selectedFiles.empty()) {
            std::cout << "No files selected. Project creation canceled." << std::endl;
            return;
        }
        
        std::cout << "Selected files:" << std::endl;
        for (const auto& file : selectedFiles) {
            std::cout << "- " << file << std::endl;
        }
        
        commitSuccess = addSelectedFilesAndCommit(selectedFiles, commitMessage);
    } else {
        commitSuccess = addFilesAndCommit(commitMessage);
    }
    
    if (!commitSuccess) {
        std::cout << "Error creating commit." << std::endl;
        return;
    }
    
    // Push changes to remote repository
    if (!pushChanges()) {
        std::cout << "Error pushing changes to remote repository." << std::endl;
        return;
    }
    
    std::cout << "Project successfully created and uploaded to GitHub!" << std::endl;
}

// Enhanced function to update an existing project
void enhancedUpdateProject() {
    std::string localPath, commitMessage, repoName;
    char selectOption;
    int updateOption;
    
    std::cout << "=== Enhanced Project Update ===" << std::endl;
    
    // Ask for repository name
    std::cout << "Enter repository name: ";
    std::getline(std::cin, repoName);
    
    // Ask for local project path
    std::cout << "Enter local project directory path: ";
    std::getline(std::cin, localPath);
    
    // Check if directory exists
    if (!fs::exists(localPath)) {
        std::cout << "Directory doesn't exist. Exiting." << std::endl;
        return;
    }
    
    // Change to project directory
    fs::current_path(localPath);
    
    // Check if it's a Git repository
    if (!fs::exists(".git")) {
        std::cout << "The directory is not a Git repository. Exiting." << std::endl;
        return;
    }
    
    // Show current status
    std::string status = executeCommand("git status -s");
    std::cout << "\nCurrent git status:" << std::endl;
    std::cout << status << std::endl;
    
    // Ask what user wants to do
    std::cout << "\nWhat would you like to do?" << std::endl;
    std::cout << "1. Add new files only" << std::endl;
    std::cout << "2. Update existing files only" << std::endl;
    std::cout << "3. Add new files and update existing files" << std::endl;
    std::cout << "Your choice: ";
    std::cin >> updateOption;
    std::cin.ignore(); // Clear buffer
    
    // List all files in the directory
    std::vector<std::string> allFiles = listFiles(localPath);
    if (allFiles.empty()) {
        std::cout << "No files found in directory." << std::endl;
        return;
    }
    
    // Check for untracked (new) files
    std::string untrackedFiles = executeCommand("git ls-files --others --exclude-standard");
    std::vector<std::string> newFiles;
    std::string currentFile;
    std::istringstream untrackedStream(untrackedFiles);
    while(std::getline(untrackedStream, currentFile)) {
        if (!currentFile.empty()) {
            // Remove trailing newlines/whitespace
            if (currentFile.back() == '\n' || currentFile.back() == '\r')
                currentFile.pop_back();
            newFiles.push_back(currentFile);
        }
    }
    
    // Check for modified files
    std::string modifiedFiles = executeCommand("git ls-files --modified");
    std::vector<std::string> changedFiles;
    std::istringstream modifiedStream(modifiedFiles);
    while(std::getline(modifiedStream, currentFile)) {
        if (!currentFile.empty()) {
            // Remove trailing newlines/whitespace
            if (currentFile.back() == '\n' || currentFile.back() == '\r')
                currentFile.pop_back();
            changedFiles.push_back(currentFile);
        }
    }
    
    std::vector<std::string> selectedFiles;
    
    // Process based on user choice
    switch(updateOption) {
        case 1: // Add new files only
            if (newFiles.empty()) {
                std::cout << "No new files found to add." << std::endl;
                return;
            }
            
            std::cout << "\nNew files available:" << std::endl;
            for (size_t i = 0; i < newFiles.size(); ++i) {
                std::cout << i + 1 << ". " << newFiles[i] << std::endl;
            }
            
            // Ask if user wants to select specific files
            std::cout << "Do you want to select specific files? (y/n): ";
            std::cin >> selectOption;
            std::cin.ignore();
            
            if (selectOption == 'y' || selectOption == 'Y') {
                std::cout << "Enter file numbers to add (comma-separated, e.g., 1,3,5), or 'all' to select all: ";
                std::string selection;
                std::getline(std::cin, selection);
                
                if (selection == "all") {
                    selectedFiles = newFiles;
                } else {
                    // Parse selection
                    parseFileSelection(selection, newFiles, selectedFiles);
                }
            } else {
                selectedFiles = newFiles;
            }
            break;
            
        case 2: // Update existing files only
            if (changedFiles.empty()) {
                std::cout << "No modified files found to update." << std::endl;
                return;
            }
            
            std::cout << "\nModified files available:" << std::endl;
            for (size_t i = 0; i < changedFiles.size(); ++i) {
                std::cout << i + 1 << ". " << changedFiles[i] << std::endl;
            }
            
            // Ask if user wants to select specific files
            std::cout << "Do you want to select specific files? (y/n): ";
            std::cin >> selectOption;
            std::cin.ignore();
            
            if (selectOption == 'y' || selectOption == 'Y') {
                std::cout << "Enter file numbers to update (comma-separated, e.g., 1,3,5), or 'all' to select all: ";
                std::string selection;
                std::getline(std::cin, selection);
                
                if (selection == "all") {
                    selectedFiles = changedFiles;
                } else {
                    // Parse selection
                    parseFileSelection(selection, changedFiles, selectedFiles);
                }
            } else {
                selectedFiles = changedFiles;
            }
            break;
            
        case 3: // Add new files and update existing
            {
                // Combine new and modified files
                std::vector<std::string> combinedFiles;
                combinedFiles.insert(combinedFiles.end(), newFiles.begin(), newFiles.end());
                combinedFiles.insert(combinedFiles.end(), changedFiles.begin(), changedFiles.end());
                
                if (combinedFiles.empty()) {
                    std::cout << "No files found to add or update." << std::endl;
                    return;
                }
                
                std::cout << "\nAvailable files to add/update:" << std::endl;
                for (size_t i = 0; i < combinedFiles.size(); ++i) {
                    std::string prefix = "";
                    if (std::find(newFiles.begin(), newFiles.end(), combinedFiles[i]) != newFiles.end()) {
                        prefix = "[NEW] ";
                    } else {
                        prefix = "[MOD] ";
                    }
                    std::cout << i + 1 << ". " << prefix << combinedFiles[i] << std::endl;
                }
                
                // Ask if user wants to select specific files
                std::cout << "Do you want to select specific files? (y/n): ";
                std::cin >> selectOption;
                std::cin.ignore();
                
                if (selectOption == 'y' || selectOption == 'Y') {
                    std::cout << "Enter file numbers to process (comma-separated, e.g., 1,3,5), or 'all' to select all: ";
                    std::string selection;
                    std::getline(std::cin, selection);
                    
                    if (selection == "all") {
                        selectedFiles = combinedFiles;
                    } else {
                        // Parse selection
                        parseFileSelection(selection, combinedFiles, selectedFiles);
                    }
                } else {
                    selectedFiles = combinedFiles;
                }
            }
            break;
            
        default:
            std::cout << "Invalid option. Exiting." << std::endl;
            return;
    }
    
    // If no files selected
    if (selectedFiles.empty()) {
        std::cout << "No files selected. Update canceled." << std::endl;
        return;
    }
    
    // Show selected files
    std::cout << "\nSelected files for processing:" << std::endl;
    for (const auto& file : selectedFiles) {
        std::cout << "- " << file << std::endl;
    }
    
    // Get commit message
    std::cout << "\nEnter commit message (leave empty for auto-commit): ";
    std::getline(std::cin, commitMessage);
    
    if (commitMessage.empty()) {
        commitMessage = "Update for " + repoName;
    }
    
    // First reset staging area
    executeCommand("git reset");
    
    // Add selected files
    bool commitSuccess = true;
    for (const auto& file : selectedFiles) {
        std::cout << "Adding file: " << file << std::endl;
        std::string addOutput = executeCommand("git add \"" + file + "\"");
        if (addOutput.find("error") != std::string::npos) {
            std::cout << "Error adding file: " << file << std::endl;
            commitSuccess = false;
        }
    }
    
    if (!commitSuccess) {
        std::cout << "Errors occurred while adding files. Proceeding with commit anyway..." << std::endl;
    }
    
    // Create commit
    std::string commitOutput = executeCommand("git commit -m \"" + commitMessage + "\"");
    commitSuccess = (commitOutput.find("file changed") != std::string::npos || 
                      commitOutput.find("files changed") != std::string::npos);
    
    if (!commitSuccess) {
        std::cout << "Error creating commit: " << commitOutput << std::endl;
        return;
    }
    
    std::cout << commitOutput << std::endl;
    
    // Push changes
    std::cout << "Pushing changes to GitHub..." << std::endl;
    if (!pushChanges()) {
        std::cout << "Error pushing changes to remote repository." << std::endl;
        return;
    }
    
    std::cout << "Project successfully updated and changes uploaded to GitHub!" << std::endl;
}

// Helper function to parse file selection
void parseFileSelection(const std::string& selection, const std::vector<std::string>& availableFiles, std::vector<std::string>& selectedFiles) {
    std::string number;
    for (char c : selection) {
        if (c == ',') {
            if (!number.empty()) {
                int index = std::stoi(number) - 1;
                if (index >= 0 && index < static_cast<int>(availableFiles.size())) {
                    selectedFiles.push_back(availableFiles[index]);
                }
                number.clear();
            }
        } else if (isdigit(c)) {
            number += c;
        }
    }
    
    // Handle the last number
    if (!number.empty()) {
        int index = std::stoi(number) - 1;
        if (index >= 0 && index < static_cast<int>(availableFiles.size())) {
            selectedFiles.push_back(availableFiles[index]);
        }
    }
}

// Get GitHub username
std::string getGitHubUsername() {
    std::string userInfo = executeCommand("gh api user");
    std::string username;
    
    size_t loginPos = userInfo.find("\"login\":");
    if (loginPos != std::string::npos) {
        size_t startQuote = userInfo.find("\"", loginPos + 8) + 1;
        size_t endQuote = userInfo.find("\"", startQuote);
        username = userInfo.substr(startQuote, endQuote - startQuote);
    }
    
    return username;
}

// Show auth status
void showAuthStatus() {
    std::cout << "Checking GitHub authentication status..." << std::endl;
    std::string output = executeCommand("gh auth status");
    std::cout << output << std::endl;
    
    if (checkGitHubAuth()) {
        std::string username = getGitHubUsername();
        if (!username.empty()) {
            std::cout << "Currently logged in as: " << username << std::endl;
        }
    }
}

int main() {
    std::cout << "=== GitHub Automation Tool ===" << std::endl;
    
    // Check for GitHub CLI
    std::string ghVersion = executeCommand("gh --version");
    if (ghVersion.find("gh version") == std::string::npos) {
        std::cout << "GitHub CLI not installed. Please install it from https://cli.github.com/" << std::endl;
        return 1;
    }
    
    int choice = 0;
    do {
        std::cout << "\nSelect an action:" << std::endl;
        std::cout << "1. Create a new project" << std::endl;
        std::cout << "2. Update an existing project" << std::endl;
        std::cout << "3. Check authentication status" << std::endl;
        std::cout << "4. Login to GitHub" << std::endl;
        std::cout << "0. Exit" << std::endl;
        std::cout << "Your choice: ";
        std::cin >> choice;
        std::cin.ignore(); // Clear buffer after number input
        
        switch (choice) {
            case 1:
                // Check authentication first
                if (!checkGitHubAuth()) {
                    std::cout << "GitHub authentication required." << std::endl;
                    if (!authenticateGitHub()) {
                        std::cout << "GitHub authentication failed. Exiting." << std::endl;
                        continue;
                    }
                }
                createProject();
                break;
            case 2:
                // Check authentication first
                if (!checkGitHubAuth()) {
                    std::cout << "GitHub authentication required." << std::endl;
                    if (!authenticateGitHub()) {
                        std::cout << "GitHub authentication failed. Exiting." << std::endl;
                        continue;
                    }
                }
                enhancedUpdateProject();
                break;
            case 3:
                showAuthStatus();
                break;
            case 4:
                authenticateGitHub();
                break;
            case 0:
                std::cout << "Exiting program." << std::endl;
                break;
            default:
                std::cout << "Invalid choice. Please try again." << std::endl;
        }
    } while (choice != 0);
    
    return 0;
} 
#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <algorithm>
#include <cmath>

// Helper: execute a command and return its output as a string
std::string exec(const char* cmd) {
    std::string result;
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        std::cerr << "Error: cannot execute command.\n";
        return "";
    }
    char buffer[128];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);
    return result;
}

bool getVolume(int& vol, bool& muted) {
    std::string output = exec("pactl get-sink-volume @DEFAULT_SINK@");
    if (output.empty()) return false;

    size_t pos = output.find('%');
    if (pos == std::string::npos) return false;

    size_t start = pos;
    while (start > 0 && output[start-1] != '/') --start;
    while (start < pos && !isdigit(output[start])) ++start;
    if (start >= pos) return false;

    std::string numStr = output.substr(start, pos - start);
    vol = std::stoi(numStr);
    vol = std::clamp(vol, 0, 100);

    std::string muteOutput = exec("pactl get-sink-mute @DEFAULT_SINK@");
    muted = (muteOutput.find("Mute: yes") != std::string::npos);
    return true;
}

bool setVolume(int newVol) {
    newVol = std::clamp(newVol, 0, 100);
    std::string cmd = "pactl set-sink-volume @DEFAULT_SINK@ " + std::to_string(newVol) + "%";
    int ret = system(cmd.c_str());
    return (ret == 0);
}

bool increaseVolume(int delta = 5) {
    int vol;
    bool muted;
    if (!getVolume(vol, muted)) return false;
    return setVolume(vol + delta);
}

bool decreaseVolume(int delta = 5) {
    int vol;
    bool muted;
    if (!getVolume(vol, muted)) return false;
    return setVolume(vol - delta);
}

bool toggleMute() {
    int ret = system("pactl set-sink-mute @DEFAULT_SINK@ toggle");
    return (ret == 0);
}

void printUsage(const char* prog) {
    std::cerr << "Usage: " << prog << " [option]\n"
              << "Options:\n"
              << "  --get          Show current volume and mute status\n"
              << "  --set <percent> Set volume to percent (0-100)\n"
              << "  --up [delta]   Increase volume by delta (default 5)\n"
              << "  --down [delta] Decrease volume by delta (default 5)\n"
              << "  --mute         Toggle mute\n"
              << "  --help         Show this help\n"
              << "If no option given, shows current volume.\n";
}

int main(int argc, char* argv[]) {
    if (system("which pactl > /dev/null 2>&1") != 0) {
        std::cerr << "Error: 'pactl' not found. Install pulseaudio-utils.\n";
        return 1;
    }

    if (argc == 1) {
        int vol;
        bool muted;
        if (getVolume(vol, muted)) {
            std::cout << "Volume: " << vol << "%"
                      << (muted ? " [MUTED]" : "")
                      << std::endl;
        } else {
            std::cerr << "Failed to read volume.\n";
            return 1;
        }
        return 0;
    }

    std::string option = argv[1];

    if (option == "--help") {
        printUsage(argv[0]);
        return 0;
    }

    if (option == "--get") {
        int vol;
        bool muted;
        if (getVolume(vol, muted)) {
            std::cout << "Volume: " << vol << "%"
                      << (muted ? " [MUTED]" : "")
                      << std::endl;
        } else {
            std::cerr << "Failed to read volume.\n";
            return 1;
        }
        return 0;
    }

    if (option == "--set") {
        if (argc < 3) {
            std::cerr << "Error: --set requires a percentage argument.\n";
            return 1;
        }
        int vol = std::stoi(argv[2]);
        if (setVolume(vol)) {
            int newVol; bool muted;
            getVolume(newVol, muted);
            std::cout << "Volume set to " << newVol << "%"
                      << (muted ? " [MUTED]" : "")
                      << std::endl;
        } else {
            std::cerr << "Failed to set volume.\n";
            return 1;
        }
        return 0;
    }

    if (option == "--up") {
        int delta = 5;
        if (argc >= 3) delta = std::stoi(argv[2]);
        if (increaseVolume(delta)) {
            int newVol; bool muted;
            getVolume(newVol, muted);
            std::cout << "Volume increased to " << newVol << "%"
                      << (muted ? " [MUTED]" : "")
                      << std::endl;
        } else {
            std::cerr << "Failed to increase volume.\n";
            return 1;
        }
        return 0;
    }

    if (option == "--down") {
        int delta = 5;
        if (argc >= 3) delta = std::stoi(argv[2]);
        if (decreaseVolume(delta)) {
            int newVol; bool muted;
            getVolume(newVol, muted);
            std::cout << "Volume decreased to " << newVol << "%"
                      << (muted ? " [MUTED]" : "")
                      << std::endl;
        } else {
            std::cerr << "Failed to decrease volume.\n";
            return 1;
        }
        return 0;
    }

    if (option == "--mute") {
        if (toggleMute()) {
            int vol; bool muted;
            getVolume(vol, muted);
            std::cout << "Mute " << (muted ? "ON" : "OFF")
                      << " (Volume: " << vol << "%)" << std::endl;
        } else {
            std::cerr << "Failed to toggle mute.\n";
            return 1;
        }
        return 0;
    }

    std::cerr << "Unknown option: " << option << "\n";
    printUsage(argv[0]);
    return 1;
}

#ifndef COCKPIT_COMMAND_HPP
#define COCKPIT_COMMAND_HPP

#include <string>
#include <sstream>
#include <vector>

namespace DeepStrike {

/**
 * CockpitCommand - Commands from GUI to Engine
 * 
 * Protocol format: CMD|TYPE|PARAM1|PARAM2|...
 * 
 * Command Types:
 * - EXECUTE         : Execute pending signal for symbol
 * - MANUAL_EXEC     : Force execute with explicit params
 * - SET_AUTO        : Enable/disable auto execution
 * - SET_SYMBOL      : Set active symbol (GUI focus)
 * - CONFIG          : Update config parameter
 * - RISK            : Update risk parameter
 * - CANCEL          : Cancel pending signal
 * - CLOSE_POSITION  : Close position for symbol
 * - CLOSE_ALL       : Close all positions
 * - PAUSE           : Pause trading
 * - RESUME          : Resume trading
 */

enum class CommandType {
    UNKNOWN,
    EXECUTE,           // CMD|EXECUTE|BTCUSDT
    MANUAL_EXEC,       // CMD|MANUAL_EXEC|BTCUSDT|BUY|0.001|91850.0
    SET_AUTO,          // CMD|SET_AUTO|true
    SET_SYMBOL,        // CMD|SET_SYMBOL|XAUUSD
    CONFIG,            // CMD|CONFIG|spread_bps|4.0
    RISK,              // CMD|RISK|max_pos|50
    CANCEL,            // CMD|CANCEL|BTCUSDT
    CLOSE_POSITION,    // CMD|CLOSE|BTCUSDT
    CLOSE_ALL,         // CMD|CLOSE_ALL
    PAUSE,             // CMD|PAUSE
    RESUME             // CMD|RESUME
};

struct CockpitCommand {
    CommandType type = CommandType::UNKNOWN;
    std::string symbol;
    std::string side;           // BUY/SELL
    double size = 0.0;
    double price = 0.0;
    std::string config_key;
    std::string config_value;
    bool bool_value = false;
    
    // Parse command from string
    // Format: CMD|TYPE|PARAMS...
    static CockpitCommand parse(const std::string& line) {
        CockpitCommand cmd;
        
        // Split by |
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string token;
        while (std::getline(ss, token, '|')) {
            // Trim whitespace
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end = token.find_last_not_of(" \t\r\n");
            if (start != std::string::npos && end != std::string::npos) {
                parts.push_back(token.substr(start, end - start + 1));
            } else if (token.empty() || token.find_first_not_of(" \t\r\n") == std::string::npos) {
                parts.push_back("");
            }
        }
        
        if (parts.size() < 2 || parts[0] != "CMD") {
            return cmd;  // Invalid command
        }
        
        const std::string& type_str = parts[1];
        
        // Parse by command type
        if (type_str == "EXECUTE" && parts.size() >= 3) {
            cmd.type = CommandType::EXECUTE;
            cmd.symbol = parts[2];
        }
        else if (type_str == "MANUAL_EXEC" && parts.size() >= 6) {
            cmd.type = CommandType::MANUAL_EXEC;
            cmd.symbol = parts[2];
            cmd.side = parts[3];
            cmd.size = std::stod(parts[4]);
            cmd.price = std::stod(parts[5]);
        }
        else if (type_str == "SET_AUTO" && parts.size() >= 3) {
            cmd.type = CommandType::SET_AUTO;
            cmd.bool_value = (parts[2] == "true" || parts[2] == "1");
        }
        else if (type_str == "SET_SYMBOL" && parts.size() >= 3) {
            cmd.type = CommandType::SET_SYMBOL;
            cmd.symbol = parts[2];
        }
        else if (type_str == "CONFIG" && parts.size() >= 4) {
            cmd.type = CommandType::CONFIG;
            cmd.config_key = parts[2];
            cmd.config_value = parts[3];
        }
        else if (type_str == "RISK" && parts.size() >= 4) {
            cmd.type = CommandType::RISK;
            cmd.config_key = parts[2];
            cmd.config_value = parts[3];
        }
        else if (type_str == "CANCEL" && parts.size() >= 3) {
            cmd.type = CommandType::CANCEL;
            cmd.symbol = parts[2];
        }
        else if (type_str == "CLOSE" && parts.size() >= 3) {
            cmd.type = CommandType::CLOSE_POSITION;
            cmd.symbol = parts[2];
        }
        else if (type_str == "CLOSE_ALL") {
            cmd.type = CommandType::CLOSE_ALL;
        }
        else if (type_str == "PAUSE") {
            cmd.type = CommandType::PAUSE;
        }
        else if (type_str == "RESUME") {
            cmd.type = CommandType::RESUME;
        }
        
        return cmd;
    }
    
    // Convert to string for logging
    std::string to_string() const {
        std::ostringstream ss;
        switch (type) {
            case CommandType::EXECUTE:
                ss << "EXECUTE " << symbol;
                break;
            case CommandType::MANUAL_EXEC:
                ss << "MANUAL_EXEC " << symbol << " " << side << " " << size << " @ " << price;
                break;
            case CommandType::SET_AUTO:
                ss << "SET_AUTO " << (bool_value ? "true" : "false");
                break;
            case CommandType::SET_SYMBOL:
                ss << "SET_SYMBOL " << symbol;
                break;
            case CommandType::CONFIG:
                ss << "CONFIG " << config_key << "=" << config_value;
                break;
            case CommandType::RISK:
                ss << "RISK " << config_key << "=" << config_value;
                break;
            case CommandType::CANCEL:
                ss << "CANCEL " << symbol;
                break;
            case CommandType::CLOSE_POSITION:
                ss << "CLOSE " << symbol;
                break;
            case CommandType::CLOSE_ALL:
                ss << "CLOSE_ALL";
                break;
            case CommandType::PAUSE:
                ss << "PAUSE";
                break;
            case CommandType::RESUME:
                ss << "RESUME";
                break;
            default:
                ss << "UNKNOWN";
        }
        return ss.str();
    }
};

} // namespace DeepStrike

#endif // COCKPIT_COMMAND_HPP

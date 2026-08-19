#include "config.h"
#include <fstream>
#include <sstream>
#include <string>

Config g_config;

static std::string Trim(const std::string& s) {
    size_t start = s.find_first_not_of(" \t\r\n\"");
    size_t end = s.find_last_not_of(" \t\r\n\"");
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

static float ParseFloat(const std::string& s, float def) {
    try { return std::stof(s); } catch (...) { return def; }
}

static int ParseInt(const std::string& s, int def) {
    try { return std::stoi(s); } catch (...) { return def; }
}

static bool ParseBool(const std::string& s, bool def) {
    std::string t = Trim(s);
    if (t == "true" || t == "1") return true;
    if (t == "false" || t == "0") return false;
    return def;
}

static std::string FindValue(const std::string& content, const std::string& key) {
    std::string search = "\"" + key + "\"";
    size_t pos = content.find(search);
    if (pos == std::string::npos) return "";

    pos = content.find(':', pos);
    if (pos == std::string::npos) return "";
    pos++;

    size_t end = content.find_first_of(",\n}", pos);
    if (end == std::string::npos) end = content.length();

    return Trim(content.substr(pos, end - pos));
}

static std::string FindSection(const std::string& content, const std::string& section) {
    std::string search = "\"" + section + "\"";
    size_t start = content.find(search);
    if (start == std::string::npos) return "";

    start = content.find('{', start);
    if (start == std::string::npos) return "";

    int depth = 1;
    size_t pos = start + 1;
    while (pos < content.length() && depth > 0) {
        if (content[pos] == '{') depth++;
        else if (content[pos] == '}') depth--;
        pos++;
    }

    return content.substr(start + 1, pos - start - 2);
}

void Config::Save(const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "{\n";

    if (aimbot) {
        f << "  \"aimbot\": {\n";
        f << "    \"enabled\": "        << (aimbot->enabled    ? "true" : "false") << ",\n";
        f << "    \"bone_index\": "     << aimbot->bone_index                      << ",\n";
        f << "    \"fov\": "            << aimbot->fov                             << ",\n";
        f << "    \"key\": "            << aimbot->key                             << ",\n";
        f << "    \"team_check\": "     << (aimbot->team_check ? "true" : "false") << ",\n";
        f << "    \"max_distance\": "   << aimbot->max_distance                    << ",\n";
        f << "    \"prediction\": "     << (aimbot->prediction ? "true" : "false") << ",\n";
        f << "    \"bullet_speed\": "   << aimbot->bullet_speed                    << "\n";
        f << "  },\n";
    }

    if (esp) {
        f << "  \"esp\": {\n";
        f << "    \"enabled\": "       << (esp->enabled      ? "true" : "false") << ",\n";
        f << "    \"box\": "           << (esp->box          ? "true" : "false") << ",\n";
        f << "    \"corner_box\": "    << (esp->corner_box   ? "true" : "false") << ",\n";
        f << "    \"box_filled\": "    << (esp->box_filled   ? "true" : "false") << ",\n";
        f << "    \"health_bar\": "    << (esp->health_bar   ? "true" : "false") << ",\n";
        f << "    \"armor_bar\": "     << (esp->armor_bar    ? "true" : "false") << ",\n";
        f << "    \"name\": "          << (esp->name         ? "true" : "false") << ",\n";
        f << "    \"distance\": "      << (esp->distance     ? "true" : "false") << ",\n";
        f << "    \"weapon\": "        << (esp->weapon       ? "true" : "false") << ",\n";
        f << "    \"snaplines\": "     << (esp->snaplines    ? "true" : "false") << ",\n";
        f << "    \"skeleton\": "      << (esp->skeleton     ? "true" : "false") << ",\n";
        f << "    \"head_dot\": "      << (esp->head_dot     ? "true" : "false") << ",\n";
        f << "    \"distance_fade\": " << (esp->distance_fade? "true" : "false") << ",\n";
        f << "    \"max_distance\": "  << esp->max_distance                      << ",\n";
        f << "    \"team_check\": "    << (esp->team_check   ? "true" : "false") << "\n";
        f << "  },\n";
    }

    if (player) {
        f << "  \"player\": {\n";
        f << "    \"god_mode\": " << (player->god_mode ? "true" : "false") << ",\n";
        f << "    \"unlimited_health\": " << (player->unlimited_health ? "true" : "false") << ",\n";
        f << "    \"no_clip\": " << (player->no_clip ? "true" : "false") << ",\n";
        f << "    \"super_jump\": " << (player->super_jump ? "true" : "false") << ",\n";
        f << "    \"run_speed\": " << (player->run_speed ? "true" : "false") << ",\n";
        f << "    \"run_speed_multiplier\": " << player->run_speed_multiplier << ",\n";
        f << "    \"jump_multiplier\": " << player->jump_multiplier << "\n";
        f << "  },\n";
    }

    if (vehicle) {
        f << "  \"vehicle\": {\n";
        f << "    \"god_mode\": " << (vehicle->god_mode ? "true" : "false") << ",\n";
        f << "    \"speed_boost\": " << (vehicle->speed_boost ? "true" : "false") << ",\n";
        f << "    \"boost_speed\": " << vehicle->boost_speed << ",\n";
        f << "    \"fix_on_damage\": " << (vehicle->fix_on_damage ? "true" : "false") << ",\n";
        f << "    \"seatbelt\": " << (vehicle->seatbelt ? "true" : "false") << "\n";
        f << "  }\n";
    }

    f << "}\n";
    f.close();
}

void Config::Load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return;

    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();
    f.close();

    if (aimbot) {
        std::string sec = FindSection(content, "aimbot");
        if (!sec.empty()) {
            std::string val;
            val = FindValue(sec, "enabled");      if (!val.empty()) aimbot->enabled      = ParseBool (val, false);
            val = FindValue(sec, "bone_index");   if (!val.empty()) aimbot->bone_index   = ParseInt  (val, 0);
            val = FindValue(sec, "fov");          if (!val.empty()) aimbot->fov          = ParseFloat(val, 180.f);
            val = FindValue(sec, "key");          if (!val.empty()) aimbot->key          = ParseInt  (val, 2);
            val = FindValue(sec, "team_check");   if (!val.empty()) aimbot->team_check   = ParseBool (val, true);
            val = FindValue(sec, "max_distance"); if (!val.empty()) aimbot->max_distance = ParseFloat(val, 200.f);
            val = FindValue(sec, "prediction");   if (!val.empty()) aimbot->prediction   = ParseBool (val, true);
            val = FindValue(sec, "bullet_speed"); if (!val.empty()) aimbot->bullet_speed = ParseFloat(val, 300.f);
        }
    }

    if (esp) {
        std::string sec = FindSection(content, "esp");
        if (!sec.empty()) {
            std::string val;
            val = FindValue(sec, "enabled");       if (!val.empty()) esp->enabled       = ParseBool (val, false);
            val = FindValue(sec, "box");           if (!val.empty()) esp->box           = ParseBool (val, true);
            val = FindValue(sec, "corner_box");    if (!val.empty()) esp->corner_box    = ParseBool (val, true);
            val = FindValue(sec, "box_filled");    if (!val.empty()) esp->box_filled    = ParseBool (val, false);
            val = FindValue(sec, "health_bar");    if (!val.empty()) esp->health_bar    = ParseBool (val, true);
            val = FindValue(sec, "armor_bar");     if (!val.empty()) esp->armor_bar     = ParseBool (val, true);
            val = FindValue(sec, "name");          if (!val.empty()) esp->name          = ParseBool (val, true);
            val = FindValue(sec, "distance");      if (!val.empty()) esp->distance      = ParseBool (val, true);
            val = FindValue(sec, "weapon");        if (!val.empty()) esp->weapon        = ParseBool (val, true);
            val = FindValue(sec, "snaplines");     if (!val.empty()) esp->snaplines     = ParseBool (val, false);
            val = FindValue(sec, "skeleton");      if (!val.empty()) esp->skeleton      = ParseBool (val, false);
            val = FindValue(sec, "head_dot");      if (!val.empty()) esp->head_dot      = ParseBool (val, true);
            val = FindValue(sec, "distance_fade"); if (!val.empty()) esp->distance_fade = ParseBool (val, true);
            val = FindValue(sec, "max_distance");  if (!val.empty()) esp->max_distance  = ParseFloat(val, 200.f);
            val = FindValue(sec, "team_check");    if (!val.empty()) esp->team_check    = ParseBool (val, true);
        }
    }

    if (player) {
        std::string sec = FindSection(content, "player");
        if (!sec.empty()) {
            std::string val;
            val = FindValue(sec, "god_mode"); if (!val.empty()) player->god_mode = ParseBool(val, false);
            val = FindValue(sec, "unlimited_health"); if (!val.empty()) player->unlimited_health = ParseBool(val, false);
            val = FindValue(sec, "no_clip"); if (!val.empty()) player->no_clip = ParseBool(val, false);
            val = FindValue(sec, "super_jump"); if (!val.empty()) player->super_jump = ParseBool(val, false);
            val = FindValue(sec, "run_speed"); if (!val.empty()) player->run_speed = ParseBool(val, false);
            val = FindValue(sec, "run_speed_multiplier"); if (!val.empty()) player->run_speed_multiplier = ParseFloat(val, 1.0f);
            val = FindValue(sec, "jump_multiplier"); if (!val.empty()) player->jump_multiplier = ParseFloat(val, 1.0f);
        }
    }

    if (vehicle) {
        std::string sec = FindSection(content, "vehicle");
        if (!sec.empty()) {
            std::string val;
            val = FindValue(sec, "god_mode"); if (!val.empty()) vehicle->god_mode = ParseBool(val, false);
            val = FindValue(sec, "speed_boost"); if (!val.empty()) vehicle->speed_boost = ParseBool(val, false);
            val = FindValue(sec, "boost_speed"); if (!val.empty()) vehicle->boost_speed = ParseFloat(val, 100.0f);
            val = FindValue(sec, "fix_on_damage"); if (!val.empty()) vehicle->fix_on_damage = ParseBool(val, false);
            val = FindValue(sec, "seatbelt"); if (!val.empty()) vehicle->seatbelt = ParseBool(val, false);
        }
    }
}

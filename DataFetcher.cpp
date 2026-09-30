#include "DataFetcher.h"

#include <cstdio>
#include <ctime>
#include <sstream>
#include <stdexcept>

const char* const kDefaultSourceUrl = "https://stooq.com/q/d/l/";

namespace {

// Seuls des caractères d'URL sûrs sont acceptés : l'URL est passée à un shell.
bool safeUrl(const std::string& u) {
    if (u.rfind("http://", 0) != 0 && u.rfind("https://", 0) != 0) return false;
    for (char c : u) {
        bool ok = std::isalnum(static_cast<unsigned char>(c)) || std::string(":/.?=&-_%").find(c) != std::string::npos;
        if (!ok) return false;
    }
    return true;
}

} // namespace

std::string httpGet(const std::string& url) {
    if (!safeUrl(url)) throw std::runtime_error("URL invalide ou caractères non autorisés : " + url);
    // -f : échec sur code HTTP >= 400 ; -sS : silencieux mais erreurs affichées ; délais bornés.
    std::string cmd = "curl -fsSL --connect-timeout 10 --max-time 30 \"" + url + "\" 2>&1";
    FILE* p = popen(cmd.c_str(), "r");
    if (!p) throw std::runtime_error("Impossible de lancer curl (est-il installé ?)");
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, p)) > 0) out.append(buf, n);
    int rc = pclose(p);
    if (rc != 0) {
        std::string msg = out.substr(0, 200);
        throw std::runtime_error("Échec du téléchargement de " + url + " : " + (msg.empty() ? "erreur curl" : msg));
    }
    return out;
}

std::map<int, double> parseStooqCSV(const std::string& csv) {
    std::map<int, double> res;
    std::stringstream in(csv);
    std::string line;
    while (std::getline(in, line)) {
        int y, m, d;
        if (std::sscanf(line.c_str(), "%d-%d-%d", &y, &m, &d) != 3) continue; // en-tête / lignes vides
        std::stringstream ls(line);
        std::string field;
        double close = 0;
        bool ok = true;
        for (int i = 0; i < 5; ++i) {
            if (!std::getline(ls, field, ',')) { ok = false; break; }
        }
        if (!ok) continue;
        try { close = std::stod(field); } catch (const std::exception&) { continue; }
        if (m < 1 || m > 12 || close <= 0) continue;
        res[y * 12 + (m - 1)] = close;
    }
    return res;
}

std::vector<MonthlyData> mergeGoldAndFx(const std::map<int, double>& goldUSD,
                                        const std::map<int, double>& eurusd, int months) {
    std::vector<MonthlyData> all;
    for (const auto& kv : goldUSD) {
        auto it = eurusd.find(kv.first);
        if (it == eurusd.end()) continue;
        MonthlyData d;
        d.year = kv.first / 12;
        d.month = kv.first % 12 + 1;
        d.usdPerEur = it->second;
        d.goldEUR = kv.second / it->second; // or USD -> EUR
        all.push_back(d);
    }
    if (all.size() < 6)
        throw std::runtime_error("Données en ligne insuffisantes (moins de 6 mois communs or/EURUSD)");
    if (months > 0 && static_cast<int>(all.size()) > months) all.erase(all.begin(), all.end() - months);
    return all;
}

std::vector<MonthlyData> fetchOnlineHistory(int months, const std::string& baseUrl) {
    std::string sep = baseUrl.find('?') == std::string::npos ? "?" : "&";
    std::map<int, double> gold = parseStooqCSV(httpGet(baseUrl + sep + "s=xauusd&i=m"));
    std::map<int, double> fx = parseStooqCSV(httpGet(baseUrl + sep + "s=eurusd&i=m"));
    if (gold.empty()) throw std::runtime_error("Réponse invalide pour l'or (XAUUSD) : aucune donnée");
    if (fx.empty()) throw std::runtime_error("Réponse invalide pour EURUSD : aucune donnée");

    // exclut le mois en cours (barre incomplète)
    std::time_t now = std::time(nullptr);
    std::tm* t = std::gmtime(&now);
    int cur = (t->tm_year + 1900) * 12 + t->tm_mon;
    gold.erase(cur);
    fx.erase(cur);
    return mergeGoldAndFx(gold, fx, months);
}

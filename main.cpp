#include <cstdlib>
#include <iostream>
#include <string>

#include "DataFetcher.h"
#include "GoldPricePredictor.h"

static void usage(const char* prog) {
    std::cout << "Usage : " << prog << " [options]\n"
              << "  --data <fichier.csv>   historique (date,gold_eur,usd_per_eur ; date = YYYY-MM)\n"
              << "  --online               telecharge l'historique reel (or XAUUSD + EURUSD, Stooq)\n"
              << "  --months <n>           mois d'historique a telecharger (defaut 60)\n"
              << "  --source-url <url>     URL de base de la source (defaut " << kDefaultSourceUrl << ")\n"
              << "  --save-data <fichier>  enregistre l'historique utilise (reutilisable via --data)\n"
              << "  --horizon <mois>       nombre de mois a prevoir (defaut 12)\n"
              << "  --model <linear|holt|auto>  modele (defaut auto = meilleur au backtest)\n"
              << "  --output <fichier.csv> exporte les previsions en CSV\n"
              << "  --help                 affiche cette aide\n"
              << "Sans --data, des donnees d'exemple fictives sont utilisees.\n";
}

int main(int argc, char** argv) {
    std::string dataPath, outPath, savePath, sourceUrl = kDefaultSourceUrl;
    int horizon = 12, months = 60;
    bool online = false;
    ModelType model = ModelType::Auto;

    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        auto next = [&]() -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Valeur manquante pour " << a << "\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "--help" || a == "-h") { usage(argv[0]); return 0; }
        else if (a == "--data") dataPath = next();
        else if (a == "--output") outPath = next();
        else if (a == "--online") online = true;
        else if (a == "--source-url") sourceUrl = next();
        else if (a == "--save-data") savePath = next();
        else if (a == "--months") {
            try { months = std::stoi(next()); }
            catch (const std::exception&) { std::cerr << "Nombre de mois invalide\n"; return 2; }
        }
        else if (a == "--horizon") {
            try { horizon = std::stoi(next()); }
            catch (const std::exception&) { std::cerr << "Horizon invalide\n"; return 2; }
        } else if (a == "--model") {
            std::string m = next();
            if (m == "linear") model = ModelType::Linear;
            else if (m == "holt") model = ModelType::Holt;
            else if (m == "auto") model = ModelType::Auto;
            else { std::cerr << "Modele inconnu : " << m << "\n"; return 2; }
        } else { std::cerr << "Option inconnue : " << a << "\n"; usage(argv[0]); return 2; }
    }

    try {
        GoldPricePredictor predictor;
        if (online && !dataPath.empty()) {
            std::cerr << "--online et --data sont incompatibles\n";
            return 2;
        }
        if (online) {
            std::cout << "Telechargement des donnees en ligne...\n";
            predictor.setData(fetchOnlineHistory(months, sourceUrl));
        } else if (!dataPath.empty()) {
            predictor.loadCSV(dataPath);
        }
        if (!savePath.empty()) predictor.saveHistoryCSV(savePath);
        predictor.predictPrices(horizon, model);
        predictor.printPredictions();
        if (!outPath.empty()) {
            predictor.writeCSV(outPath);
            std::cout << "Previsions exportees vers " << outPath << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << "\n";
        return 1;
    }
    return 0;
}

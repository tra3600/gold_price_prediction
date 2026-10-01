#include <cstdlib>
#include <iostream>
#include <string>

#include "GoldPricePredictor.h"

static void usage(const char* prog) {
    std::cout << "Usage : " << prog << " [options]\n"
              << "  --csv FICHIER     données historiques (mois,or_eur,taux_usd_eur)\n"
              << "  --mois N          nombre de mois à prédire (défaut 12)\n"
              << "  --debut YYYY-MM   premier mois prédit (défaut 2025-01)\n"
              << "  --sortie FICHIER  exporte les prédictions en CSV\n"
              << "  -h, --help        cette aide\n";
}

int main(int argc, char** argv) {
    std::string csv, out, start = "2025-01";
    int months = 12;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&](const char* name) -> std::string {
            if (i + 1 >= argc) {
                std::cerr << "Erreur : " << name << " attend une valeur\n";
                std::exit(2);
            }
            return argv[++i];
        };
        if (a == "-h" || a == "--help") { usage(argv[0]); return 0; }
        else if (a == "--csv") csv = next("--csv");
        else if (a == "--mois") months = std::atoi(next("--mois").c_str());
        else if (a == "--debut") start = next("--debut");
        else if (a == "--sortie") out = next("--sortie");
        else { std::cerr << "Option inconnue : " << a << "\n"; usage(argv[0]); return 2; }
    }

    try {
        GoldPricePredictor predictor = csv.empty() ? GoldPricePredictor()
                                                   : GoldPricePredictor::fromCsv(csv);
        if (csv.empty())
            std::cout << "(données d'exemple FICTIVES — utilisez --csv pour vos données)\n";

        const auto predictions = predictor.predict(months, start);
        std::printf("Tendance or : %+.2f EUR/mois (R² = %.3f) | taux : %+.5f/mois (R² = %.3f)\n\n",
                    predictor.goldModel().slope, predictor.goldModel().r2,
                    predictor.rateModel().slope, predictor.rateModel().r2);
        GoldPricePredictor::print(predictions);
        if (!out.empty()) {
            GoldPricePredictor::exportCsv(predictions, out);
            std::cout << "\nPrédictions exportées dans " << out << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Erreur : " << e.what() << "\n";
        return 1;
    }
    return 0;
}

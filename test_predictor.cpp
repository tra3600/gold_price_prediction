#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>

#include "GoldPricePredictor.h"

static bool near(double a, double b, double eps = 1e-6) { return std::fabs(a - b) < eps; }

int main() {
    // Régression exacte sur une droite : y = 100 + 10 x
    LinearModel m = LinearModel::fit({100, 110, 120, 130, 140});
    assert(near(m.slope, 10) && near(m.intercept, 100) && near(m.r2, 1.0));
    assert(near(m.predict(5), 150) && near(m.stdError, 0));

    // Prédiction sur données parfaitement linéaires
    GoldPricePredictor p({100, 110, 120, 130}, {1.0, 1.1, 1.2, 1.3});
    auto pr = p.predict(2, "2025-12");
    assert(pr.size() == 2);
    assert(pr[0].label == "2025-12" && pr[1].label == "2026-01");  // passage d'année
    assert(near(pr[0].goldEUR, 140) && near(pr[0].rate, 1.4));
    assert(near(pr[0].goldUSD, 140 * 1.4));
    assert(near(pr[0].goldEURLow, pr[0].goldEURHigh));  // aucun bruit

    // Les intervalles s'élargissent avec l'horizon sur données bruitées
    GoldPricePredictor d;
    auto pd = d.predict(12);
    assert(pd[0].goldEURHigh - pd[0].goldEURLow < pd[11].goldEURHigh - pd[11].goldEURLow);
    assert(pd[0].goldEURLow < pd[0].goldEUR && pd[0].goldEUR < pd[0].goldEURHigh);
    // predict() est sans effet de bord : deux appels donnent le même résultat
    assert(near(d.predict(12)[5].goldEUR, pd[5].goldEUR));

    // Erreurs
    bool threw = false;
    try { GoldPricePredictor({1, 2, 3}, {1, 2}); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    threw = false;
    try { d.predict(0); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);
    threw = false;
    try { d.predict(3, "2025-13"); } catch (const std::invalid_argument&) { threw = true; }
    assert(threw);

    // CSV
    {
        std::ofstream f("test_data.csv");
        f << "mois,or_eur,taux\n2024-01,100,1.0\n2024-02,110,1.1\n\n2024-03,120,1.2\n";
    }
    GoldPricePredictor c = GoldPricePredictor::fromCsv("test_data.csv");
    assert(near(c.goldModel().slope, 10));
    std::remove("test_data.csv");

    std::cout << "Tous les tests passent\n";
    return 0;
}
